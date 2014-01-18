#include "Grid.h"

Point<3> Grid::numCellsPerBlockSide;
Point<3> Grid::numBlocksInGridSide;
Point<3> Grid::myBlockPos;

int ipow(int base, int exponent) {
  if (exponent == 0)
    return 1;
  int subexp = ipow(base, exponent / 2);
  subexp *= subexp;
  if (exponent % 2 == 1)
    subexp *= base;
  return subexp;
}

Grid::Grid(int level, bool ghostCellsNeeded, bool proc0BuffersNeeded) {
  int numCellsInGridSide = ipow(2, level);
  numCellsPerBlockSide = numCellsInGridSide / numBlocksInGridSide;

  // determine the startCell and endCell for each rectangular block (using global coords)
  Point<3> startCell, endCell;
  global_ndarray<double, 3> myPoints;

  if (level >= THRESHOLD_LEVEL) {
    startCell = myBlockPos * numCellsPerBlockSide;
    endCell = startCell + (numCellsPerBlockSide - POINT(1,1,1));
  }
  else {
    // only block with position (0,0,0) has cells in it
    if (myBlockPos == POINT(0,0,0)) {
      startCell = POINT(0,0,0);
      endCell = POINT(numCellsInGridSide-1,
                      numCellsInGridSide-1,
                      numCellsInGridSide-1);
    }
    // otherwise, produce invalid domain (THIS COULD BE CLEANER)
    else {
      startCell = POINT(2,2,2);
      endCell = POINT(-1,-1,-1);
    }
  }

  // create "points" distributed array
  if (ghostCellsNeeded) {
    RectDomain<3> rd(startCell - POINT(1, 1, 1), endCell + POINT(2, 2, 2));
    myPoints = ndarray<double, 3>(rd);
  }
  else {
    RectDomain<3> rd(startCell, endCell + POINT(1, 1, 1));
    myPoints = ndarray<double, 3>(rd);
  }

  ndarray<global_ndarray<double, 3>, 1> tempPoints(RECTDOMAIN((0), ((int)THREADS)));
  tempPoints.exchange(myPoints);

  RectDomain<3> blocks_rd(POINT(0, 0, 0), numBlocksInGridSide);
  points = ndarray<global_ndarray<double, 3>, 3>(blocks_rd);
  foreach (p, points.domain()) {
    points[p] = tempPoints[procForBlockPosition(p)];
  }

  // create "myPointsOnProc0" and "bufferForProc0" if needed
  if (proc0BuffersNeeded) {
    if (MYTHREAD == 0) {
      localBuffersOnProc0 = ndarray<global_ndarray<double, 3>, 3>(blocks_rd);
      foreach (p, localBuffersOnProc0.domain()) {
        startCell = p * numCellsPerBlockSide;
        endCell = startCell + (numCellsPerBlockSide - POINT(1,1,1));

        // includes necessary ghost cells
        RectDomain<3> buf_rd(startCell - POINT(1, 1, 1), endCell + POINT(2, 2, 2));
        localBuffersOnProc0[p] = ndarray<double, 3>(buf_rd);
      }
    }

    global_ndarray<global_ndarray<double, 3>, 3> buffersOnProc0 =
      broadcast(localBuffersOnProc0, 0);
    myRemoteBufferOnProc0 = buffersOnProc0[myBlockPos];

    startCell = myBlockPos * numCellsPerBlockSide;
    endCell = startCell + (numCellsPerBlockSide - POINT(1,1,1));

    // includes necessary ghost cells
    RectDomain<3> mybuf_rd(startCell - POINT(1, 1, 1), endCell + POINT(2, 2, 2));
    myBufferForProc0 = ndarray<double, 3>(mybuf_rd);
  }

  // create contiguous array buffers (if appropriate flag is set)
#ifdef CONTIGUOUS_ARRAY_BUFFERS
  RectDomain<3> myPointsRD = myPoints.domain();

  if (ghostCellsNeeded && (level >= THRESHOLD_LEVEL)) {
#if UPDATE_BORDER_DIRECTIONS == 6
    // this RectDomain indicates the direction of the buffer
    // e.g. -3= -z dir (neg. xy-plane), +2= +y dir (pos. xz-plane)
    RectDomain<1> allDirections(POINT(-3), POINT(4));

    ndarray<global_ndarray<double, 3>, 1> myOutBuffer(allDirections);
    ndarray<global_ndarray<double, 3>, 1> myInBuffer(allDirections);

    foreach (dir, allDirections) {
      if (dir != POINT(0)) {
        RectDomain<3> myOutBufferRD = myPointsRD.border(1, dir[1], -1);
        RectDomain<3> myInBufferRD = myPointsRD.border(1, dir[1], 0);

        // the following loop is used to "shrink" the border edges appropriately
        int maxDim = abs(dir[1]);
        for (int dim=2; dim<=maxDim; dim++) {
          int shrinkDim = dim-1;
          myOutBufferRD = myOutBufferRD.shrink(1, -shrinkDim).shrink(1, shrinkDim);
          myInBufferRD = myInBufferRD.shrink(1, -shrinkDim).shrink(1, shrinkDim);
        }

        if (abs(dir[1]) == 1) {
          // if pos or neg yz-plane, no buffers needed- data is already contiguous
          myOutBuffer[dir] = myPoints.constrict(myOutBufferRD);
          myInBuffer[dir] = myPoints.constrict(myInBufferRD);
        }
        else {
          myOutBuffer[dir] = ndarray<double, 3>(myOutBufferRD);
          myInBuffer[dir] = ndarray<double, 3>(myInBufferRD);
        }
      }
    }

    // create the distributed arrays "outBuffers" and "inBuffers"
    ndarray<global_ndarray<global_ndarray<double, 3>, 1>, 1> tempOutBuffer(RECTDOMAIN((0), ((int)THREADS)));
    ndarray<global_ndarray<global_ndarray<double, 3>, 1>, 1> tempInBuffer(RECTDOMAIN((0), ((int)THREADS)));

    tempOutBuffer.exchange(myOutBuffer);
    tempInBuffer.exchange(myInBuffer);

    outBuffers = ndarray<global_ndarray<global_ndarray<double, 3>, 1>, 3>(blocks_rd);
    inBuffers = ndarray<global_ndarray<global_ndarray<double, 3>, 1>, 3>(blocks_rd);

    foreach (p, blocks_rd) {
      outBuffers[p] = tempOutBuffer[procForBlockPosition(p)];
      inBuffers[p] = tempInBuffer[procForBlockPosition(p)];
    }

#elif UPDATE_BORDER_DIRECTIONS == 26
    // this RectDomain indicates the direction of the buffer
    // e.g. [-1, -1, -1] is a corner point, while [0, -1, 0] is a plane
    RectDomain<3> allDirections((Point(-1, -1, -1), POINT(2, 2, 2)));

    ndarray<global_ndarray<double, 3>, 3> myOutBuffer(allDirections);
    ndarray<global_ndarray<double, 3>, 3> myInBuffer(allDirections);

    foreach (dir, allDirections) {
      RectDomain<3> myOutBufferRD = myPointsRD;
      RectDomain<3> myInBufferRD = myPointsRD;

      if (dir != [0,0,0]) {
        // dim is the dimension- x=1, y=2, z=3
        for (int dim=1; dim<=3; dim++) {
          if (dir[dim] != 0) {
            int dimDir = dim*dir[dim];
            myOutBufferRD = myOutBufferRD * myPointsRD.border(1, dimDir, -1);
            myInBufferRD = myInBufferRD * myPointsRD.border(1, dimDir, 0);
          }
        }

        // the following loop is used to "shrink" the border edges appropriately
        for (int dim=1; dim<=3; dim++) {
          if (dir[dim] == 0) {
            myOutBufferRD = myOutBufferRD.shrink(1, -dim).shrink(1, dim);
            myInBufferRD = myInBufferRD.shrink(1, -dim).shrink(1, dim);
          }
        }

        if ((dir[1] == 0) || (dir[2] == 0)) {
          // case where buffers are needed
          myOutBuffer[dir] = new double[myOutBufferRD];
          myInBuffer[dir] = new double[myInBufferRD];
        }
        else {
          // case where no buffers needed
          myOutBuffer[dir] = myPoints.constrict(myOutBufferRD);
          myInBuffer[dir] = myPoints.constrict(myInBufferRD);
        }
      }
    }

    // create the distributed array buffers "outBuffers" and "inBuffers"
    ndarray<global_ndarray<global_ndarray<double, 3>, 3, 1> tempOutBuffer(RECTDOMAIN((0), ((int)THREADS)));
    ndarray<global_ndarray<global_ndarray<double, 3>, 3, 1> tempInBuffer(RECTDOMAIN((0), ((int)THREADS)));

    tempOutBuffer.exchange(myOutBuffer);
    tempInBuffer.exchange(myInBuffer);

    outBuffers = ndarray<global_ndarray<global_ndarray<double, 3>, 3, 3>(blocks_rd);
    inBuffers = ndarray<global_ndarray<global_ndarray<double, 3>, 3, 3>(blocks_rd);

    foreach (p, blocks_rd) {
      outBuffers[p] = tempOutBuffer[procForBlockPosition(p)];
      inBuffers[p] = tempInBuffer[procForBlockPosition(p)];
    }
#endif
  }
#endif
	
  barrier();
}

void Grid::init() {
  // initialize "numBlocksInGridSide", a 3D point indicating the
  // number of blocks in each dim
  int numBlocksInGridSide_x = 1;
  int numBlocksInGridSide_y = 1;
  int numBlocksInGridSide_z = 1;
  int numBlocks = 1;
	
  while (2*numBlocks <= THREADS) {
    numBlocksInGridSide_x *= 2;
    numBlocks *= 2;
    if (2*numBlocks <= THREADS) {
      numBlocksInGridSide_y *= 2;
      numBlocks *= 2;
      if (2*numBlocks <= THREADS) {
        numBlocksInGridSide_z *= 2;
        numBlocks *= 2;
      }
    }
  }

  // numBlocksInGridSide = broadcast [numBlocksInGridSide_x, numBlocksInGridSide_y, numBlocksInGridSide_z] from 0;
  numBlocksInGridSide = POINT(numBlocksInGridSide_x,
                              numBlocksInGridSide_y,
                              numBlocksInGridSide_z);

  // initialize myBlockPos (this does the reverse of procForBlockPosition)
  int temp = MYTHREAD;
  int blockPos_x = temp / (numBlocksInGridSide[2] * numBlocksInGridSide[3]);
  temp -= blockPos_x * (numBlocksInGridSide[2] * numBlocksInGridSide[3]);
  int blockPos_y = temp / numBlocksInGridSide[3];
  temp -= blockPos_y * numBlocksInGridSide[3];
  int blockPos_z = temp;
  myBlockPos = POINT(blockPos_x, blockPos_y, blockPos_z);
}
