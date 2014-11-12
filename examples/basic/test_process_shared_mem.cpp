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

  dom.num_boxes = 4*4*4;

  dom.boxes.init(dom.num_boxes, 1); // cyclic distribution

  // dom.boxes.init(dom.num_boxes, dom.num_boxes/THREADS); // blocked distribution

  for (int i=upcxx::myrank(); i<dom.num_boxes; i += upcxx::ranks()) {
    dom.boxes[i] = upcxx::allocate<Box>(upcxx::myrank(), 1);
  }

  upcxx::barrier();

  // If I'm the first guy in the shared-memory node, I do the setup.
  if (MYTHREAD == 0) {
    for (int i=0; i<4; i++) {
      for (int j=0; j<4; j++) {
        for (int k=0; k<4; k++) {
          int idx = i*4*4 + j*4 + k;
          upcxx::global_ptr<Box> global_box = dom.boxes[idx];
          Box *box = (Box *)global_box;
          if (box != NULL) {
            std::cout << "Rank " << upcxx::myrank() << " is setting up box "
                      << idx << " located on rank " << global_box.where() << " \n";
            box->i = i;
            box->j = j;
            box->k = k;
            box->data = upcxx::allocate<double>(idx%upcxx::ranks(), 4);
          }
        }
      }
    }
  }

  upcxx::barrier();

  // If I'm the last guy in the shared-memory node, I print out data to verify
  if (MYTHREAD == 1) {
    for (int i=0; i<4; i += 1) {
      for (int j=0; j<4; j += 1) {
        for (int k=0; k<4; k += 1) {
          int idx = i*4*4 + j*4 + k;
          Box *box = (Box *)dom.boxes[idx].get();
          if (box != NULL) {
            std::cout << "Rank " << upcxx::myrank() << " is reading box " << idx
                      << " (" << box->i << ", " << box->j << ", " << box->k
                      << ")->data = " << box->data << "\n";
          }
        }
      }
    }
  }

  upcxx::barrier();

  upcxx::finalize();
  return 0;
}
