// Stencil
#include "globals.h"

static int xdim, ydim, zdim;
static int xparts, yparts, zparts;
static ndarray<rectdomain<3>, 1> allDomains;
static rectdomain<3> myDomain;
static ndarray<double, 3 UNSTRIDED> myGridA, myGridB;
static ndarray<global_ndarray<double, 3 UNSTRIDED>, 1> allGridsA, allGridsB;
static ndarray<global_ndarray<double, 3>, 1> targetsA, targetsB;
static ndarray<ndarray<double, 3>, 1> sourcesA, sourcesB;
static int steps;

enum timer_type {
  COMMUNICATION,
  COMPUTATION
};
enum timer_index {
  LAUNCH_COPIES = 0,
  SYNC_COPIES,
  COPY_BARRIER,
  LAUNCH_X_COPIES,
  SYNC_X_COPIES,
  LAUNCH_Y_COPIES,
  SYNC_Y_COPIES,
  LAUNCH_Z_COPIES,
  SYNC_Z_COPIES,
  PRE_COMPUTE_BARRIER,
  PROBE_COMPUTE,
  POST_COMPUTE_BARRIER,
  NUM_TIMERS
};
static timer timers[NUM_TIMERS];
static string timerStrings[] = {"launch copies", "sync copies",
                                "copy barrier", "launch x copies",
                                "sync x copies", "launch y copies",
                                "sync y copies", "launch z copies",
                                "sync z copies", "pre compute barrier",
                                "probe compute", "post compute barrier"};
static timer_type timerTypes[] = {COMMUNICATION, COMMUNICATION,
                                  COMMUNICATION, COMMUNICATION,
                                  COMMUNICATION, COMMUNICATION,
                                  COMMUNICATION, COMMUNICATION,
                                  COMMUNICATION, COMMUNICATION,
                                  COMPUTATION, COMPUTATION};

static point<3> threadToPos(point<3> parts, int threads, int i) {
  int xpos = i / (parts[2] * parts[3]);
  int ypos = (i % (parts[2] * parts[3])) / parts[3];
  int zpos = i % parts[3];
  return POINT(xpos, ypos, zpos);
}

static int posToThread(point<3> parts, int threads, point<3> pos) {
  if (pos[1] < 0 || pos[1] >= parts[1] ||
      pos[2] < 0 || pos[2] >= parts[2] ||
      pos[3] < 0 || pos[3] >= parts[3]) {
    return -1;
  } else {
    return pos[1] * parts[2] * parts[3] + pos[2] * parts[3] + pos[3];
  }
}

// Compute grid domains for each thread.
static ndarray<rectdomain<3>, 1> computeDomains(point<3> dims, point<3> parts,
                                                int threads) {
  ndarray<rectdomain<3>, 1> domains = ARRAY(rectdomain<3>, ((0), (threads)));
  for (int i = 0; i < threads; i++) {
    point<3> pos = threadToPos(parts, threads, i);
    int xstart, xend, ystart, yend, zstart, zend;
    int num, rem;
    num = dims[1] / parts[1];
    rem = dims[1] % parts[1];
    xstart = num * pos[1] + (pos[1] <= rem ?  pos[1] : rem);
    xend = xstart + num + (pos[1] < rem ? 1 : 0);
    num = dims[2] / parts[2];
    rem = dims[2] % parts[2];
    ystart = num * pos[2] + (pos[2] <= rem ?  pos[2] : rem);
    yend = ystart + num + (pos[2] < rem ? 1 : 0);
    num = dims[3] / parts[3];
    rem = dims[3] % parts[3];
    zstart = num * pos[3] + (pos[3] <= rem ?  pos[3] : rem);
    zend = zstart + num + (pos[3] < rem ? 1 : 0);
    domains[i] = RECTDOMAIN((xstart, ystart, zstart), (xend, yend, zend));
  }
  return domains;
}

static ndarray<int, 1> computeNeighbors(point<3> parts, int threads,
                                        int mythread) {
  point<3> mypos = threadToPos(parts, threads, mythread);
  ndarray<int, 1> neighbors = ARRAY(int, ((0), (6)));
  neighbors[0] = posToThread(parts, threads, mypos -POINT(1,0,0));
  neighbors[1] = posToThread(parts, threads, mypos +POINT(1,0,0));
  neighbors[2] = posToThread(parts, threads, mypos -POINT(0,1,0));
  neighbors[3] = posToThread(parts, threads, mypos +POINT(0,1,0));
  neighbors[4] = posToThread(parts, threads, mypos -POINT(0,0,1));
  neighbors[5] = posToThread(parts, threads, mypos +POINT(0,0,1));
  return neighbors;
}

static void initGrid(ndarray<double, 3> grid) {
#if DEBUG
  cout << MYTHREAD << ": initializing grid with domain "
       << grid.domain() << endl;
#endif
#ifdef RANDOM_VALUES
  foreach (p, grid.domain()) {
    grid[p] = Math.random();
  }
#else
  grid.set(CONSTANT_VALUE);
#endif
}

// Perform stencil.
static void probe(int steps) {
  double fac = allGridsA[0][POINT(0, 0, 0)]; // prevent constant folding
  for (int i = 0; i < steps; i++) {
    // Copy ghost zones from previous timestep.
    // first x dimension
    TIMER_START(timers[LAUNCH_X_COPIES]);
    if (targetsA[0] != NULL) {
      targetsA[0].copy_nbi(sourcesA[0]);
    }
    if (targetsA[1] != NULL) {
      targetsA[1].copy_nbi(sourcesA[1]);
    }
    TIMER_STOP(timers[LAUNCH_X_COPIES]);
#ifdef SYNC_BETWEEN_DIM
    TIMER_START(timers[SYNC_X_COPIES]);
    // Handle.syncNBI();
    async_copy_fence();
    barrier();
    TIMER_STOP(timers[SYNC_X_COPIES]);
#endif
    // now y dimension
    TIMER_START(timers[LAUNCH_Y_COPIES]);
    if (targetsA[2] != NULL) {
      targetsA[2].copy_nbi(sourcesA[2]);
    }
    if (targetsA[3] != NULL) {
      targetsA[3].copy_nbi(sourcesA[3]);
    }
    TIMER_STOP(timers[LAUNCH_Y_COPIES]);
#ifdef SYNC_BETWEEN_DIM
    TIMER_START(timers[SYNC_Y_COPIES]);
    // Handle.syncNBI();
    async_copy_fence();
    barrier();
    TIMER_STOP(timers[SYNC_Y_COPIES]);
#endif
    // finally z dimension
    TIMER_START(timers[LAUNCH_Z_COPIES]);
    if (targetsA[4] != NULL) {
      targetsA[4].copy_nbi(sourcesA[4]);
    }
    if (targetsA[5] != NULL) {
      targetsA[5].copy_nbi(sourcesA[5]);
    }
    TIMER_STOP(timers[LAUNCH_Z_COPIES]);
    TIMER_START(timers[SYNC_Z_COPIES]);
    // Handle.syncNBI();
    async_copy_fence();
    barrier(); // wait for puts from all nodes
    TIMER_STOP(timers[SYNC_Z_COPIES]);

    TIMER_START(timers[PROBE_COMPUTE]);
#ifdef OPT_LOOP
    foreachd (i, myDomain, 1) {
      ndarray<double, 2, unstrided> myGridBi = myGridB.slice(i);
      ndarray<double, 2, unstrided> myGridAi = myGridA.slice(i);
      ndarray<double, 2, unstrided> myGridAim = myGridA.slice(i-1);
      ndarray<double, 2, unstrided> myGridAip = myGridA.slice(i+1);
      foreachd (j, myDomain, 2) {
        ndarray<double, 1, simple> myGridBij = myGridBi.slice(j);
        ndarray<double, 1, simple> myGridAij = myGridAi.slice(j);
        ndarray<double, 1, simple> myGridAijm = myGridAi.slice(j-1);
        ndarray<double, 1, simple> myGridAijp = myGridAi.slice(j+1);
        ndarray<double, 1, simple> myGridAimj = myGridAim.slice(j);
        ndarray<double, 1, simple> myGridAipj = myGridAip.slice(j);
        foreachd (k, myDomain, 3) {
          myGridBij[k] =
            myGridAij[k+1] +
            myGridAij[k-1] +
            myGridAijp[k] +
            myGridAijm[k] +
            myGridAipj[k] +
            myGridAimj[k] -
            WEIGHT * myGridAij[k] / (fac * fac);
        }
      }
    }
#elif defined(SPLIT_LOOP)
    foreach3 (i, j, k, myDomain) {
      myGridB[i][j][k] =
        myGridA[i][j][k+1] +
        myGridA[i][j][k-1] +
        myGridA[i][j+1][k] +
        myGridA[i][j-1][k] +
        myGridA[i+1][j][k] +
        myGridA[i-1][j][k] -
        WEIGHT * myGridA[i][j][k] / (fac * fac);
    }
#else
    foreach (p, myDomain) {
# ifdef SECOND_ORDER
      myGridB[p] =
        myGridA[p + POINT( 0,  0,  2)] +
        myGridA[p + POINT( 0,  0,  1)] +
        myGridA[p + POINT( 0,  0, -1)] +
        myGridA[p + POINT( 0,  0, -2)] +
        myGridA[p + POINT( 0,  2,  0)] +
        myGridA[p + POINT( 0,  1,  0)] +
        myGridA[p + POINT( 0, -1,  0)] +
        myGridA[p + POINT( 0, -2,  0)] +
        myGridA[p + POINT( 2,  0,  0)] +
        myGridA[p + POINT( 1,  0,  0)] +
        myGridA[p + POINT(-1,  0,  0)] +
        myGridA[p + POINT(-2,  0,  0)] -
        WEIGHT * myGridA[p] / (fac * fac);
# else
      myGridB[p] =
        myGridA[p + POINT( 0,  0,  1)] +
        myGridA[p + POINT( 0,  0, -1)] +
        myGridA[p + POINT( 0,  1,  0)] +
        myGridA[p + POINT( 0, -1,  0)] +
        myGridA[p + POINT( 1,  0,  0)] +
        myGridA[p + POINT(-1,  0,  0)] -
        WEIGHT * myGridA[p] / (fac * fac);
# endif
    }
#endif
    TIMER_STOP(timers[PROBE_COMPUTE]);
    TIMER_START(timers[POST_COMPUTE_BARRIER]);
    barrier(); // wait for computation to finish
    TIMER_STOP(timers[POST_COMPUTE_BARRIER]);
    // Swap pointers
    SWAP(myGridA, myGridB, ndarray<double COMMA 3 UNSTRIDED>);
    SWAP(targetsA, targetsB, ndarray<global_ndarray<double COMMA 3> COMMA 1>);
    SWAP(sourcesA, sourcesB, ndarray<ndarray<double COMMA 3> COMMA 1>);
  }
}

#ifdef TIMERS_ENABLED
# define report(d, s) do { \
    if (MYTHREAD == 0)      \
      cout << s;            \
    report_value(d);        \
  } while (0)

// Report min, average, max of given value.
static void report_value(double d) {
  double ave = reduce::add(d, 0) / THREADS;
  double max = reduce::max(d, 0);
  double min = reduce::min(d, 0);
  if (MYTHREAD == 0) {
    cout << ": " << min << ", " << ave << ", " << max << endl;
  }
}
#else
# define report(d, s)
#endif

static void printTimingStats() {
  for (int i = 0; i < NUM_TIMERS; i++) {
    report(timers[i].secs(), "Time for " << timerStrings[i] << " (s)");
  }
}

int main(int argc, char **args) {
#ifdef MEMORY_DISTRIBUTED
  init(&argc, &args);
#endif
  if (argc > 1 && (argc != 8 || !strncmp(args[1], "-h", 2))) {
    cout << "Usage: stencil <xdim> <ydim> <zdim> <xparts> <yparts> <zparts> <timesteps>" << endl;
    exit(1);
  } else if (argc > 1) {
    xdim = atoi(args[1]);
    ydim = atoi(args[2]);
    zdim = atoi(args[3]);
    xparts = atoi(args[4]);
    yparts = atoi(args[5]);
    zparts = atoi(args[6]);
    steps = atoi(args[7]);
    assert(THREADS == xparts * yparts * zparts);
  } else {
    xdim = ydim = zdim = 64;
    xparts = THREADS;
    yparts = zparts = 1;
    steps = 8;
  }

#if DEBUG
  if (MYTHREAD == 0) {
    for (int i = 0; i < THREADS; i++) {
      point<3> pos = threadToPos(POINT(xparts,yparts,zparts), THREADS, i);
      cout << i << " -> " << pos << ", " << pos << " -> "
           << posToThread(POINT(xparts,yparts,zparts), THREADS, pos) << endl;
      ndarray<int, 1> nb = computeNeighbors(POINT(xparts,yparts,zparts), THREADS, i);
      for (int j = 0; j < nb.size(); j++) {
        cout << "  " << nb[j];
      }
      cout << endl;
    }
  }
#endif

  allDomains = computeDomains(POINT(xdim,ydim,zdim),
                              POINT(xparts,yparts,zparts),
                              THREADS);
  myDomain = allDomains[MYTHREAD];
#if DEBUG
  cout << MYTHREAD << ": thread domain is " << myDomain << endl;
#endif

  myGridA = ndarray<double, 3 UNSTRIDED>(myDomain.accrete(GHOST_WIDTH));
  myGridB = ndarray<double, 3 UNSTRIDED>(myDomain.accrete(GHOST_WIDTH));
  allGridsA = ARRAY(global_ndarray<double COMMA 3 UNSTRIDED>, ((0), ((int)THREADS)));
  allGridsB = ARRAY(global_ndarray<double COMMA 3 UNSTRIDED>, ((0), ((int)THREADS)));
  allGridsA.exchange(myGridA);
  allGridsB.exchange(myGridB);

  // Compute ordered ghost zone overlaps, x -> y -> z.
  ndarray<int, 1> nb = computeNeighbors(POINT(xparts,yparts,zparts),
                                        THREADS, MYTHREAD);
  rectdomain<3> targetDomain = myDomain;
  targetsA = ARRAY(global_ndarray<double COMMA 3>, ((0), (6)));
  targetsB = ARRAY(global_ndarray<double COMMA 3>, ((0), (6)));
  sourcesA = ARRAY(ndarray<double COMMA 3>, ((0), (6)));
  sourcesB = ARRAY(ndarray<double COMMA 3>, ((0), (6)));
  for (int i = 0; i < 6; i++) {
    if (nb[i] != -1) {
#if DEBUG
      cout << MYTHREAD << ": overlap " << i << " = "
           << (allGridsA[nb[i]].domain() * targetDomain) << endl;
#endif
      targetsA[i] = allGridsA[nb[i]].constrict(targetDomain);
      targetsB[i] = allGridsB[nb[i]].constrict(targetDomain);
      sourcesA[i] = myGridA;
      sourcesB[i] = myGridB;
#ifdef SYNC_BETWEEN_DIM
      if (i % 2 == 1) {
        targetDomain = targetDomain.accrete(i / 2, -GHOST_WIDTH);
        targetDomain = targetDomain.accrete(i / 2, +GHOST_WIDTH);
      }
#endif
    }
  }

#ifdef TIMERS_ENABLED
  timer t;
#endif
  for (int i = 0; i < NUM_TRIALS; i++) {
    initGrid(myGridA);
    initGrid(myGridB);
    TIMER_RESET(t);
    barrier(); // wait for all threads before starting
    TIMER_START(t);
    probe(steps);
    TIMER_STOP(t);

    double val = myGridA[myDomain.min()];
    report(t.secs(), "Time for trial " << i << " with split " << xparts << 
           "x" << yparts << "x" << zparts << " (s)");
    if (MYTHREAD == 0) {
      cout << "Verification value: " << val << endl;
    }
#ifdef TIMERS_ENABLED
    printTimingStats();
    double comm = 0, comp = 0;
    for (int j = 0; j < NUM_TIMERS; j++) {
      if (timerTypes[j] == COMMUNICATION) {
        comm += timers[j].secs();
      } else {
        comp += timers[j].secs();
      }
      timers[j].reset();
    }
    report(comm, "Time for total communication");
    report(comp, "Time for total computation");
#endif
  }

  finalize(); // wait for completion before exiting
}
