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
    if (targetsA[0] != NULL) {
      targetsA[0].copy_nbi(sourcesA[0]);
    }
    if (targetsA[1] != NULL) {
      targetsA[1].copy_nbi(sourcesA[1]);
    }
#ifdef SYNC_BETWEEN_DIM
    // Handle.syncNBI();
    async_copy_fence();
    barrier();
#endif
    // now y dimension
    if (targetsA[2] != NULL) {
      targetsA[2].copy_nbi(sourcesA[2]);
    }
    if (targetsA[3] != NULL) {
      targetsA[3].copy_nbi(sourcesA[3]);
    }
#ifdef SYNC_BETWEEN_DIM
    // Handle.syncNBI();
    async_copy_fence();
    barrier();
#endif
    // finally z dimension
    if (targetsA[4] != NULL) {
      targetsA[4].copy_nbi(sourcesA[4]);
    }
    if (targetsA[5] != NULL) {
      targetsA[5].copy_nbi(sourcesA[5]);
    }
    // Handle.syncNBI();
    async_copy_fence();
    barrier(); // wait for puts from all nodes

#ifdef OPT_LOOP
    foreachD (i, myDomain, 1) {
      ndarray<double, 2, unstrided> myGridBi = (ndarray<double, 2, unstrided>) myGridB.slice(1, i);
      ndarray<double, 2, unstrided> myGridAi = (ndarray<double, 2, unstrided>) myGridA.slice(1, i);
      ndarray<double, 2, unstrided> myGridAim = (ndarray<double, 2, unstrided>) myGridA.slice(1, i-1);
      ndarray<double, 2, unstrided> myGridAip = (ndarray<double, 2, unstrided>) myGridA.slice(1, i+1);
      foreachD (j, myDomain, 2) {
        ndarray<double, 1, simple> myGridBij = (ndarray<double, 1, simple>) myGridBi.slice(1, j);
        ndarray<double, 1, simple> myGridAij = (ndarray<double, 1, simple>) myGridAi.slice(1, j);
        ndarray<double, 1, simple> myGridAijm = (ndarray<double, 1, simple>) myGridAi.slice(1, j-1);
        ndarray<double, 1, simple> myGridAijp = (ndarray<double, 1, simple>) myGridAi.slice(1, j+1);
        ndarray<double, 1, simple> myGridAimj = (ndarray<double, 1, simple>) myGridAim.slice(1, j);
        ndarray<double, 1, simple> myGridAipj = (ndarray<double, 1, simple>) myGridAip.slice(1, j);
        foreachD (k, myDomain, 3) {
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
    barrier(); // wait for computation to finish
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
  }

  finalize(); // wait for completion before exiting
}
