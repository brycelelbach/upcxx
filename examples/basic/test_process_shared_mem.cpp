/**
 * \example test_process_shared_mem.cpp
 *
 * Test process-shared memory features
 *
 * You can see the process-shared-memory feature in action by changing
 * the size of each supernode (a group of processes sharing physical memory)
 * For example,
 * export GASNET_SUPERNODE_MAXSIZE=1
 * mpirun -n 3 ./test_process_shared_mem
 * export GASNET_SUPERNODE_MAXSIZE=2
 * mpirun -n 3 ./test_process_shared_mem
 * export GASNET_SUPERNODE_MAXSIZE=3
 * mpirun -n 3 ./test_process_shared_mem
 *
 */
#include <upcxx.h>

#include <iostream>

typedef struct Box_ {
  int i;
  int j;
  int k;
  upcxx::global_ptr<double> data;
} Box;

struct Domain {
  int num_boxes;
  upcxx::shared_array< upcxx::global_ptr<Box> > boxes;
};

Domain dom;

int main(int argc, char **argv)
{
  upcxx::init(&argc, &argv);

  // print out pshm_teams
  // std::vector< std::vector<rank_t> > pshm_teams;
  if (upcxx::myrank() == 0) {
    for (uint32_t i=0; i<upcxx::pshm_teams.size(); i++) {
      std::cout << "PSHM team " << i << ": ( ";
      for (uint32_t j=0; j<upcxx::pshm_teams[i].size(); j++) {
        std::cout << upcxx::pshm_teams[i][j] << " ";
      }
      std::cout << ")\n";
    }
  }
  upcxx::barrier();

  dom.num_boxes = 3*3*3;

  dom.boxes.init(dom.num_boxes, 1); // cyclic distribution

  // dom.boxes.init(dom.num_boxes, dom.num_boxes/ranks()); // blocked distribution

  for (int i=upcxx::myrank(); i<dom.num_boxes; i += upcxx::ranks()) {
    dom.boxes[i] = upcxx::allocate<Box>(upcxx::myrank(), 1);
  }

  upcxx::barrier();

  // If I'm the first guy in the shared-memory node, I do the setup.
  if (upcxx::myrank() == 0) {
    for (int i=0; i<3; i++) {
      for (int j=0; j<3; j++) {
        for (int k=0; k<3; k++) {
          int idx = i*3*3 + j*3 + k;
          upcxx::global_ptr<Box> global_box = dom.boxes[idx];
          Box *box = (Box *)global_box;
          if (box != NULL) {
            std::cout << "Rank " << upcxx::myrank() << " is setting up box "
                      << idx << " located on rank " << global_box.where() << " \n";
            box->i = i;
            box->j = j;
            box->k = k;
            box->data = upcxx::allocate<double>(idx%upcxx::ranks(), 3);
          }
        }
      }
    }
  }

  upcxx::barrier();

  // If I'm the last guy in the shared-memory node, I print out data to verify
  if (upcxx::myrank() == 1) {
    for (int i=0; i<3; i += 1) {
      for (int j=0; j<3; j += 1) {
        for (int k=0; k<3; k += 1) {
          int idx = i*3*3 + j*3 + k;
          Box *box = (Box *)dom.boxes[idx].get();
          if (box != NULL) {
            std::cout << "Rank " << upcxx::myrank() << " is reading box " << idx
                      << " (" << box->i << ", " << box->j << ", " << box->k
                      << ")->data = " << box->data << "\n";
          }
        }
      }
    }
    for (uint32_t i=0; i<upcxx::pshm_teams.size(); i++) {
      std::cout << "PSHM team " << i << ": ( ";
      for (uint32_t j=0; j<upcxx::pshm_teams[i].size(); j++) {
        std::cout << upcxx::pshm_teams[i][j] << " ";
      }
      std::cout << ")\n";
    }
  }

  upcxx::barrier();

  upcxx::finalize();
  return 0;
}
