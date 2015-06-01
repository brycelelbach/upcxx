/**
 * \example test_copy_and_set.cpp
 *
 * Test async_copy_and_set, which asynchronously copy data and set a
 * flag on remote process after the data transfer is complete.
 *
 */
#include <upcxx.h>
#include <iostream>

using namespace std;
using namespace upcxx;

// #define DEBUG 1

#define NUM_PEERS 4
#define DEFAULT_COUNT 16

//#ifdef UPCXX_HAVE_CXX11
//enum class Neighbor { NORTH = 0, SOUTH = 1, EAST = 2, WEST = 3};
typedef enum { NORTH = 0, SOUTH = 1, EAST = 2, WEST = 3} Neighbor;

struct Buffer {
  flag_t flag; // polled (read) locally and only set by remote peer
  double data[0]; // this is just a marker for the payload data, actual size may vary
  flag_t *get_flag_addr() { return &flag; }
  double *get_data_addr() { return &data[0]; }
};

global_ptr<Buffer> allocate_buffer(int n)
{
  // allocate the buffer that can hold a flag and n doubles
  size_t nbytes = n*sizeof(double)+sizeof(flag_t);
  return (global_ptr<Buffer>)upcxx::allocate<char>(myrank(), nbytes);
}

void deallocate_buffer(global_ptr<Buffer> b)
{
  upcxx::deallocate(b);
}

shared_array <global_ptr<Buffer>[NUM_PEERS]> inbuffers(ranks());

shared_array <global_ptr<Buffer>[NUM_PEERS]> outbuffers(ranks());

global_ptr<flag_t> my_peers_sendflags[NUM_PEERS]; // peer's inbuffer->flag
global_ptr<double> my_peers_buffers[NUM_PEERS];
global_ptr<flag_t> my_peers_recvflags[NUM_PEERS]; // outbuffer->flag

global_ptr<Buffer> *my_inbuffers, *my_outbuffers;

//  row-major grid layout, i.e., consecutive ranks goes in the row direction
uint32_t grid_cord_to_rank(uint32_t my_row, uint32_t my_col,
                           uint32_t nrows, uint32_t ncols)
{
  uint32_t rank;

  assert(my_col < ncols);
  assert(my_row < nrows);

  rank = my_row * ncols + my_col;

  assert(rank <= ranks());

  return rank;
}

void rank_to_grid_cord(uint32_t rank, uint32_t nrows, uint32_t ncols,
                       uint32_t *my_row, uint32_t *my_col)
{
  assert(rank <= ranks());

  *my_row = rank / ncols;
  *my_col = rank % ncols;
}

uint32_t neighbor_to_rank(Neighbor d, uint32_t nrows, uint32_t ncols)
{
  uint32_t my_row, my_col, peer_rank;

  rank_to_grid_cord(myrank(), nrows, ncols, &my_row, &my_col);

  switch(d) {
  case NORTH:
    peer_rank = grid_cord_to_rank((my_row + 1)%nrows, my_col, nrows, ncols);
    break;
  case SOUTH:
    peer_rank = grid_cord_to_rank((my_row + nrows - 1)%nrows, my_col, nrows, ncols);
    break;
  case EAST:
    peer_rank = grid_cord_to_rank(my_row, (my_col + 1)%ncols, nrows, ncols);
    break;
  case WEST:
    peer_rank = grid_cord_to_rank(my_row, (my_col + ncols - 1)%ncols, nrows, ncols);
    break;
  default:
    printf("Error: wrong direction of neighbor %u\n", d);
    exit(1);
  }

  return peer_rank;
}

Neighbor reciprocal_dir(Neighbor d)
{
  Neighbor o;
  switch(d) {
  case NORTH: o = SOUTH; break;
  case SOUTH: o = NORTH; break;
  case EAST:  o = WEST;  break;
  case WEST:  o = EAST;  break;
  default:
    printf("Error: wrong direction of neighbor %u\n", d);
    exit(1);
  }
  return o;
}

inline double compute_buffer_data(int i, uint32_t rank, Neighbor peer, int level)
{
  return (double)i + (double)peer*1e3 + (double)rank*1e6 + (double)level*1e9;
}
  
void set_buffer_data(double *buf, int n, uint32_t rank, Neighbor peer, int level)
{
  assert(buf != NULL);
  for (int i = 0; i < n; i++) {
    buf[i] = compute_buffer_data(i, rank, peer, level);
  }
}

void clear_buffer_data(double *buf, int n)
{
  assert(buf != NULL);
  for (int i = 0; i < n; i++) {
    buf[i] = 0.0;
  }
}

int verify_buffer_data(double *buf, int n, uint32_t rank, Neighbor peer, int level)
{
  int num_errors = 0;
  double expected;
  assert(buf != NULL);
  for (int i = 0; i < n; i++) {
    expected = compute_buffer_data(i, rank, peer, level);
    if (buf[i] != expected) {
      num_errors++;
      printf("My rank %u data verification error (rank %u, peer %u, buf %p): buf[%d]=%f but expecting %f.\n",
             myrank(), rank, peer, buf, i, buf[i], expected);      
    }
  }
  return num_errors;
}

void test_async_copy_and_set(int count, uint32_t nrows, uint32_t ncols)
{
  // printf("Testing test_copy with %d doubles...\n", count);

  // Initialize data pointed by ptr by a local pointer
  const int total_levels = 3;

  for (uint32_t peer = 0; peer<NUM_PEERS; peer++) {
    my_inbuffers[peer]->flag = UPCXX_FLAG_VAL_UNSET;
    my_outbuffers[peer]->flag = UPCXX_FLAG_VAL_UNSET;
    double *in = my_inbuffers[peer]->get_data_addr();
    clear_buffer_data(in, count);
  }

  barrier();

  // start timer

  // assume circular boundary which is simpler
  for (int l=0; l<total_levels; l++) {
#if DEBUG
    printf("Rank %u starts level %d of %d\n", myrank(), l, total_levels);
#endif

    for (uint32_t peer = 0; peer<NUM_PEERS; peer++) {
      double *out = my_outbuffers[peer]->get_data_addr();
      set_buffer_data(out, count,
                      neighbor_to_rank((Neighbor)peer, nrows, ncols),
                      (Neighbor)peer, l);
    }

    // Init my send buffer based of the current level
    int num_send = 0;
    int num_recv = 0;

    // Notify remote peer that my buffer is ready to receive
    for (uint32_t peer = 0; peer < NUM_PEERS; peer++) {
      async_set_flag(my_peers_sendflags[peer]); // set remote ready send flag
    }

#if DEBUG
    printf("Rank %u finishes async_set_flag at level %d.\n", myrank(), l);
#endif
      
    while (num_send < NUM_PEERS || num_recv < NUM_PEERS) {
      for (uint32_t peer = 0; peer < NUM_PEERS; peer++) {
        // check if any peer is ready to receive my data
        if (my_outbuffers[peer]->flag == UPCXX_FLAG_VAL_SET) {
          // now we can send to the peer rank (its buffer is ready)
          // send the data to remote buffer and set the data available flag on the remote rank
          async_copy_and_set<double>(global_ptr<double>(my_outbuffers[peer]->data),
                                     my_peers_buffers[peer],
                                     count,
                                     my_peers_recvflags[peer]);
          my_outbuffers[peer]->flag = UPCXX_FLAG_VAL_UNSET;
          num_send++;
#if DEBUG
          printf("Rank %u level %u sent data to peer %u, num_send %d, num_recv %d\n",
                 myrank(), l, peer, num_send, num_recv);
#endif
        }
        // check if any data has arrived to my buffer
        if (my_inbuffers[peer]->flag == UPCXX_FLAG_VAL_SET) {
          // unpack data in buffer[peer];
          // Verify data
          int num_errors = verify_buffer_data(my_inbuffers[peer]->data,
                                              count,
                                              myrank(),
                                              reciprocal_dir((Neighbor)peer),
                                              l);
          my_inbuffers[peer]->flag = UPCXX_FLAG_VAL_UNSET; // clear the data available flag
          num_recv++;
#if DEBUG
          printf("Rank %u level %u received data from peer %u, num_send %d, num_recv %d\n",
                 myrank(), l, peer, num_send, num_recv);
#endif
        }
        advance();
      } // end of for peer loop
    } // end of while loop
    barrier();
    // At this point all data exchange at this level is done
#if DEBUG
    printf("Rank %u finished level %d\n", myrank(), l);
#endif
  } // end of for l loop

  barrier();
  
  // end timer 

  // Output the timer info
}

int main (int argc, char **argv)
{
  int count = DEFAULT_COUNT;

  if (argc > 1) {
    if (strcmp(argv[1], "-h") == 0) {
      printf("usage: %s num_of_doubles_per_buffer nrows_of_processor_grid \n",
             argv[0]);
      return 0;
    }

    count = atoi(argv[1]);
    if (count < 1) {
      count = DEFAULT_COUNT;
    }
  }

  uint32_t nrows, ncols;

  // nrows * ncols must equal to ranks()!
  nrows = 1;
  if (argc > 2) {
    nrows = atoi(argv[2]);
  }
  // try to fix pgrid_nrow if it is incorrect
  if (nrows < 1 || nrows > ranks()) {
    nrows = 1;
  }
  ncols = ranks() / nrows;
  assert(nrows * ncols == ranks());

  if (myrank() == 0) {
    printf("# elements per buffer %u, process grid (nrows %u, ncols %u)\n",
           count, nrows, ncols);
  }

  my_inbuffers = (global_ptr<Buffer> *)&inbuffers[myrank()][0];
  my_outbuffers = (global_ptr<Buffer> *)&outbuffers[myrank()][0];

  for (uint32_t peer = 0; peer < NUM_PEERS; peer++) {
    my_inbuffers[peer] = allocate_buffer(count);
    my_outbuffers[peer] = allocate_buffer(count);
  }

  barrier();

  // Get a local copy of my peer's buffer and flag addresses
  for (uint32_t peer = 0; peer < NUM_PEERS; peer++) {
    uint32_t peer_rank = neighbor_to_rank((Neighbor)peer, nrows, ncols);
    global_ptr<Buffer> tmp =  inbuffers[peer_rank][reciprocal_dir((Neighbor)peer)];
    my_peers_recvflags[peer] = (global_ptr<flag_t>)tmp;
    my_peers_buffers[peer] = (global_ptr<double>)((global_ptr<char>)tmp+sizeof(flag_t));
    tmp = outbuffers[peer_rank][reciprocal_dir((Neighbor)peer)];
    my_peers_sendflags[peer] = (global_ptr<flag_t>)tmp;
  }

#if DEBUG
    for (int r = 0; r < ranks(); r++) {
      if (myrank() == r) {
        for (uint32_t peer = 0; peer < NUM_PEERS; peer++) {
          uint32_t peer_rank = neighbor_to_rank((Neighbor)peer, nrows, ncols);
          printf("Rank %u, my_inbuffers[%u] = ", myrank(), peer);
          std::cout << my_inbuffers[peer] << "\n";
          printf("Rank %u, my_outbuffers[%u] = ", myrank(), peer);
          std::cout << my_outbuffers[peer] << "\n";
          printf("Rank %u peer %u peer_rank %u reciprocal_dir %u\n",
                 myrank(), peer, peer_rank, reciprocal_dir((Neighbor)peer));
          std::cout << "Rank " << myrank() << " peer " << peer
                    << " my_peers_sendflags[peer]=" << my_peers_sendflags[peer]
                    << " my_peers_buffers[peer]=" <<  my_peers_buffers[peer]  << "\n";
          std::cout << "Rank " << myrank() << " peer " << peer 
                    << " my_peers_recvflags[peer]=" << my_peers_recvflags[peer]
                    << "\n";
        }
      }
      barrier();
    }
#endif

  
  barrier();
  
  test_async_copy_and_set(count, nrows, ncols);

  barrier();
  
  for (uint32_t peer = 0; peer < NUM_PEERS; peer++) {
    deallocate_buffer(my_inbuffers[peer]);
    deallocate_buffer(my_outbuffers[peer]);
  }

  if (myrank() == 0)
    printf("Passed test_copy_and_set!\n");
  
  return 0;
}
