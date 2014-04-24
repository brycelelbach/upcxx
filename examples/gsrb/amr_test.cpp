#include <cstdlib>
#include <cstring>
#include "amr_test.h"
#include "amr_level.h"

execution_log amr_test::log;
string amr_test::bb = "begin barrier";
string amr_test::eb = "end_barrier";

int main(int argc, char **argv) {
  init(&argc, &argv);

  int length_x = 32;
  int length_y = 32;
  int np = 1, maxi = 10;  // Will become num_patches and iters later.
    
  for (int i = 1; (i < argc && !strncmp(argv[i], "-", 1));
       i++) {
    if (!strncmp(argv[i], "-Lx", 3)) {
      length_x = atoi(argv[++i]);
    }
    if (!strncmp(argv[i], "-Ly", 3)) {
      length_y = atoi(argv[++i]);
    }
    if (!strncmp(argv[i], "-P", 2)) {
      np = atoi(argv[++i]);
    }
    if (!strncmp(argv[i], "-I", 2)) {
      maxi = atoi(argv[++i]);
    }
  }

  amr_test::log.init(3);

  int num_patches = np;
  int iters = maxi;
  double h = 1.0/length_y;
    
  ndarray<RectDomain<2>, 1> patch(RECTDOMAIN((0), (num_patches)));

  int minx = 0; 
  int maxx = length_x - 1;
  for (int i = 0; i < num_patches; i++) {
    patch[i] = RECTDOMAIN((minx, 0), (maxx+1, length_y));
    minx += length_x;
    maxx += length_x;
  }

  ndarray<int, 1> owner(RECTDOMAIN((0), (num_patches)));
  for (int i = 0; i < THREADS; i++) {
    owner[i] = i%THREADS;
  }

  ndarray<ndarray<double, 2, global>, 1> input(RECTDOMAIN((0), (num_patches)));
  ndarray<ndarray<double, 2, global>, 1> soln(RECTDOMAIN((0), (num_patches)));

  ndarray<double, 2, global> ip, sn;
  foreach (i, input.domain()) {
    if (MYTHREAD == owner[i]) {
      ip = ndarray<double, 2, global>(patch[i]);
      sn = ndarray<double, 2, global>(patch[i]);
    }
    input[i] = broadcast(ip, owner[i]);
    soln[i] = broadcast(sn, owner[i]);
  }	

  foreach (i, input.domain()) {
    if (owner[i] == MYTHREAD) {
      input[i].set(1.);
      soln[i].set(0.);
    }
  }
  //amr_level a = new amr_level(patch, h);
  amr_level a(input, soln, owner, h);
  a.verify();

  double error;
  timer t;
  int i;
  // log = new execution_log(3);
  t.start();

  for (i = 0; i < iters; i++) {
    a.smooth_level();
  }
  t.stop();
  execution_log::end(amr_test::log);
  execution_log::histDump();
  //    a.report_time();
  if (MYTHREAD == 0) {
    println("iterations: " << iters << ", Total time: " << t.secs());
  }
  t.reset();
  t.start();
  error = a.errorest_level();
  t.stop();
  if (MYTHREAD == 0) {
    println("error: " << error << ", time: " << t.secs());
  }

  finalize();
}
