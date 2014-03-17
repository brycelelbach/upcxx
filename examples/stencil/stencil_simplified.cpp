// Stencil
#include "globals.h"

static int xdim, ydim, zdim;
static int xparts, yparts, zparts;
static ndarray<rectdomain<3>, 1> allDomains;
static rectdomain<3> myDomain;
static ndarray<double, 3, simple> myGridA, myGridB;
static ndarray<ndarray<double, 3, global>, 1> allGridsA, allGridsB;
static ndarray<ndarray<double, 3, global>, 1> targetsA, targetsB;
static ndarray<ndarray<double, 3>, 1> sourcesA, sourcesB;
static int steps;

static point<3> threadToPos(point<3> parts, int threads, int i) {
  int xpos = i / (parts[2] * parts[3]);
  int ypos = (i % (parts[2] * parts[3])) / parts[3];
  int zpos = i % parts[3];
  return PT(xpos, ypos, zpos);
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
static ndarray<rectdomain<3>, 1> computeDomains(point<3> dims,
                                                point<3> parts,
                                                int threads) {
  ndarray<rectdomain<3>, 1> domains(RD(PT(0), PT(threads)));
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

static ndarray<int, 1> computeNeighbors(point<3> parts, int threads,
                                        int mythread) {
  point<3> mypos = threadToPos(parts, threads, mythread);
  ndarray<int, 1> neighbors = ARRAY(int, ((0), (6)));
  neighbors[0] = posToThread(parts, threads, mypos - PT(1,0,0));
  neighbors[1] = posToThread(parts, threads, mypos + PT(1,0,0));
  neighbors[2] = posToThread(parts, threads, mypos - PT(0,1,0));
  neighbors[3] = posToThread(parts, threads, mypos + PT(0,1,0));
  neighbors[4] = posToThread(parts, threads, mypos - PT(0,0,1));
  neighbors[5] = posToThread(parts, threads, mypos + PT(0,0,1));
  return neighbors;
}

static void initGrid(ndarray<double, 3> grid) {
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
  for (int i = 0; i < steps; i++) {
    // Copy ghost zones from previous timestep.
    for (int j = 0; j < 6; j++) {
      if (targetsA[j] != NULL) {
        targetsA[j].copy_nbi(sourcesA[j]);
      }
    }
    async_copy_fence(); // sync async copies
    barrier(); // wait for puts from all nodes

    foreach3 (i, j, k, myDomain) {
      myGridB[PT(i, j, k)] =
        myGridA[PT(i, j, k+1)] +
        myGridA[PT(i, j, k-1)] +
        myGridA[PT(i, j+1, k)] +
        myGridA[PT(i, j-1, k)] +
        myGridA[PT(i+1, j, k)] +
        myGridA[PT(i-1, j, k)] +
        WEIGHT * myGridA[PT(i, j, k)];
    }
    // Swap pointers
    SWAP(myGridA, myGridB);
    SWAP(targetsA, targetsB);
    SWAP(sourcesA, sourcesB);
  }
}

int main(int argc, char **args) {
  init(&argc, &args);

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

  allDomains = computeDomains(PT(xdim,ydim,zdim),
                              PT(xparts,yparts,zparts),
                              THREADS);
  myDomain = allDomains[MYTHREAD];

  myGridA = ndarray<double, 3, simple>(myDomain.accrete(1));
  myGridB = ndarray<double, 3, simple>(myDomain.accrete(1));
  allGridsA =
    ndarray<ndarray<double, 3, global>, 1>(RD(PT(0), PT(THREADS)));
  allGridsB =
    ndarray<ndarray<double, 3, global>, 1>(RD(PT(0), PT(THREADS)));
  allGridsA.exchange(myGridA);
  allGridsB.exchange(myGridB);

  // Compute ordered ghost zone overlaps, x -> y -> z.
  ndarray<int, 1> nb = computeNeighbors(PT(xparts,yparts,zparts),
                                        THREADS, MYTHREAD);
  targetsA = ndarray<ndarray<double, 3, global>, 1>(RD(PT(0), PT(6)));
  targetsB = ndarray<ndarray<double, 3, global>, 1>(RD(PT(0), PT(6)));
  sourcesA = ndarray<ndarray<double, 3>, 1>(RD(PT(0), PT(6)));
  sourcesB = ndarray<ndarray<double, 3>, 1>(RD(PT(0), PT(6)));
  for (int i = 0; i < 6; i++) {
    if (nb[i] != -1) {
      targetsA[i] = allGridsA[nb[i]].constrict(myDomain);
      targetsB[i] = allGridsB[nb[i]].constrict(myDomain);
      sourcesA[i] = myGridA;
      sourcesB[i] = myGridB;
    }
  }

  timer t;
  initGrid(myGridA);
  initGrid(myGridB);
  barrier(); // wait for all threads before starting
  t.start();
  probe(steps);
  t.stop();

  double val = myGridA[myDomain.min()];
  if (MYTHREAD == 0) {
    cout << "Time for stencil with split " << xparts << "x" << yparts
         << "x" << zparts << " (s): " << t.secs() << endl;
    cout << "Verification value: " << val << endl;
  }

  finalize();
}
