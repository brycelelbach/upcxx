#include <upcxx.h>
#include <stdio.h>
#include <iostream>

using namespace std;
using namespace upcxx;

typedef struct Box_ {
  int i;
  int j;
  int k;
  global_ptr< global_ptr<double> > data;
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
          global_ptr<global_ptr<double> > arrays = memberof(box, data) = upcxx::allocate< global_ptr<double> >(idx%THREADS, 4);
          for (int l=0; l<4; l++) {
            // allocate the actual data on a different thread (note "idx+1")
            arrays[l] = upcxx::allocate<double>((idx+1)%THREADS, 16);
            for (int m=0; m<16; m++) {
#ifdef UPCXX_HAVE_CXX11
              arrays[l][m] = idx*100 + m;
#else
              arrays[l].get()[m] = idx*100 + m;
#endif
            }
          }
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
          std::cout << "box(" << memberof(box, i) << ", " << memberof(box, j) << ", " << memberof(box, k) 
                        << ")->data = " << memberof(box, data).get() << "\n";
          global_ptr<global_ptr<double> > arrays = memberof(box, data);
          for (int l=0; l<2; l++) {
            for (int m=0; m<8; m++) {
#ifdef UPCXX_HAVE_CXX11
              std::cout << arrays[l][m] << " ";
#else
              std::cout << arrays[l].get()[m] << " ";
#endif
            }
          }
          cout << "\n";
        }
      }
    }
  }

  upcxx::barrier();

  upcxx::finalize();
  return 0;
}
