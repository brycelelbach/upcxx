/**
 * \example test_copy_and_signal.cpp
 *
 * Test async_copy_and_signal, which asynchronously copy data and then
 * signal a event on the remote process after the transfer is complete.
 *
 */
#include <upcxx.h>
#include <iostream>
#include <map>

using namespace std;
using namespace upcxx;

// #define DEBUG 1
#define VERBOSE

#define NUM_PEERS 4
#define DEFAULT_COUNT 16

//#ifdef UPCXX_HAVE_CXX11
//enum class Neighbor { NORTH = 0, SOUTH = 1, EAST = 2, WEST = 3};
typedef enum { NORTH = 0, SOUTH = 1, EAST = 2, WEST = 3} Neighbor;

global_ptr<double> allocate_buffer(int n)
{
  size_t nbytes = n*sizeof(double);
  return (global_ptr<double>)upcxx::allocate<char>(myrank(), nbytes);
}

void deallocate_buffer(global_ptr<double> b)
{
  upcxx::deallocate(b);
}

shared_array <global_ptr<double>[NUM_PEERS]> inbuffers; // (ranks());
shared_array <global_ptr<double>[NUM_PEERS]> outbuffers; // (ranks());
global_ptr<double> my_peers_buffers[NUM_PEERS];
global_ptr<double> *my_inbuffers, *my_outbuffers;

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
#ifdef VERBOSE
      printf("My rank %u data verification error (rank %u, peer %u, buf %p): buf[%d]=%f but expecting %f.\n",
             myrank(), rank, peer, buf, i, buf[i], expected);
#endif
    }
  }
  return num_errors;
}

event my_send_event;
event my_recv_event;
int num_sends=0;

struct SendInfo {
  global_ptr<void> src_ptr;
  global_ptr<void> dst_ptr;
  size_t nbytes;
  int SeqNum;
  event *signal_event;
  event *done_event;

  SendInfo(global_ptr<void> src,
           global_ptr<void> dst,
           size_t nb,
           int SN,
           event *signal_e,
           event *done_e)
    : src_ptr (src),
      dst_ptr (dst),
      nbytes  (nb),
      SeqNum  (SN),
      signal_event(signal_e),
      done_event(done_e)
  { ; }

  void check()
  {
    assert(src_ptr != NULL);
    assert(dst_ptr != NULL);
    assert(nbytes > 0);
    assert(signal_event != NULL);
    assert(done_event != NULL);
  }
};

typedef std::multimap< upcxx::rank_t, SendInfo > pgas_send_info_map_t;
static pgas_send_info_map_t pgas_send_info_map;

void pgas_send(global_ptr<void> src,
               global_ptr<void> dst,
               size_t nbytes,
               int SeqNum,
               event *signal_event,
               event *done_event)
{
#ifdef DEBUG1
  std::cout << "myrank " << myrank() << " pgas_send: src " << src << " dst " << dst << " nbytes " << nbytes
            << " SeqNum " << SeqNum << " signal_event " << signal_event
            << " done_event " << done_event << "\n";
#endif

  // We use dst_rank as the key for the hash table
  std::pair <pgas_send_info_map_t::iterator, pgas_send_info_map_t::iterator> ret;
  ret = pgas_send_info_map.equal_range(dst.where());

  // try to match the SeqNum
  for (pgas_send_info_map_t::iterator it = ret.first;
       it != ret.second;
       ++it) {
    SendInfo& send_info = it->second;
    if (SeqNum == send_info.SeqNum) {
      // found the matched message
      // Check if data size matches
      assert(nbytes == send_info.nbytes);

      // Fire off the non-blocking one-sided communication (put)
      // If the receive request comes first, then the src_ptr in the existing
      // send_info entry should be NULL; otherwise, if the send request comes
      // first, then the dst_ptr must be NULL.  If neither is true, then there
      // must be something wrong!
      if (send_info.src_ptr == NULL) {
        // pgas_send request from Recv came earlier
        send_info.src_ptr = src;
        send_info.done_event = done_event;
#ifdef DEBUG
        std::cout << "myrank " << myrank() << " send found SeqNum match "
                  << SeqNum << "\n";
#endif
      } else {
        // pgas_send request from Send came earlier
        assert(send_info.dst_ptr == NULL);
        send_info.dst_ptr = dst;
        send_info.signal_event = signal_event;
#ifdef DEBUG
        std::cout << "myrank " << myrank() << " recv found SeqNum match "
                  << SeqNum << "\n";
#endif
      }

      send_info.check();

      async_copy_and_signal(send_info.src_ptr,
                            send_info.dst_ptr,
                            send_info.nbytes,
                            send_info.signal_event,
                            send_info.done_event);
      num_sends++;
      // Delete the send_info from the map
      pgas_send_info_map.erase(it);
      return;
    }
  }

  // Can't find the send_info entry in the hash table
  // Create a new send_info and store the receiver part of it
  SendInfo send_info(src, dst, nbytes, SeqNum, signal_event, done_event);
  pgas_send_info_map.insert(std::pair<upcxx::rank_t, SendInfo>(dst.where(), send_info));
}

void test_async_copy_and_set(int count, uint32_t nrows, uint32_t ncols)
{
  // printf("Testing test_copy with %d doubles...\n", count);

  // Initialize data pointed by ptr by a local pointer
  const int total_levels = 3;

  for (uint32_t peer = 0; peer<NUM_PEERS; peer++) {
    double *in = (double *)my_inbuffers[peer];
    clear_buffer_data(in, count);
  }

  barrier();

  // start timer

  // assume circular boundary which is simpler
  for (int l=0; l<total_levels; l++) {
#if DEBUG
    printf("Rank %u starts level %d of %d\n", myrank(), l, total_levels);
#endif

    // Init my send buffer based of the current level
    for (uint32_t peer = 0; peer<NUM_PEERS; peer++) {
      double *out = (double *)my_outbuffers[peer];
      set_buffer_data(out, count,
                      neighbor_to_rank((Neighbor)peer, nrows, ncols),
                      (Neighbor)peer, l);
    }

    // Each rank is expected to receive NUM_PEERS messages
    my_recv_event.incref(NUM_PEERS);
    num_sends = 0; // reset num_sends

    // Notify remote peer that my buffer is ready to receive
    for (uint32_t peer = 0; peer < NUM_PEERS; peer++) {
      uint32_t peer_rank = neighbor_to_rank((Neighbor)peer, nrows, ncols);
      async(peer_rank)(pgas_send, global_ptr<void>(NULL, peer_rank),
                       (global_ptr<void>)my_inbuffers[peer],
                       count*sizeof(double), l*1000+reciprocal_dir((Neighbor)peer), &my_recv_event, (event *)NULL);
    }

    for (uint32_t peer = 0; peer < NUM_PEERS; peer++) {
      uint32_t peer_rank = neighbor_to_rank((Neighbor)peer, nrows, ncols);
      pgas_send((global_ptr<void>)my_outbuffers[peer],
                global_ptr<void>(NULL, peer_rank), // need the peer_rank info for the hash table
                count*sizeof(double),
                l*1000+peer, // tag (SeqNum)
                (event *)NULL, // this should be the recv event set by my peer
                &my_send_event);
    }

    // printf("start waiting...\n");
    my_recv_event.wait();
    // printf("done waiting recv\n");
    
    /* Need to Make sure all sends are started before waiting on the event! */
    while (num_sends < NUM_PEERS)
      advance();
    my_send_event.wait(); // only wait for the send have been started 
    // printf("done waiting send\n");

    // All data should have arrived
    for (uint32_t peer = 0; peer < NUM_PEERS; peer++) {
      // unpack data in buffer[peer];
      // Verify data
      int num_errors = verify_buffer_data((double *)my_inbuffers[peer],
                                          count,
                                          myrank(),
                                          reciprocal_dir((Neighbor)peer),
                                          l);
      if (num_errors > 0) {
        printf("Rank %u: test_copy_and_signal failed: %d data verification errors!\n",
               myrank(), num_errors);
        exit(1);
      }
    }

    // proceed to the next level
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
  init(&argc, &argv);

  inbuffers.init(ranks());
  outbuffers.init(ranks());

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

  my_inbuffers = (global_ptr<double> *)&inbuffers[myrank()][0];
  my_outbuffers = (global_ptr<double> *)&outbuffers[myrank()][0];

  for (uint32_t peer = 0; peer < NUM_PEERS; peer++) {
    my_inbuffers[peer] = allocate_buffer(count);
    my_outbuffers[peer] = allocate_buffer(count);
  }

  barrier();

  // Get a local copy of my peer's buffer and flag addresses
  for (uint32_t peer = 0; peer < NUM_PEERS; peer++) {
    uint32_t peer_rank = neighbor_to_rank((Neighbor)peer, nrows, ncols);
    my_peers_buffers[peer] = inbuffers[peer_rank][reciprocal_dir((Neighbor)peer)];
  }

#if DEBUG1
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
    printf("Passed test_copy_and_signal!\n");

  finalize();
  return 0;
}
