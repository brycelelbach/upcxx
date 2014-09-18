#include <vector>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <string>
#include <sstream>
#include <climits>

#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <math.h>

#define USE_UPCXX

#ifdef USE_UPCXX
#include <upcxx.h>
#endif

using namespace std;

double mysecond()
{
  struct timeval tv;
  gettimeofday(&tv, 0);
  return tv.tv_sec + ((double) tv.tv_usec / 1000000);
}

struct node_t {
  int id;
  int degree;
  int elim_step;
};

void dump_local_nodes(node_t *local_nodes, int n);

bool node_comp(node_t * & a, node_t * & b)
{
  if (a->degree < b->degree) {
    return true;
  } else {
    if(a->degree == b->degree) {
#ifdef USE_RANDOM_TIE_BREAK
      double tmp = fmod(rand(),10.0);
      return (tmp>5.0);
#else
      return a->id < b->id; // use deterministic tie break
#endif
    } else {
      return false;
    }
  }
}

void get_reach(const vector<int> &xadj,
               upcxx::shared_array<int> &adj,
               // const vector<int> &adj,
               upcxx::shared_array<node_t> &nodes,
               // vector<node_t> &nodes,
               const int min_node_id,
               const int elim_step, 
               // vector<node_t*> &reach_set
               vector< upcxx::global_ptr<node_t> > &reach_set)
{
  //this array is used to mark the node so that 
  //they are not added twice into the reachable set
  vector<bool> explored(nodes.size(), false);
  //this list contains the nodes to explore
  
  // vector<node_t *> explore_set;
  vector< upcxx::global_ptr<node_t> > explore_set;
  
  /*
  fprintf(stdout, "Rank %d, get_reach: min_node_id %d, elim_step %d\n",
          upcxx::myrank(), min_node_id, elim_step);
  */
  
  //initialize explore_set with the neighborhood
  // of min_node in the original graph 
  int beg = xadj[min_node_id-1];
  int end = xadj[min_node_id]-1;
  for(int i = beg; i<=end; ++i){
    int curr_adj = adj[i-1]; // remote access once
    if(curr_adj != 0) {
      // node_t * next_node = &nodes[curr_adj-1];
      upcxx::global_ptr<node_t> next_node = &nodes[curr_adj-1];
      explore_set.push_back(next_node);
      explored[curr_adj-1] = true;
    } else {
      fprintf(stderr, "Fatal error: adj[%d]=0 \n", i-1);
      exit(1);
    }
  }  

  //now find path between min_nodes and other nodes
  while (explore_set.size()>0) {
    //pop a node
    // node_t *cur_node = explore_set.back();
    upcxx::global_ptr<node_t> cur_node = explore_set.back();
    
    explore_set.pop_back();

    assert(cur_node.raw_ptr() != NULL);
    
    // if (cur_node->id == min_node_id) {
    if (memberof(cur_node, id) == min_node_id) {
      continue;
    }

    // if (cur_node->elim_step == -1) {
    if (memberof(cur_node, elim_step) == -1) {
      reach_set.push_back(cur_node);
    } else {
      // int beg = xadj[cur_node->id-1];
      int beg = xadj[memberof(cur_node, id) - 1];
      // int end = xadj[cur_node->id]-1;
      int end = xadj[memberof(cur_node, id)] - 1;
      // fprintf(stdout, "beg %d, end %d\n", beg, end);
      for (int i=beg; i<=end; ++i) {
        int curr_adj = adj[i-1]; // remote access once
        if (curr_adj != 0) {
          if (!explored[curr_adj-1]) {
            // node_t *next_node = &nodes[curr_adj-1];
            upcxx::global_ptr<node_t> next_node = &nodes[curr_adj-1];
            
            explore_set.push_back(next_node);
            explored[curr_adj-1] = true;
          }
        } else {
          fprintf(stderr, "Fatal error: adj[%d]=0 \n", i-1);
          exit(1);
        }
      }
    }
  }
}

int main(int argc, char *argv[]) 
{
  upcxx::init(&argc, &argv);

  /* initialize random seed: */
  int seed =time(NULL); 
  srand (seed);

  vector<int> xadj;
  // upcxx::shared_array<int> xadj_shared;
  // xadj doesn't need to be shared because it's static and relatively small.
  
  vector<int> local_adj;
  upcxx::shared_array<int> adj;

  if(argc<2){
    cerr<<"Usage is: "<<argv[0]<<" input.file"<<endl;
    return -1;
  }

  string filename(argv[1]);
  ifstream infile;
  infile.open(filename.c_str());

  string line;
  //read xadj on the first line of the input file
  if(getline(infile, line)) {
    istringstream iss(line);
    int i;
    while(iss>> i){ xadj.push_back(i);}
  } else {
    return -2;
  }

  //read adj on the second line of the input file
  if(getline(infile, line)) {
    istringstream iss(line);
    int i;
    while(iss>> i){ local_adj.push_back(i);}
  } else {
    return -2;
  }

  infile.close();

  // YZ: init the shared array
  adj.init(local_adj.size());
  for (uint64_t i = upcxx::myrank();
       i < local_adj.size();
       i += upcxx::ranks()) {
    adj[i] = local_adj[i];
    // printf("adj[%lu] %d ", i, (int)adj[i]);
  }
  // printf("\n");

  int n = xadj.size()-1;
  // vector<node_t> nodes(n);
  upcxx::shared_array<node_t> nodes(n);
  nodes.init();

  vector<node_t *> remain(n);

  // YZ: comment out unused variables
  // int initEdgeCnt = 0;
  // int edgeCnt = 0;

  //initialize nodes
  for (int i = upcxx::myrank(); i < n; i += upcxx::ranks()) {
    //node_t &cur_node = nodes[i];
    upcxx::global_ptr<node_t> cur_node = &nodes[i];

    // cur_node.id = i+1;
    memberof(cur_node, id) = i+1;

    // cur_node.degree = xadj[i+1] - xadj[i];
    memberof(cur_node, degree) = xadj[i+1] - xadj[i];
    
    // cur_node.elim_step = -1;
    memberof(cur_node, elim_step) = -1;

    // remain[i] = &cur_node;

    /*
    for(int idx = xadj[i]; idx <= xadj[i+1]-1; ++idx) {
      if(local_adj[idx-1]>i+1){
        initEdgeCnt++;
      }
    }
     */
  }

  upcxx::shared_array<int> all_min_degrees(upcxx::ranks());
  upcxx::shared_array<int> all_min_ids(upcxx::ranks());
  all_min_degrees.init();
  all_min_ids.init();
  
  vector<int> schedule;
  // vector< upcxx::global_ptr<node_t> > schedule_shared;

  // YZ: loop-carried dependencies between steps
  //process with MD algorithm
  for (int step=1; step<=n; ++step) {

    // std::make_heap (node_heap.begin(),node_heap.end(),node_comp);
    //get the min degree node
    // std::pop_heap (node_heap.begin(),node_heap.end(),node_comp);
    // node_t &min_node = *node_heap.back();
    // node_heap.pop_back();

    // YZ: we only need to find the minimum here.Every process finds
    // its local minimum of Un-eliminated nodes and then finds the
    // global minimum by Allreduce(MIN)

    /*
    vector<node_t *>::iterator min_node = std::min_element(remain.begin(), remain.end(), node_comp);
    assert(min_node != remain.end());
    */
    
    node_t *local_nodes = (node_t *)&nodes[upcxx::myrank()];
    node_t *my_min = NULL;
    // YZ: need to handle the case when n is not a multiple of ranks()!!
    int local_size = n / upcxx::ranks();
    if (upcxx::myrank() < (n - local_size * upcxx::ranks())) {
      local_size++;
    }
    // printf("step %d, local_size %d\n", step, local_size);
    int cur_min_degree = -1;
    for (int i = 0; i < local_size; i++) {
      node_t *cur_node = &local_nodes[i];
      if (cur_node->elim_step == -1) {
        if (cur_min_degree == -1 || cur_node->degree < cur_min_degree) {
          cur_min_degree = cur_node->degree;
          my_min = cur_node;
        }
      }
    }
    
    if (my_min != NULL) {
      all_min_degrees[upcxx::myrank()] = my_min->degree;
      all_min_ids[upcxx::myrank()] = my_min->id;
    } else {
      all_min_degrees[upcxx::myrank()] = INT_MAX;
      all_min_ids[upcxx::myrank()] = -1;
    }
    /*
    if (my_min != NULL) {
      printf("Rank %d, my_min->degree %d, my_min->id %d\n",
             upcxx::myrank(), my_min->degree, my_min->id);
    }
    */
    upcxx::barrier();
    
    int global_min_id;
    
    if (upcxx::myrank() == 0) {
      int cur_min_degree = INT_MAX;
      int min_rank = 0;
      // find the global min node
      for (int i=0; i<upcxx::ranks(); i++) {
        if (cur_min_degree > all_min_degrees[i]) {
          cur_min_degree = all_min_degrees[i];
          min_rank = i;
        }
      }
      
      /*
      printf("Rank %d, all_min_ids[%d] %d\n",
             upcxx::myrank(), min_rank, all_min_ids[min_rank].get());
       */
      
      upcxx::global_ptr<node_t> min_node = &nodes[all_min_ids[min_rank]-1];
      global_min_id = all_min_ids[min_rank]; // memberof(min_node, id);
      memberof(min_node, elim_step) = step;
      /*
      fprintf(stdout, "Rank %d, min_node->elim_step %d, min_node->id %d\n",
              upcxx::myrank(), memberof(min_node, elim_step).get(),
              memberof(min_node, id).get());
       */
    }
    
    // dump_local_nodes(local_nodes, n);
    
    upcxx::barrier();

    upcxx::upcxx_bcast(&global_min_id, &global_min_id, sizeof(int), 0);

    assert(global_min_id != -1);
    
    /*
    fprintf(stdout, "Rank %d, global_min_id %d\n",
            upcxx::myrank(), global_min_id);
    */
    
    schedule.push_back(global_min_id);

    // (*min_node)->elim_step = step;
    
    //update the degree of its reachable set
    // vector<node_t *> reach;
    vector< upcxx::global_ptr<node_t> > reach;
    
    get_reach(xadj, adj, nodes, global_min_id, step, reach);

    // remain.erase(min_node);

    // YZ: Use UPC++ to parallel this loop.  There is no data dependencies
    // inside the for loop because the get_reach function does not change
    // the original graph (and the adjacency list) though some nodes' degree
    // might be changed.
#ifdef USE_UPCXX
    // vector< upcxx::global_ptr<node_t> >::iterator
    for (auto it = reach.begin() + upcxx::myrank();
        it < reach.end();
        it += upcxx::ranks()) {
#else
#pragma omp parallel for
    for (vector<node_t *>::iterator it = reach.begin();
         it != reach.end();
         ++it) {
#endif
      // node_t *cur_neighbor = *it;
      upcxx::global_ptr<node_t> cur_neighbor = *it;
      
      // vector<node_t *> nghb_reach;
      vector<upcxx::global_ptr<node_t> > nghb_reach;
      
      get_reach(xadj, adj, nodes, memberof(cur_neighbor, id).get(), step+1, nghb_reach);
      // cur_neighbor->degree = nghb_reach.size();
      memberof(cur_neighbor, degree) = nghb_reach.size();
    }
    upcxx::barrier();
  } // close of  for (int step=1; step<=n; ++step)

  for (int i = 0; i < upcxx::ranks(); i++) {
    if (upcxx::myrank() == i) {
      cout << "\n";
      cout<<"Rank " << upcxx::myrank() << " Schedule: ";
      for (int step=1; step<=n; ++step) {
        cout << " " << schedule[step-1];
      }
      cout << "\n";
    }
    upcxx::barrier();
  }

  upcxx::finalize();

  return 0;
}
  
void dump_local_nodes(node_t *local_nodes, int n)
  {
    int local_size = n / upcxx::ranks();
    if (upcxx::myrank() < (n - local_size * upcxx::ranks())) {
      local_size++;
    }
    for (int i = 0; i < local_size; i++) {
      node_t *cur_node = &local_nodes[i];
      fprintf(stdout, "local_nodes[%d], id %d, degree %d, elim_step %d\n",
              i, local_nodes[i].id, local_nodes[i].degree, local_nodes[i].elim_step);
    }
    printf("\n");
  }
  
