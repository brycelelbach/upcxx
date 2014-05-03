#include <cstdlib>
#include <cstring>
#include <fstream>
#include "pmg_test.h"
#include "pmg.h"

int pmg_test::barrierCount;
string pmg_test::bb = "begin barrier";
string pmg_test::eb = "end barrier";
execution_log pmg_test::log;  // stuff to track execution.

void pmg_test::main(int argc, char **argv) {
  string phase;
  log.init(2);
  phase = "Read inputs";
  log.begin(phase);

  /** Various defaults. */
  int reporting = 0;
  int C = 1;
  int star_shaped = 0;
  int Nref = 4;
  int id_border = 10;
  int LongDim = 128;
  int S = 9;
  int P = 0;
  int NumPatches = 1;
  int M = 0;
  bool from_file = false;
  string filename;
  bool solve = true;
  bool report = false;
  double H;

  // println(THREADS + " processors");

  pmg_test::barrier();

  /*if (MYTHREAD == 0) {*/
  for (int i = 0; (i < argc && !strncmp(argv[i], "-", 1)); 
       i++) {
    if (!strncmp(argv[i], "-C", 2)) {
      C = atoi(argv[++i]);
    }
    else if (!strncmp(argv[i], "-Nr", 3)) {
      Nref = atoi(argv[++i]);
    }
    else if (!strncmp(argv[i], "-Np", 3)) {
      NumPatches = atoi(argv[++i]);
    }
    else if (!strncmp(argv[i], "-I", 2)) {
      id_border = atoi(argv[++i]);
    }
    else if (!strncmp(argv[i], "-L", 2)) {
      LongDim = atoi(argv[++i]);
    }
    else if (!strncmp(argv[i], "-W", 2)) {
      M = atoi(argv[++i]);
    }
    else if (!strncmp(argv[i], "-st", 3)) {
      star_shaped = 1;
    }
    else if (!strncmp(argv[i], "-S", 2)) {
      S = atoi(argv[++i]);
    }
    else if (!strncmp(argv[i], "-P", 2)) {
      P = atoi(argv[++i]);
    }
    else if (!strncmp(argv[i], "-r", 2)) {
      reporting = atoi(argv[++i]);
    }
    else if (!strncmp(argv[i], "-no", 3)) {
      solve = false;
    }
    else if (!strncmp(argv[i], "-f", 2)) {
      filename = string(argv[++i]);
      from_file = true;
    }
    else if (!strncmp(argv[i], "-R", 2)) {
      report = true;
    }
  }

  if (report) {
    println("Processor " << MYTHREAD);
  }
  log.end(phase);
      
  pmg_test::barrier();

  phase = "PMG setup";
  log.begin(phase);

  if (MYTHREAD == 0) {
    switch (S) {
    case 9:
      println("Doing 9-point method through coarse solve...");
      break;
    case 95:
      println("Using 5-point MG in higher-accuracy infinite domain solver...");
      break;
    case 59:
      println("Using 9-point MG in lower-accuracy infinite domain solver...");
      break;
    case 5:
      println("Using 5-point MG and lower-accuracy infinite domain solver for the local solves...");
      break;
    default:
      S = 9;
      println("Defaulting back to 9-point method: in future please use -S 9, 5, 59, or 95...");
      break;
    }
    println("Size: " << LongDim << "x" << LongDim 
            << ", Total Patches: " << NumPatches*NumPatches
            << ", Refinement Ratio: " << Nref
            << ", Correction Radius: " << C);
  }

  RectDomain<2> interior(POINT(0, 0),
                         POINT(LongDim+1, LongDim+1));

  H = 1.0/LongDim;
  int npatches = NumPatches;//broadcast(NumPatches, 0);

  ndarray<charge, 1> charges;

  if (from_file) { //broadcast(from_file, 0)) {
    charges = parse_input_file(filename);
  } else {
    charges = ndarray<charge, 1>(RECTDOMAIN((1), (2)));
    charges[1] = charge(0.48, 0.48, 0.42, M, star_shaped);
  }
    
  log.end(phase);

  phase = "PMG constructor";
  log.begin(phase);

  pmg myProblem(interior, npatches, C, Nref, id_border, H, S, charges,
                reporting); //broadcast(reporting, 0));

  log.end(phase);

  pmg_test::barrier();

  timer totalTime;
  if (MYTHREAD == 0) {
    totalTime.start();
  }

  if (solve) { //broadcast(solve, 0)) {
    //      pmg_test::barrier();
    myProblem.solve();
    //      pmg_test::barrier();
  }

  pmg_test::barrier();

  if (MYTHREAD == 0) {
    totalTime.stop();
    println("Time taken: " << totalTime.secs());
  }

  pmg_test::barrier();
  myProblem.wrap_up();
  pmg_test::barrier();
  execution_log::end(log);
  execution_log::histDump();
}

ndarray<charge, 1> pmg_test::parse_input_file(const string &filename) {
  ndarray<charge, 1> charges;
  if (MYTHREAD == 0) {
    ifstream ifs;
    ifs.open(filename.c_str(), ios_base::in);

    int i, m, s;
    double c1, c2, r0;
    ifs >> i;

    if (i > 0) {
      charges = ndarray<charge, 1>(RECTDOMAIN((1), (i+1)));
      foreach (p, charges.domain()) {
        ifs >> c1;
        ifs >> c2;
        ifs >> r0;
        ifs >> m;
        ifs >> s;
        charges[p] = charge(c1, c2, r0, m, s);
      }
    } else {
      println("Bad input format");
      exit(-1);
    }
  }
  int num = broadcast(charges.size(), 0);
  if (MYTHREAD != 0) {
    charges = ndarray<charge, 1>(RECTDOMAIN((1), (num+1)));
  }
  broadcast(charges, charges, 0);
  return charges;
}

int main(int argc, char **argv) {
  init(&argc, &argv);
  pmg_test::main(argc-1, argv+1);
  finalize();
}
