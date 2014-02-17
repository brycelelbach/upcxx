#include <upcxx.h>

upcxx::shared_array<volatile int, 1> a;

int main(int argc, char **argv)
{
  upcxx::init(&argc, &argv);
  a.init(10);
  upcxx::finalize();
  return 0;
}
