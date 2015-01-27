/**
 * Test the reduce collective functions
 */

#include <upcxx.h>

using namespace upcxx;

template<class T>
void test(upcxx_datatype_t dt, size_t count)
{
  global_ptr<T> src;
  global_ptr<T> dst;
  
  src = allocate<T>(myrank(), count);
  dst = allocate<T>(myrank(), count);
  
#ifdef DEBUG
  cerr << myrank() << " scr: " << src << "\n";
  cerr << myrank() << " dst: " << dst << "\n";
#endif
  
  // Initialize data pointed by host_ptr by a local pointer
  T *lsrc = (T *)src;
  for (int i=0; i<count; i++) {
    lsrc[i] = (T)i + (T)MYTHREAD / THREADS;
  }
  
  barrier();
  
  uint32_t root = 0;
  upcxx_reduce<T>((T *) src, (T *) dst, count, root, UPCXX_MAX, dt);
  
  if (MYTHREAD == 0) {
    cout << myrank() << " dst: ";
    T *ldst = (T *)dst;
    for (int i=0; i<count; i++) {
      cout << ldst[i] << " ";
    }
    cout << endl;
  }
  
  upcxx_reduce<T>((T *) src, (T *) dst, count, root, UPCXX_SUM, dt);
  
  if (MYTHREAD == 0) {
    cout << myrank() << " dst: ";
    T *ldst = (T *)dst;
    for (int i=0; i<count; i++) {
      cout << ldst[i] << " ";
    }
    cout << endl;
  }

  // Need to add verification of the reduce
  
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
