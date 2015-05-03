#pragma once

#include "globals.h"

/** This class computes common values used in the CG benchmark. */

class Util {
 public:
  // Size of each dimension.
  static int N;

  // Number of processor rows and columns.
  static int numProcRows, numProcCols;

  // Row and column of this processor.
  static int procRowPos, procColPos;

  // Numer of rows and columns owned by this processor.
  static int procRowSize, procColSize;

  // Start and end row, inclusive, of this processor.
  static int rowStart, rowEnd;

  // Start and end column, inclusive, of this processor.
  static int colStart, colEnd;

  // Log base 2 of the number of columns.
  static int log2numProcCols;

#ifdef TEAMS
  static team *rowTeam;
  static team *colTeam;
#endif

  static int procIdToRowPos(int procId) {
    return procId / numProcCols;
  }

  static int procIdToColPos(int procId) {
    return procId % numProcCols;
  }

  static int posToProcId(int rowPos, int colPos) {
    return rowPos * numProcCols + colPos;
  }

  static int transposeProc(int procId) {
    int num = numProcRows;
    int res;
    if (numProcRows == numProcCols) {
      res = (procId % num) * num + procId/num;
    } else {
      res = 2 * (((procId/2) % num) * num + procId/2/num) + (procId % 2);
    }
    return res;
  }

  static void initialize(int paramN, int procCount, int procId);

  static void initialize(int paramN) {
    initialize(paramN, ranks(), myrank());
  }

  static void test(int na, int minnum, int maxnum);
};
