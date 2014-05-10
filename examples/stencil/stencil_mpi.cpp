// MPI version of stencil code
#include <assert.h>
#include <mpi.h>
#define USE_UPCXX 0
#define USE_MPI 1
#include "globals.h"

#define Index3D(i,j,k)                                          \
  ((LAST_DIM(i, j, k))+                                         \
   (LAST_DIM(nx, ny, nz))*((j) + (ny)*(FIRST_DIM(i, j, k))))

struct position {
  int x, y, z;
  position operator+(position p) {
    position res = { x + p.x, y + p.y, z + p.z };
    return res;
  }
  position operator-(position p) {
    position res = { x - p.x, y - p.y, z - p.z };
    return res;
  }
};

inline position POS(int x, int y, int z) {
  position p = { x, y, z };
  return p;
}

int THREADS, MYTHREAD;
int xdim, ydim, zdim;
int xparts, yparts, zparts;
int nx, ny, nz;
int nx_interior, ny_interior, nz_interior;
#ifdef USE_ARRAYS
rectdomain<3> myDomain;
ndarray<double, 3 UNSTRIDED> myGridA, myGridB;
#endif
double *myGridAPtr, *myGridBPtr;
int neighbors[6];
int planeSizes[6];
position packStarts[6], packEnds[6];
position unpackStarts[6], unpackEnds[6];
double *inBufs[6], *outBufs[6];
MPI_Request requests[12];
int s2r[] = { 1, 0, 3, 2, 5, 4 };
int steps;

enum timer_type {
  COMMUNICATION,
  COMPUTATION
};
enum timer_index {
  POST_RECEIVES = 0,
  PACK,
  POST_SENDS,
  WAITALL,
  UNPACK,
  COMPUTE,
  NUM_TIMERS
};
timer timers[NUM_TIMERS];
string timerStrings[] = {"post receives", "pack",
                         "post sends", "sync messages",
                         "unpack", "probe compute"};
timer_type timerTypes[] = {COMMUNICATION, COMMUNICATION,
                           COMMUNICATION, COMMUNICATION,
                           COMMUNICATION, COMPUTATION};

position threadToPos(position parts, int threads, int i) {
  int xpos = i / (parts.y * parts.z);
  int ypos = (i % (parts.y * parts.z)) / parts.z;
  int zpos = i % parts.z;
  return POS(xpos, ypos, zpos);
}

int posToThread(position parts, int threads, position pos) {
  if (pos.x < 0 || pos.x >= parts.x ||
      pos.y < 0 || pos.y >= parts.y ||
      pos.z < 0 || pos.z >= parts.z) {
    return -1;
  } else {
    return pos.x * parts.y * parts.z + pos.y * parts.z + pos.z;
  }
}

position computeSize(position dims, position parts, int threads,
                     int mythread) {
  position pos = threadToPos(parts, threads, mythread);
  int xstart, xend, ystart, yend, zstart, zend;
  int num, rem;
  num = dims.x / parts.x;
  rem = dims.x % parts.x;
  xstart = num * pos.x + (pos.x <= rem ?  pos.x : rem);
  xend = xstart + num + (pos.x < rem ? 1 : 0);
  num = dims.y / parts.y;
  rem = dims.y % parts.y;
  ystart = num * pos.y + (pos.y <= rem ?  pos.y : rem);
  yend = ystart + num + (pos.y < rem ? 1 : 0);
  num = dims.z / parts.z;
  rem = dims.z % parts.z;
  zstart = num * pos.z + (pos.z <= rem ?  pos.z : rem);
  zend = zstart + num + (pos.z < rem ? 1 : 0);
  return POS(xend - xstart, yend - ystart, zend - zstart);
}

void setupComm(position parts, int threads, int mythread) {
  position mypos = threadToPos(parts, threads, mythread);
  neighbors[0] = posToThread(parts, threads, mypos - POS(1,0,0));
  neighbors[1] = posToThread(parts, threads, mypos + POS(1,0,0));
  neighbors[2] = posToThread(parts, threads, mypos - POS(0,1,0));
  neighbors[3] = posToThread(parts, threads, mypos + POS(0,1,0));
  neighbors[4] = posToThread(parts, threads, mypos - POS(0,0,1));
  neighbors[5] = posToThread(parts, threads, mypos + POS(0,0,1));
  planeSizes[0] = ny_interior * nz_interior;
  planeSizes[1] = ny_interior * nz_interior;
  planeSizes[2] = nx_interior * nz_interior;
  planeSizes[3] = nx_interior * nz_interior;
  planeSizes[4] = nx_interior * ny_interior;
  planeSizes[5] = nx_interior * ny_interior;
  packStarts[0] = POS(GHOST_WIDTH, GHOST_WIDTH, GHOST_WIDTH);
  packStarts[1] = POS(nx-2*GHOST_WIDTH, GHOST_WIDTH, GHOST_WIDTH);
  packStarts[2] = POS(GHOST_WIDTH, GHOST_WIDTH, GHOST_WIDTH);
  packStarts[3] = POS(GHOST_WIDTH, ny-2*GHOST_WIDTH, GHOST_WIDTH);
  packStarts[4] = POS(GHOST_WIDTH, GHOST_WIDTH, GHOST_WIDTH);
  packStarts[5] = POS(GHOST_WIDTH, GHOST_WIDTH, nz-2*GHOST_WIDTH);
  packEnds[0] = POS(2*GHOST_WIDTH, ny-GHOST_WIDTH, nz-GHOST_WIDTH);
  packEnds[1] = POS(nx-GHOST_WIDTH, ny-GHOST_WIDTH, nz-GHOST_WIDTH);
  packEnds[2] = POS(nx-GHOST_WIDTH, 2*GHOST_WIDTH, nz-GHOST_WIDTH);
  packEnds[3] = POS(nx-GHOST_WIDTH, ny-GHOST_WIDTH, nz-GHOST_WIDTH);
  packEnds[4] = POS(nx-GHOST_WIDTH, ny-GHOST_WIDTH, 2*GHOST_WIDTH);
  packEnds[5] = POS(nx-GHOST_WIDTH, ny-GHOST_WIDTH, nz-GHOST_WIDTH);
  unpackStarts[0] = POS(0, GHOST_WIDTH, GHOST_WIDTH);
  unpackStarts[1] = POS(nx-GHOST_WIDTH, GHOST_WIDTH, GHOST_WIDTH);
  unpackStarts[2] = POS(GHOST_WIDTH, 0, GHOST_WIDTH);
  unpackStarts[3] = POS(GHOST_WIDTH, ny-GHOST_WIDTH, GHOST_WIDTH);
  unpackStarts[4] = POS(GHOST_WIDTH, GHOST_WIDTH, 0);
  unpackStarts[5] = POS(GHOST_WIDTH, GHOST_WIDTH, nz-GHOST_WIDTH);
  unpackEnds[0] = POS(GHOST_WIDTH, ny-GHOST_WIDTH, nz-GHOST_WIDTH);
  unpackEnds[1] = POS(nx, ny-GHOST_WIDTH, nz-GHOST_WIDTH);
  unpackEnds[2] = POS(nx-GHOST_WIDTH, GHOST_WIDTH, nz-GHOST_WIDTH);
  unpackEnds[3] = POS(nx-GHOST_WIDTH, ny, nz-GHOST_WIDTH);
  unpackEnds[4] = POS(nx-GHOST_WIDTH, ny-GHOST_WIDTH, GHOST_WIDTH);
  unpackEnds[5] = POS(nx-GHOST_WIDTH, ny-GHOST_WIDTH, nz);
}

void initGrid(double *grid, size_t size) {
#ifdef RANDOM_VALUES
  for (size_t i = 0; i < size; i++) {
    grid[i] = ((double) rand()) / RAND_MAX;
  }
#else
  for (size_t i = 0; i < size; i++) {
    grid[i] = CONSTANT_VALUE;
  }
#endif
}

void pack(double *grid, double **bufs) {
  for (int i = 0; i < 6; i++) {
    if (neighbors[i] != -1) {
      double *buf = bufs[i];
      int bidx = 0;
      for (int x = packStarts[i].x; x < packEnds[i].x; x++) {
        for (int y = packStarts[i].y; y < packEnds[i].y; y++) {
          for (int z = packStarts[i].z; z < packEnds[i].z; z++) {
            buf[bidx++] = grid[Index3D(x, y, z)];
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
      for (int x = unpackStarts[i].x; x < unpackEnds[i].x; x++) {
        for (int y = unpackStarts[i].y; y < unpackEnds[i].y; y++) {
          for (int z = unpackStarts[i].z; z < unpackEnds[i].z; z++) {
            grid[Index3D(x, y, z)] = buf[bidx++];
          }
        }
      }
    }
  }
}

// Perform stencil.
void probe(int steps) {
  for (int i = 0; i < steps; i++) {
    // Copy ghost zones from previous timestep.
    timers[POST_RECEIVES].start();
    int messageNum = 0;
    for (int j = 0; j < 6; j++) {
      if (neighbors[j] != -1) {
        MPI_Irecv(inBufs[s2r[j]], planeSizes[j] * GHOST_WIDTH,
                  MPI_DOUBLE, neighbors[j], s2r[j],
                  MPI_COMM_WORLD, &requests[messageNum++]);
      }
    }
    timers[POST_RECEIVES].stop();
    timers[PACK].start();
    pack(myGridAPtr, outBufs);
    timers[PACK].stop();
    timers[POST_SENDS].start();
    for (int j = 0; j < 6; j++) {
      if (neighbors[j] != -1) {
        MPI_Isend(outBufs[j], planeSizes[j] * GHOST_WIDTH, MPI_DOUBLE,
                  neighbors[j], j, MPI_COMM_WORLD,
                  &requests[messageNum++]);
      }
    }
    timers[POST_SENDS].stop();
    timers[WAITALL].start();
    MPI_Waitall(messageNum, requests, MPI_STATUSES_IGNORE);
    timers[WAITALL].stop();
    timers[UNPACK].start();
    unpack(myGridAPtr, inBufs);
    timers[UNPACK].stop();

    timers[COMPUTE].start();
#ifdef USE_ARRAYS
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
#else
    for (int FIRST_DIM(i, j, k) = GHOST_WIDTH;
         FIRST_DIM(i, j, k) < FIRST_DIM(nx, ny, nz)-GHOST_WIDTH;
         FIRST_DIM(i, j, k)++) {
      for (int j = GHOST_WIDTH; j < ny-GHOST_WIDTH; j++) {
        for (int LAST_DIM(i, j, k) = GHOST_WIDTH;
             LAST_DIM(i, j, k) < LAST_DIM(nx, ny, nz)-GHOST_WIDTH;
             LAST_DIM(i, j, k)++) {
          myGridBPtr[Index3D(i, j, k)] =
            myGridAPtr[Index3D(i, j, k+1)] +
            myGridAPtr[Index3D(i, j, k-1)] +
            myGridAPtr[Index3D(i, j+1, k)] +
            myGridAPtr[Index3D(i, j-1, k)] +
            myGridAPtr[Index3D(i+1, j, k)] +
            myGridAPtr[Index3D(i-1, j, k)] +
            WEIGHT * myGridAPtr[Index3D(i, j, k)];
        }
      }
    }
#endif
    timers[COMPUTE].stop();

    // Swap pointers
#ifdef USE_ARRAYS
    SWAP(myGridA, myGridB);
#endif
    SWAP(myGridAPtr, myGridBPtr);
  }
}

# define report(d, s) do { \
    if (MYTHREAD == 0)      \
      cout << s;            \
    report_value(d);        \
  } while (0)

void report_value(double d) {
  if (MYTHREAD == 0) {
    cout << ": " << d << endl;
  }
}

void printTimingStats() {
  for (int i = 0; i < NUM_TIMERS; i++) {
    report(timers[i].secs(), "Time for " << timerStrings[i] << " (s)");
  }
}

int main(int argc, char **args) {
  MPI_Init(&argc, &args);

  MPI_Comm_size(MPI_COMM_WORLD, &THREADS);
  MPI_Comm_rank(MPI_COMM_WORLD, &MYTHREAD);

  if (argc > 1 && (argc < 8 || !strncmp(args[1], "-h", 2))) {
    cout << "Usage: stencil <xdim> <ydim> <zdim> "
         << "<xparts> <yparts> <zparts> <timesteps>" << endl;
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

  position size = computeSize(POS(xdim,ydim,zdim),
                              POS(xparts,yparts,zparts),
                              THREADS, MYTHREAD);
  nx_interior = size.x;
  ny_interior = size.y;
  nz_interior = size.z;
  nx = size.x + 2 * GHOST_WIDTH;
  ny = size.y + 2 * GHOST_WIDTH;
  nz = size.z + 2 * GHOST_WIDTH;

#ifdef USE_ARRAYS
  myDomain =
    RD(PT(0, 0, 0), PT(nx_interior, ny_interior, nz_interior));
  myGridA.create(myDomain.accrete(GHOST_WIDTH) CMAJOR);
  myGridB.create(myDomain.accrete(GHOST_WIDTH) CMAJOR);
  myGridAPtr = myGridA.base_ptr();
  myGridBPtr = myGridB.base_ptr();
#else
  myGridAPtr = (double *) malloc(nx * ny * nz * sizeof(double));
  myGridBPtr = (double *) malloc(nx * ny * nz * sizeof(double));
#endif

  // Compute ordered ghost zone overlaps, x -> y -> z.
  setupComm(POS(xparts,yparts,zparts), THREADS, MYTHREAD);
  
  for (int i = 0; i < 6; i++) {
    if (neighbors[i] != -1) {
      inBufs[s2r[i]] =
        (double *) malloc(planeSizes[i] * GHOST_WIDTH *
                          sizeof(double));
      outBufs[i] =
        (double *) malloc(planeSizes[i] * GHOST_WIDTH *
                          sizeof(double));
    }
#if DEBUG
    position p = packEnds[i] - packStarts[i];
    position q = unpackEnds[i] - unpackStarts[i];
    cout << MYTHREAD << ": Ghost size in dir " << i << ": "
         << (planeSizes[i] * GHOST_WIDTH) << ", "
         << (p.x * p.y * p.z) << ", "
         << (q.x * q.y * q.z) << endl;
#endif
  }

  timer t;
  initGrid(myGridAPtr, nx * ny * nz);
  initGrid(myGridBPtr, nx * ny * nz);
  MPI_Barrier(MPI_COMM_WORLD); // wait for all threads before starting
  t.start();
  probe(steps);
  t.stop();

  double val =
    myGridAPtr[Index3D(GHOST_WIDTH, GHOST_WIDTH, GHOST_WIDTH)];
  if (MYTHREAD == 0) {
    cout << "Time for stencil with split " << xparts << "x" << yparts
         << "x" << zparts << " (s): " << t.secs() << endl;
    cout << "Verification value: " << val << endl;
  }
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

  MPI_Finalize();
}
