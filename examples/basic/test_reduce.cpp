/**
 * Test the reduce collective functions
 */

#include <upcxx.h>

using namespace upcxx;

template<class T>
void test(upcxx_datatype_t dt, size_t count)
{
  ptr_to_shared<T> src;
  ptr_to_shared<T> dst;
  
  src = allocate<T>(my_node, count);
  dst = allocate<T>(my_node, count);
  
#ifdef DEBUG
  cerr << my_node << " scr: " << src << "\n";
  cerr << my_node << " dst: " << dst << "\n";
#endif
  
  // Initialize data pointed by host_ptr by a local pointer
  T *lsrc = (T *)src;
  for (int i=0; i<count; i++) {
    lsrc[i] = (T)i + (T)MYTHREAD / THREADS;
  }
  
  barrier();
  
  uint32_t root = 0;
  upcxx_reduce<T>(src, dst, count, root, UPCXX_MAX, dt);
  
  if (MYTHREAD == 0) {
    cout << my_node << " dst: ";
    T *ldst = (T *)dst;
    for (int i=0; i<count; i++) {
      cout << ldst[i] << " ";
    }
    cout << endl;
  }
  
  upcxx_reduce<T>(src, dst, count, root, UPCXX_SUM, dt);
  
  if (MYTHREAD == 0) {
    cout << my_node << " dst: ";
    T *ldst = (T *)dst;
    for (int i=0; i<count; i++) {
      cout << ldst[i] << " ";
    }
    cout << endl;
  }

  barrier();
}

int main(int argc, char **argv)
{
  // \Todo: group all init functions together in a global init function
  init(&argc, &argv);
  
  test<double>(UPCXX_DOUBLE, 10);
  
  test<int>(UPCXX_INT, 5);
  
  finalize();

  return 0;

}
