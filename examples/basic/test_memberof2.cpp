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

  // cyclic distribution
  /*
  Domain(int num) : num_boxes(num), boxes(num)
  { }
  */

  // blocked distribution
  Domain(int num) : num_boxes(num), boxes(num, num/ranks())
  { }
};

Domain dom(8*8*8);

int main(int argc, char **argv)
{
  for (int i=myrank(); i<dom.num_boxes; i += ranks()) {
    dom.boxes[i] = upcxx::allocate<Box>(i%ranks(), 1);
  }

  upcxx::barrier();

  // printf("Here 1\n");

  if (myrank() == 0) {
    for (int i=0; i<8; i++) {
      for (int j=0; j<8; j++) {
        for (int k=0; k<8; k++) {
          int idx = i*8*8 + j*8 + k;
          global_ptr<Box> box = dom.boxes[idx];
          // printf("Here 2 idx %i\n", idx);
          upcxx_memberof(box, i) = i;
          upcxx_memberof(box, j) = j;
          upcxx_memberof(box, k) = k;
          // printf("Here 3\n");
          global_ptr<global_ptr<double> > arrays = upcxx_memberof(box, data) = upcxx::allocate< global_ptr<double> >(idx%ranks(), 4);
          for (int l=0; l<4; l++) {
            // allocate the actual data on a different thread (note "idx+1")
            arrays[l] = upcxx::allocate<double>((idx+1)%ranks(), 16);
            // printf("Here 4\n");
            for (int m=0; m<16; m++) {
#ifdef UPCXX_HAVE_CXX11
              arrays[l][m] = idx*100 + m;
#else
              arrays[l].get()[m] = idx*100 + m;
#endif
            }
            // printf("Here 5\n");
          }
        }
      }
    }
  }

  // print out box data structures to verify
  upcxx::barrier();

  if (myrank() == 1) {
    for (int i=0; i<8; i += 1) {
      for (int j=0; j<8; j += 1) {
        for (int k=0; k<8; k += 1) {
          int idx = i*8*8 + j*8 + k;
          global_ptr<Box> box = dom.boxes[idx];
          std::cout << "box(" << upcxx_memberof(box, i) << ", "
                    << upcxx_memberof(box, j) << ", " << upcxx_memberof(box, k)
                    << ")->data = " << upcxx_memberof(box, data).get() << "\n";
          global_ptr<global_ptr<double> > arrays = upcxx_memberof(box, data);
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
  return 0;
}
