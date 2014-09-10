// Stencil
#if USE_MPI
# define USE_UPCXX 0
# define USE_ARRAYS
# define MPI_STYLE
#endif
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
#ifdef MPI_STYLE
int nx, ny, nz;
int nx_interior, ny_interior, nz_interior;
int neighbors[6];
int planeSizes[6];
point<3> packStarts[6], packEnds[6];
point<3> unpackStarts[6], unpackEnds[6];
struct buffer_list {
  double *buffers[6];
};
buffer_list inBufs, outBufs, targetBufs;
ndarray<buffer_list, 1> allInBufs;
int s2r[] = { 1, 0, 3, 2, 5, 4 };
# if USE_MPI
MPI_Request requests[12];
# endif
#endif
int steps;
int numTrials = 1;

enum timer_type {
  COMMUNICATION,
  COMPUTATION
};
enum timer_index {
#if USE_MPI
  POST_RECEIVES = 0,
  PACK,
  POST_SENDS,
  WAITALL,
  UNPACK,
#elif defined(MPI_STYLE)
  PACK = 0,
  LAUNCH_PUTS,
  SYNC_PUTS,
  PUT_BARRIER,
  UNPACK,
#else
  LAUNCH_X_COPIES = 0,
  SYNC_X_COPIES,
  LAUNCH_Y_COPIES,
  SYNC_Y_COPIES,
  LAUNCH_Z_COPIES,
  SYNC_Z_COPIES,
#endif
  PROBE_COMPUTE,
  POST_COMPUTE_BARRIER,
  NUM_TIMERS
};
timer timers[NUM_TIMERS];
string timerStrings[] = {
#if USE_MPI
  "post receives", "pack", "post sends", "sync messages", "unpack",
#elif defined(MPI_STYLE)
  "pack", "launch puts", "sync puts", "put barrier", "unpack",
#else
  "launch x copies", "sync x copies", "launch y copies",
  "sync y copies", "launch z copies", "sync z copies",
#endif
  "probe compute", "post compute barrier"
};
timer_type timerTypes[] = {
  COMMUNICATION, COMMUNICATION, COMMUNICATION, COMMUNICATION,
#ifndef MPI_STYLE
  COMMUNICATION,
#endif
  COMMUNICATION, COMPUTATION, COMPUTATION
};

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
  };
#else
  grid.set(CONSTANT_VALUE);
#endif
}

#ifdef MPI_STYLE
void setupComm(point<3> parts, int threads, int mythread) {
  point<3> mypos = threadToPos(parts, threads, mythread);
  neighbors[0] = posToThread(parts, threads, mypos - PT(1,0,0));
  neighbors[1] = posToThread(parts, threads, mypos + PT(1,0,0));
  neighbors[2] = posToThread(parts, threads, mypos - PT(0,1,0));
  neighbors[3] = posToThread(parts, threads, mypos + PT(0,1,0));
  neighbors[4] = posToThread(parts, threads, mypos - PT(0,0,1));
  neighbors[5] = posToThread(parts, threads, mypos + PT(0,0,1));
  planeSizes[0] = ny_interior * nz_interior;
  planeSizes[1] = ny_interior * nz_interior;
  planeSizes[2] = nx_interior * nz_interior;
  planeSizes[3] = nx_interior * nz_interior;
  planeSizes[4] = nx_interior * ny_interior;
  planeSizes[5] = nx_interior * ny_interior;
  packStarts[0] = PT(GHOST_WIDTH, GHOST_WIDTH, GHOST_WIDTH);
  packStarts[1] = PT(nx-2*GHOST_WIDTH, GHOST_WIDTH, GHOST_WIDTH);
  packStarts[2] = PT(GHOST_WIDTH, GHOST_WIDTH, GHOST_WIDTH);
  packStarts[3] = PT(GHOST_WIDTH, ny-2*GHOST_WIDTH, GHOST_WIDTH);
  packStarts[4] = PT(GHOST_WIDTH, GHOST_WIDTH, GHOST_WIDTH);
  packStarts[5] = PT(GHOST_WIDTH, GHOST_WIDTH, nz-2*GHOST_WIDTH);
  packEnds[0] = PT(2*GHOST_WIDTH, ny-GHOST_WIDTH, nz-GHOST_WIDTH);
  packEnds[1] = PT(nx-GHOST_WIDTH, ny-GHOST_WIDTH, nz-GHOST_WIDTH);
  packEnds[2] = PT(nx-GHOST_WIDTH, 2*GHOST_WIDTH, nz-GHOST_WIDTH);
  packEnds[3] = PT(nx-GHOST_WIDTH, ny-GHOST_WIDTH, nz-GHOST_WIDTH);
  packEnds[4] = PT(nx-GHOST_WIDTH, ny-GHOST_WIDTH, 2*GHOST_WIDTH);
  packEnds[5] = PT(nx-GHOST_WIDTH, ny-GHOST_WIDTH, nz-GHOST_WIDTH);
  unpackStarts[0] = PT(0, GHOST_WIDTH, GHOST_WIDTH);
  unpackStarts[1] = PT(nx-GHOST_WIDTH, GHOST_WIDTH, GHOST_WIDTH);
  unpackStarts[2] = PT(GHOST_WIDTH, 0, GHOST_WIDTH);
  unpackStarts[3] = PT(GHOST_WIDTH, ny-GHOST_WIDTH, GHOST_WIDTH);
  unpackStarts[4] = PT(GHOST_WIDTH, GHOST_WIDTH, 0);
  unpackStarts[5] = PT(GHOST_WIDTH, GHOST_WIDTH, nz-GHOST_WIDTH);
  unpackEnds[0] = PT(GHOST_WIDTH, ny-GHOST_WIDTH, nz-GHOST_WIDTH);
  unpackEnds[1] = PT(nx, ny-GHOST_WIDTH, nz-GHOST_WIDTH);
  unpackEnds[2] = PT(nx-GHOST_WIDTH, GHOST_WIDTH, nz-GHOST_WIDTH);
  unpackEnds[3] = PT(nx-GHOST_WIDTH, ny, nz-GHOST_WIDTH);
  unpackEnds[4] = PT(nx-GHOST_WIDTH, ny-GHOST_WIDTH, GHOST_WIDTH);
  unpackEnds[5] = PT(nx-GHOST_WIDTH, ny-GHOST_WIDTH, nz);
}

#define PackIndex3D(i,j,k)                                      \
  ((LAST_DIM(i, j, k))+                                         \
   (LAST_DIM(nx, ny, nz))*((j) + (ny)*(FIRST_DIM(i, j, k))))

void pack(double *grid, double **bufs) {
  for (int i = 0; i < 6; i++) {
    if (neighbors[i] != -1) {
      double *buf = bufs[i];
      int bidx = 0;
      for (int x = packStarts[i][1]; x < packEnds[i][1]; x++) {
        for (int y = packStarts[i][2]; y < packEnds[i][2]; y++) {
          for (int z = packStarts[i][3]; z < packEnds[i][3]; z++) {
            buf[bidx++] = grid[PackIndex3D(x, y, z)];
          }
        }
      }
    }
  }
}

void unpack(double *grid, double **bufs) {
  for (int i = 0; i < 6; i++) {
    if (neighbors[i] != -1) {
      double *buf = bufs[s2r[i]];
      int bidx = 0;
      for (int x = unpackStarts[i][1]; x < unpackEnds[i][1]; x++) {
        for (int y = unpackStarts[i][2]; y < unpackEnds[i][2]; y++) {
          for (int z = unpackStarts[i][3]; z < unpackEnds[i][3]; z++) {
            grid[PackIndex3D(x, y, z)] = buf[bidx++];
          }
        }
      }
    }
  }
}
#endif

// Perform stencil.
void probe(int steps) {
  for (int i = 0; i < steps; i++) {
    // Copy ghost zones from previous timestep.
#ifdef MPI_STYLE
# if USE_MPI
    timers[POST_RECEIVES].start();
    int messageNum = 0;
    for (int j = 0; j < 6; j++) {
      if (neighbors[j] != -1) {
        MPI_Irecv(inBufs.buffers[s2r[j]], planeSizes[j] * GHOST_WIDTH,
                  MPI_DOUBLE, neighbors[j], s2r[j],
                  MPI_COMM_WORLD, &requests[messageNum++]);
      }
    }
    timers[POST_RECEIVES].stop();
# endif
    timers[PACK].start();
    pack(myGridA.base_ptr(), outBufs.buffers);
    timers[PACK].stop();
# if USE_MPI
    timers[POST_SENDS].start();
    for (int j = 0; j < 6; j++) {
      if (neighbors[j] != -1) {
        MPI_Isend(outBufs.buffers[j], planeSizes[j] * GHOST_WIDTH,
                  MPI_DOUBLE, neighbors[j], j, MPI_COMM_WORLD,
                  &requests[messageNum++]);
      }
    }
    timers[POST_SENDS].stop();
    timers[WAITALL].start();
    MPI_Waitall(messageNum, requests, MPI_STATUSES_IGNORE);
    timers[WAITALL].stop();
# else
    timers[LAUNCH_PUTS].start();
    for (int j = 0; j < 6; j++) {
      if (neighbors[j] != -1) {
        gasnet_put_nbi_bulk(neighbors[j], targetBufs.buffers[j],
                            outBufs.buffers[j], planeSizes[j] *
                            GHOST_WIDTH * sizeof(double));
      }
    }
    timers[LAUNCH_PUTS].stop();
    timers[SYNC_PUTS].start();
    async_wait();
    timers[SYNC_PUTS].stop();
    timers[PUT_BARRIER].start();
    barrier();
    timers[PUT_BARRIER].stop();
# endif
    timers[UNPACK].start();
    unpack(myGridA.base_ptr(), inBufs.buffers);
    timers[UNPACK].stop();
#else // MPI_STYLE
    // first x dimension
    TIMER_START(timers[LAUNCH_X_COPIES]);
    if (targetsA[0] != NULL) {
      targetsA[0].async_copy(sourcesA[0]);
    }
    if (targetsA[1] != NULL) {
      targetsA[1].async_copy(sourcesA[1]);
    }
    TIMER_STOP(timers[LAUNCH_X_COPIES]);
# ifdef SYNC_BETWEEN_DIM
    TIMER_START(timers[SYNC_X_COPIES]);
    // Handle.syncNBI();
    async_wait();
    barrier();
    TIMER_STOP(timers[SYNC_X_COPIES]);
# endif
    // now y dimension
    TIMER_START(timers[LAUNCH_Y_COPIES]);
    if (targetsA[2] != NULL) {
      targetsA[2].async_copy(sourcesA[2]);
    }
    if (targetsA[3] != NULL) {
      targetsA[3].async_copy(sourcesA[3]);
    }
    TIMER_STOP(timers[LAUNCH_Y_COPIES]);
# ifdef SYNC_BETWEEN_DIM
    TIMER_START(timers[SYNC_Y_COPIES]);
    // Handle.syncNBI();
    async_wait();
    barrier();
    TIMER_STOP(timers[SYNC_Y_COPIES]);
# endif
    // finally z dimension
    TIMER_START(timers[LAUNCH_Z_COPIES]);
    if (targetsA[4] != NULL) {
      targetsA[4].async_copy(sourcesA[4]);
    }
    if (targetsA[5] != NULL) {
      targetsA[5].async_copy(sourcesA[5]);
    }
    TIMER_STOP(timers[LAUNCH_Z_COPIES]);
    TIMER_START(timers[SYNC_Z_COPIES]);
    // Handle.syncNBI();
    async_wait();
    barrier(); // wait for puts from all nodes
    TIMER_STOP(timers[SYNC_Z_COPIES]);
#endif // MPI_STYLE

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
# ifdef UNPACK_SPEC
#  ifndef CONCAT_
#   define CONCAT_(a, b) CONCAT__(a, b)
#   define CONCAT__(a, b) a ## b
#  endif
#  define AINDEX3_SPEC CONCAT_(AINDEX3_, UNPACK_SPEC)
# else
#  define AINDEX3_SPEC AINDEX3
# endif
# if defined(USE_RESTRICT) && defined(__GNUC__)
    AINDEX3_SETUP_QUAL(myGridA, __restrict__);
    AINDEX3_SETUP_QUAL(myGridB, __restrict__);
# elif defined(USE_RESTRICT)
    AINDEX3_SETUP_QUAL(myGridA, __restrict);
    AINDEX3_SETUP_QUAL(myGridB, __restrict);
# else
    AINDEX3_SETUP(myGridA);
    AINDEX3_SETUP(myGridB);
# endif
    cforeach3(i, j, k, myDomain) {
      AINDEX3_SPEC(myGridB, i, j, k) =
	AINDEX3_SPEC(myGridA, i, j, k+1) +
	AINDEX3_SPEC(myGridA, i, j, k-1) +
	AINDEX3_SPEC(myGridA, i, j+1, k) +
	AINDEX3_SPEC(myGridA, i, j-1, k) +
	AINDEX3_SPEC(myGridA, i+1, j, k) +
	AINDEX3_SPEC(myGridA, i-1, j, k) +
	WEIGHT * AINDEX3_SPEC(myGridA, i, j, k);
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
    };
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
  init(&argc, &args);

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

#ifdef MPI_STYLE
  nx_interior = myDomain.upb()[1] - myDomain.lwb()[1];
  ny_interior = myDomain.upb()[2] - myDomain.lwb()[2];
  nz_interior = myDomain.upb()[3] - myDomain.lwb()[3];
  nx = nx_interior + 2 * GHOST_WIDTH;
  ny = ny_interior + 2 * GHOST_WIDTH;
  nz = nz_interior + 2 * GHOST_WIDTH;
  setupComm(PT(xparts,yparts,zparts), THREADS, MYTHREAD);

  for (int i = 0; i < 6; i++) {
    if (neighbors[i] != -1) {
      inBufs.buffers[s2r[i]] =
        (double *) allocate(planeSizes[i] * GHOST_WIDTH *
                            sizeof(double));
      outBufs.buffers[i] =
        (double *) allocate(planeSizes[i] * GHOST_WIDTH *
                            sizeof(double));
    }
# if DEBUG
    point<3> p = packEnds[i] - packStarts[i];
    point<3> q = unpackEnds[i] - unpackStarts[i];
    cout << MYTHREAD << ": Ghost size in dir " << i << ": "
         << (planeSizes[i] * GHOST_WIDTH) << ", "
         << (p[1] * p[2] * p[3]) << ", "
         << (q[1] * q[2] * q[3]) << endl;
# endif
  }

# if !USE_MPI
  allInBufs.create(RD(THREADS));
  allInBufs.exchange(inBufs);
  for (int i = 0; i < 6; i++) {
    if (neighbors[i] != -1) {
      targetBufs.buffers[i] =
        allInBufs[neighbors[i]].buffers[i];
#  if DEBUG
      cout << MYTHREAD << ": target " << i << ": "
           << targetBufs.buffers[i] << endl;
#  endif
    }
  }
# endif
#endif // MPI_STYLE

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
