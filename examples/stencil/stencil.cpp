// Stencil
#include "globals.h"

int xdim, ydim, zdim;
int xparts, yparts, zparts;
ndarray<rectdomain<3>, 1> allDomains;
rectdomain<3> myDomain;
ndarray<double, 3 UNSTRIDED> myGridA, myGridB;
#ifdef SHARED_DIR
shared_array<ndarray<double, 3, global> > allGridsA, allGridsB;
#else
ndarray<ndarray<double, 3, global>, 1> allGridsA, allGridsB;
#endif
ndarray<ndarray<double, 3, global>, 1> targetsA, targetsB;
ndarray<ndarray<double, 3>, 1> sourcesA, sourcesB;
int steps;
int numTrials = 1;

enum timer_type {
  COMMUNICATION,
  COMPUTATION
};
enum timer_index {
  LAUNCH_X_COPIES = 0,
  SYNC_X_COPIES,
  LAUNCH_Y_COPIES,
  SYNC_Y_COPIES,
  LAUNCH_Z_COPIES,
  SYNC_Z_COPIES,
  PROBE_COMPUTE,
  POST_COMPUTE_BARRIER,
  NUM_TIMERS
};
timer timers[NUM_TIMERS];
string timerStrings[] = {"launch x copies", "sync x copies",
                                "launch y copies", "sync y copies",
                                "launch z copies", "sync z copies",
                                "probe compute", "post compute barrier"};
timer_type timerTypes[] = {COMMUNICATION, COMMUNICATION,
                                  COMMUNICATION, COMMUNICATION,
                                  COMMUNICATION, COMMUNICATION,
                                  COMPUTATION, COMPUTATION};

point<3> threadToPos(point<3> parts, int threads, int i) {
  int xpos = i / (parts[2] * parts[3]);
  int ypos = (i % (parts[2] * parts[3])) / parts[3];
  int zpos = i % parts[3];
  return POINT(xpos, ypos, zpos);
}

int posToThread(point<3> parts, int threads, point<3> pos) {
  if (pos[1] < 0 || pos[1] >= parts[1] ||
      pos[2] < 0 || pos[2] >= parts[2] ||
      pos[3] < 0 || pos[3] >= parts[3]) {
    return -1;
  } else {
    return pos[1] * parts[2] * parts[3] + pos[2] * parts[3] + pos[3];
  }
}

// Compute grid domains for each thread.
ndarray<rectdomain<3>, 1> computeDomains(point<3> dims,
                                         point<3> parts,
                                         int threads) {
  ndarray<rectdomain<3>, 1> domains(RD(threads));
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
    domains[i] = RD(PT(xstart, ystart, zstart), PT(xend, yend, zend));
  }
  return domains;
}

ndarray<int, 1> computeNeighbors(point<3> parts, int threads,
                                 int mythread) {
  point<3> mypos = threadToPos(parts, threads, mythread);
  ndarray<int, 1> neighbors(RD(6));
  neighbors[0] = posToThread(parts, threads, mypos - PT(1,0,0));
  neighbors[1] = posToThread(parts, threads, mypos + PT(1,0,0));
  neighbors[2] = posToThread(parts, threads, mypos - PT(0,1,0));
  neighbors[3] = posToThread(parts, threads, mypos + PT(0,1,0));
  neighbors[4] = posToThread(parts, threads, mypos - PT(0,0,1));
  neighbors[5] = posToThread(parts, threads, mypos + PT(0,0,1));
  return neighbors;
}

void initGrid(ndarray<double, 3> grid) {
#if DEBUG
  cout << MYTHREAD << ": initializing grid with domain "
       << grid.domain() << endl;
#endif
#ifdef RANDOM_VALUES
  foreach (p, grid.domain()) {
    grid[p] = ((double) rand()) / RAND_MAX;
  }
#else
  grid.set(CONSTANT_VALUE);
#endif
}

// Perform stencil.
void probe(int steps) {
  double fac = myGridA[myGridA.domain().min()]; // prevent constant folding
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
      ndarray<double, 2 UNSTRIDED> myGridBi = myGridB.slice(i);
      ndarray<double, 2 UNSTRIDED> myGridAi = myGridA.slice(i);
      ndarray<double, 2 UNSTRIDED> myGridAim = myGridA.slice(i-1);
      ndarray<double, 2 UNSTRIDED> myGridAip = myGridA.slice(i+1);
      foreachd (j, myDomain, 2) {
        ndarray<double, 1 UNSTRIDED> myGridBij = myGridBi.slice(j);
        ndarray<double, 1 UNSTRIDED> myGridAij = myGridAi.slice(j);
        ndarray<double, 1 UNSTRIDED> myGridAijm = myGridAi.slice(j-1);
        ndarray<double, 1 UNSTRIDED> myGridAijp = myGridAi.slice(j+1);
        ndarray<double, 1 UNSTRIDED> myGridAimj = myGridAim.slice(j);
        ndarray<double, 1 UNSTRIDED> myGridAipj = myGridAip.slice(j);
        foreachd (k, myDomain, 3) {
          myGridBij[k] =
            myGridAij[k+1] +
            myGridAij[k-1] +
            myGridAijp[k] +
            myGridAijm[k] +
            myGridAipj[k] +
            myGridAimj[k] +
            WEIGHT * myGridAij[k];
        }
      }
    }
#elif defined(SPEC_LOOP)
    cforeach3 (i, j, k, myDomain) {
      myGridB[PT(i, j, k)] =
        myGridA[PT(i, j, k+1)] +
        myGridA[PT(i, j, k-1)] +
        myGridA[PT(i, j+1, k)] +
        myGridA[PT(i, j-1, k)] +
        myGridA[PT(i+1, j, k)] +
        myGridA[PT(i-1, j, k)] +
        WEIGHT * myGridA[PT(i, j, k)];
    }
#elif defined(SPLIT_LOOP)
    cforeach3 (i, j, k, myDomain) {
      myGridB[i][j][k] =
        myGridA[i][j][k+1] +
        myGridA[i][j][k-1] +
        myGridA[i][j+1][k] +
        myGridA[i][j-1][k] +
        myGridA[i+1][j][k] +
        myGridA[i-1][j][k] +
        WEIGHT * myGridA[i][j][k];
    }
#elif defined(VAR_LOOP)
    cforeach3 (i, j, k, myDomain) {
      myGridB(i, j, k) =
        myGridA(i, j, k+1) +
        myGridA(i, j, k-1) +
        myGridA(i, j+1, k) +
        myGridA(i, j-1, k) +
        myGridA(i+1, j, k) +
        myGridA(i-1, j, k) +
        WEIGHT * myGridA(i, j, k);
    }
#elif defined (UNPACKED_LOOP)
# if defined(USE_RESTRICT) && defined(__GNUC__)
    UNPACK_ARRAY3_QUAL(myGridA, __restrict__);
    UNPACK_ARRAY3_QUAL(myGridB, __restrict__);
# elif defined(USE_RESTRICT)
    UNPACK_ARRAY3_QUAL(myGridA, __restrict);
    UNPACK_ARRAY3_QUAL(myGridB, __restrict);
# else
    UNPACK_ARRAY3(myGridA);
    UNPACK_ARRAY3(myGridB);
# endif
    cforeach3(i, j, k, myDomain) {
      AINDEX3(myGridB, i, j, k) =
	AINDEX3(myGridA, i, j, k+1) +
	AINDEX3(myGridA, i, j, k-1) +
	AINDEX3(myGridA, i, j+1, k) +
	AINDEX3(myGridA, i, j-1, k) +
	AINDEX3(myGridA, i+1, j, k) +
	AINDEX3(myGridA, i-1, j, k) +
	WEIGHT * AINDEX3(myGridA, i, j, k);
    }
#elif defined(RAW_LOOP)
# define Index3D(i,j,k)                                                 \
    ((LAST_DIM(i, j, k)+GHOST_WIDTH)+                                   \
     (LAST_DIM(nx, ny, nz))*((j+GHOST_WIDTH)+                           \
                             (ny)*(FIRST_DIM(i, j, k)+GHOST_WIDTH)))
    int nx = myDomain.upb()[1] - myDomain.lwb()[1] + 2*GHOST_WIDTH;
    int ny = myDomain.upb()[2] - myDomain.lwb()[2] + 2*GHOST_WIDTH;
    int nz = myDomain.upb()[3] - myDomain.lwb()[3] + 2*GHOST_WIDTH;
    double *ptrA = myGridA.base_ptr();
    double *ptrB = myGridB.base_ptr();
    cforeach3 (i, j, k, myDomain) {
      ptrB[Index3D(i, j, k)] =
        ptrA[Index3D(i, j, k+1)] +
        ptrA[Index3D(i, j, k-1)] +
        ptrA[Index3D(i, j+1, k)] +
        ptrA[Index3D(i, j-1, k)] +
        ptrA[Index3D(i+1, j, k)] +
        ptrA[Index3D(i-1, j, k)] +
	WEIGHT * ptrA[Index3D(i, j, k)];
    }
#elif defined(RAW_FOR_LOOP)
# define Index3D(i,j,k)                                         \
    ((LAST_DIM(i, j, k))+                                       \
     (LAST_DIM(nx, ny, nz))*((j)+                               \
                             (ny)*(FIRST_DIM(i, j, k))))
    int nx = myDomain.upb()[1] - myDomain.lwb()[1] + 2*GHOST_WIDTH;
    int ny = myDomain.upb()[2] - myDomain.lwb()[2] + 2*GHOST_WIDTH;
    int nz = myDomain.upb()[3] - myDomain.lwb()[3] + 2*GHOST_WIDTH;
    double *ptrA = myGridA.base_ptr();
    double *ptrB = myGridB.base_ptr();
    for (int FIRST_DIM(i, j, k) = GHOST_WIDTH;
         FIRST_DIM(i, j, k) < FIRST_DIM(nx, ny, nz)-GHOST_WIDTH;
         FIRST_DIM(i, j, k)++) {
      for (int j = GHOST_WIDTH; j < ny-GHOST_WIDTH; j++) {
        for (int LAST_DIM(i, j, k) = GHOST_WIDTH;
             LAST_DIM(i, j, k) < LAST_DIM(nx, ny, nz)-GHOST_WIDTH;
             LAST_DIM(i, j, k)++) {
          ptrB[Index3D(i, j, k)] =
            ptrA[Index3D(i, j, k+1)] +
            ptrA[Index3D(i, j, k-1)] +
            ptrA[Index3D(i, j+1, k)] +
            ptrA[Index3D(i, j-1, k)] +
            ptrA[Index3D(i+1, j, k)] +
            ptrA[Index3D(i-1, j, k)] +
            WEIGHT * ptrA[Index3D(i, j, k)];
        }
      }
    }
#elif defined(OMP_SPLIT_LOOP) && defined(USE_FOREACHH)
    foreachh (3, myDomain, lwb, upb, stride, done) {
      foreachhd (i, 0, lwb, upb, stride) {
# pragma omp parallel for
        foreachhd (j, 1, lwb, upb, stride) {
# pragma ivdep
          foreachhd (k, 2, lwb, upb, stride) {
            myGridB[i][j][k] =
              myGridA[i][j][k+1] +
              myGridA[i][j][k-1] +
              myGridA[i][j+1][k] +
              myGridA[i][j-1][k] +
              myGridA[i+1][j][k] +
              myGridA[i-1][j][k] +
              WEIGHT * myGridA[i][j][k];
          }
        }
      }
    }
#elif defined(OMP_SPLIT_LOOP)
    foreachd (i, myDomain, 1) {
      foreachd_pragma (omp parallel for, j, myDomain, 2) {
	foreachd_pragma (ivdep, k, myDomain, 3) {
	  myGridB[i][j][k] =
	    myGridA[i][j][k+1] +
	    myGridA[i][j][k-1] +
	    myGridA[i][j+1][k] +
	    myGridA[i][j-1][k] +
	    myGridA[i+1][j][k] +
	    myGridA[i-1][j][k] +
	    WEIGHT * myGridA[i][j][k];
	}
      }
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
        myGridA[p + POINT(-2,  0,  0)] +
        WEIGHT * myGridA[p];
# else
      myGridB[p] =
        myGridA[p + POINT( 0,  0,  1)] +
        myGridA[p + POINT( 0,  0, -1)] +
        myGridA[p + POINT( 0,  1,  0)] +
        myGridA[p + POINT( 0, -1,  0)] +
        myGridA[p + POINT( 1,  0,  0)] +
        myGridA[p + POINT(-1,  0,  0)] +
        WEIGHT * myGridA[p];
# endif
    }
#endif
    TIMER_STOP(timers[PROBE_COMPUTE]);
    TIMER_START(timers[POST_COMPUTE_BARRIER]);
    barrier(); // wait for computation to finish
    TIMER_STOP(timers[POST_COMPUTE_BARRIER]);
    // Swap pointers
    SWAP(myGridA, myGridB);
    SWAP(targetsA, targetsB);
    SWAP(sourcesA, sourcesB);
  }
}

#ifdef TIMERS_ENABLED
# define report(d, s) do { \
    if (MYTHREAD == 0)      \
      cout << s;            \
    report_value(d);        \
  } while (0)

// Report min, average, max of given value.
void report_value(double d) {
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

void printTimingStats() {
  for (int i = 0; i < NUM_TIMERS; i++) {
    report(timers[i].secs(), "Time for " << timerStrings[i] << " (s)");
  }
}

int main(int argc, char **args) {
#ifdef MEMORY_DISTRIBUTED
  init(&argc, &args);
#endif
  if (argc > 1 && (argc < 8 || !strncmp(args[1], "-h", 2))) {
    cout << "Usage: stencil <xdim> <ydim> <zdim> "
         << "<xparts> <yparts> <zparts> <timesteps> "
         << "[num_trials]" << endl;
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
    if (argc > 8)
      numTrials = atoi(args[8]);
  } else {
    xdim = ydim = zdim = 64;
    xparts = THREADS;
    yparts = zparts = 1;
    steps = 8;
  }

#if DEBUG
  if (MYTHREAD == 0) {
    for (int i = 0; i < THREADS; i++) {
      point<3> pos =
        threadToPos(POINT(xparts,yparts,zparts), THREADS, i);
      cout << i << " -> " << pos << ", " << pos << " -> "
           << posToThread(POINT(xparts,yparts,zparts), THREADS, pos)
           << endl;
      ndarray<int, 1> nb =
        computeNeighbors(POINT(xparts,yparts,zparts), THREADS, i);
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

  myGridA =
    ndarray<double, 3 UNSTRIDED>(myDomain.accrete(GHOST_WIDTH) CMAJOR);
  myGridB =
    ndarray<double, 3 UNSTRIDED>(myDomain.accrete(GHOST_WIDTH) CMAJOR);
#ifdef SHARED_DIR
  allGridsA.init(THREADS);
  allGridsB.init(THREADS);
  allGridsA[MYTHREAD] = myGridA;
  allGridsB[MYTHREAD] = myGridB;
#else
  allGridsA =
    ndarray<ndarray<double, 3, global>, 1>(RD((int)THREADS));
  allGridsB =
    ndarray<ndarray<double, 3, global>, 1>(RD((int)THREADS));
  allGridsA.exchange(myGridA);
  allGridsB.exchange(myGridB);
#endif

  // Compute ordered ghost zone overlaps, x -> y -> z.
  ndarray<int, 1> nb = computeNeighbors(POINT(xparts,yparts,zparts),
                                        (int)THREADS, (int)MYTHREAD);
  rectdomain<3> targetDomain = myDomain;
  targetsA = ndarray<ndarray<double, 3, global>, 1>(RD(6));
  targetsB = ndarray<ndarray<double, 3, global>, 1>(RD(6));
  sourcesA = ndarray<ndarray<double, 3>, 1>(RD(6));
  sourcesB = ndarray<ndarray<double, 3>, 1>(RD(6));
  for (int i = 0; i < 6; i++) {
    if (nb[i] != -1) {
#if DEBUG
      cout << MYTHREAD << ": overlap " << i << " = "
           << (((ndarray<double, 3, global>)
                allGridsA[nb[i]]).domain() * targetDomain) << endl;
#endif
#ifdef SHARED_DIR
      targetsA[i] = allGridsA[nb[i]].get().constrict(targetDomain);
      targetsB[i] = allGridsB[nb[i]].get().constrict(targetDomain);
#else
      targetsA[i] = allGridsA[nb[i]].constrict(targetDomain);
      targetsB[i] = allGridsB[nb[i]].constrict(targetDomain);
#endif
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
  for (int i = 0; i < numTrials; i++) {
    initGrid(myGridA);
    initGrid(myGridB);
    TIMER_RESET(t);
    barrier(); // wait for all threads before starting
    TIMER_START(t);
    probe(steps);
    TIMER_STOP(t);

    double val = myGridA[myDomain.min()];
    report(t.secs(), "Time for trial " << i << " with split " <<
           xparts << "x" << yparts << "x" << zparts << " (s)");
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
