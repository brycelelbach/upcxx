#include "Util.h"

int Util::N;
int Util::numProcRows, Util::numProcCols;
int Util::procRowPos, Util::procColPos;
int Util::procRowSize, Util::procColSize;
int Util::rowStart, Util::rowEnd;
int Util::colStart, Util::colEnd;
int Util::log2numProcCols;
#ifdef TEAMS
Team *Util::rowTeam;
Team *Util::colTeam;
#endif

void Util::initialize(int paramN, int procCount,
                      int procId) {
  N = paramN;

  // Determine processor grid.
  numProcRows = numProcCols = 1;
  while (procCount > 1) {
    numProcCols *= 2;
    procCount /= 2;
    if (procCount > 1) {
      numProcRows *= 2;
      procCount /= 2;
    }
  }

  // determine position and sizes within the processor grid
  procRowPos = procIdToRowPos(procId);
  procColPos = procIdToColPos(procId);
  procRowSize = N / numProcRows;
  procColSize = N / numProcCols;
  // adjust for remainder
  if (procRowPos < N % numProcRows) {
    procRowSize++;
    rowStart = procRowSize * procRowPos;
    rowEnd = rowStart + procRowSize - 1;
  } else {
    rowEnd = N - (procRowSize * (numProcRows - procRowPos - 1)) - 1;
    rowStart = rowEnd - procRowSize + 1;
  }
  // if numProcRows != numProcCols, must make sure that columns line
  // up with rows, 2 columns to a row
  if (numProcRows != numProcCols) {
    // Essentially, divide into numProcRows pieces, then divide each
    // piece into two.
    int ncpos = procColPos / 2;
    int npcols = numProcRows;
    int npcsize = N / npcols;
    if (ncpos < N % npcols) {
      npcsize++;
      colStart = npcsize * ncpos;
      colEnd = colStart + npcsize - 1;
    } else {
      colEnd = N - (npcsize * (npcols - ncpos - 1)) - 1;
      colStart = colEnd - npcsize + 1;
    }
    if (procColPos % 2 == 0) {
      colEnd = (colStart + colEnd) / 2;
    } else {
      colStart = (colStart + colEnd) / 2 + 1;
    }
    procColSize = colEnd - colStart + 1;
  } else if (procColPos < N % numProcCols) {
    procColSize++;
    colStart = procColSize * procColPos;
    colEnd = colStart + procColSize - 1;
  } else {
    colEnd = N - (procColSize * (numProcCols - procColPos - 1)) - 1;
    colStart = colEnd - procColSize + 1;
  }

  // determine log base 2 of the number of columns
  log2numProcCols = 0;
  int i = numProcCols / 2;
  while (i > 0) {
    log2numProcCols++;
    i /= 2;
  }
#ifdef TEAMS
  rowTeam = new Team();
  rowTeam->splitTeamAll(procRowPos, procColPos);
  // rowTeam->initialize();
  // if (MYTHREAD == 0)
  //   println("done initializing row team");
  colTeam = new Team();
  colTeam->splitTeamAll(procColPos, procRowPos);
  // colTeam->initialize();
  // if (MYTHREAD == 0) {
  //   println("rowTeam:");
  //   println(rowTeam);
  //   println("colTeam:");
  //   println(colTeam);
  // }
#endif
}

// Test code.
void Util::test(int na, int minnum, int maxnum) {
  for (int i = minnum; i <= maxnum; i *= 2) {
    initialize(na, i, 0);
    println("numProcs: " << i);
    println("numProcRows: " << numProcRows);
    println("numProcCols: " << numProcCols);
    println("procRowPos: " << procRowPos);
    println("procColPos: " << procColPos);
    println("procRowSize: " << procRowSize);
    println("procColSize: " << procColSize);
    println("log2numProcCols: " << log2numProcCols);
    println("id: rowStart - rowEnd, colStart - colEnd");
    for (int j = 0; j < i; j++) {
      initialize(na, i, j);
      println(j << ": " << rowStart << " - " << rowEnd <<
              ", " << colStart << " - " << colEnd);
    }
    println("");
  }
}
