#include <upcxx.h>
#include <stdio.h>
#include <iostream>

using namespace upcxx;

typedef struct Box_ {
  int i;
  int j;
  int k;
  global_ptr<double> data;
} Box;

struct Domain {
  int num_boxes;
  shared_array< global_ptr<Box> > boxes;
};

Domain dom;

int main(int argc, char **argv)
{
  upcxx::init(&argc, &argv);

  dom.num_boxes = 8*8*8;

  dom.boxes.init(dom.num_boxes, 1); // cyclic distribution

  // dom.boxes.init(dom.num_boxes, dom.num_boxes/THREADS); // blocked distribution

  for (int i=MYTHREAD; i<dom.num_boxes; i += THREADS) {
    dom.boxes[i] = upcxx::allocate<Box>(i%THREADS, 1);
  }

  upcxx::barrier();

  if (MYTHREAD == 0) {
    for (int i=0; i<8; i++) {
      for (int j=0; j<8; j++) {
        for (int k=0; k<8; k++) {
          int idx = i*8*8 + j*8 + k;
          global_ptr<Box> box = dom.boxes[idx];
          memberof(box, i) = i;
          memberof(box, j) = j;
          memberof(box, k) = k;
          memberof(box, data) = upcxx::allocate<double>(idx%THREADS, 4);
        }
      }
    }
  }

  // print out box data structures to verify
  upcxx::barrier();

  if (MYTHREAD == 1) {
    for (int i=0; i<8; i += 1) {
      for (int j=0; j<8; j += 1) {
        for (int k=0; k<8; k += 1) {
          int idx = i*8*8 + j*8 + k;
          global_ptr<Box> box = dom.boxes[idx];
          //printf("box(%d, %d, %d) \n", memberof(box, i).get(),  memberof(box, j).get(), memberof(box, k).get());
          std::cout << "box(" << memberof(box, i) << ", " << memberof(box, j) << ", " << memberof(box, k) 
                    << ")->data = " << memberof(box, data).get() << endl;
        }
      }
    }
  }

  upcxx::barrier();

  upcxx::finalize();
  return 0;
}
