#include <upcxx.h>
#include <upcxx/array.h>
#include <iostream>
using namespace std;
using namespace upcxx;

void my_func( ndarray<double,2,global> block )
{
  cout << "rank " << myrank() << " has data from: " \
       <<  block.domain() << endl;

  ndarray<double,2,global> block2(block.domain());
  block2.copy(block);

  upcxx_foreach( pt, block2.domain() ) {
    cout << "remote_block["<<pt<<"] = " << block2[pt] << endl;
  };

  return;
}


int main(int argc, char** argv)
{
  init(&argc, &argv);

  if (myrank() == 0) {

    point<2> upper_left  = PT(4,4); 
    point<2> lower_right = PT(7,7);
    rectdomain<2> rd = RD( upper_left, lower_right );
    ndarray<double,2,global> myblock( rd );

    cout << "rank " << myrank() << " sending this data:" << endl;
    upcxx_foreach( pt, rd ) {
      myblock[pt] = pt.x[0] + pt.x[1];
      cout << "local_block["<<pt<<"] = " << myblock[pt] << endl;
    };

    async(1)( my_func, myblock );
    async_wait();

  }
  barrier();

  if (myrank() == 0)
    cout << "test_acc_async done.\n";
  
  finalize();
  return 0;
}
