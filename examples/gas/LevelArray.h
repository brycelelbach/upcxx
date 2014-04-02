#pragma once

#include <sstream>
#include "BoxArray.h"
#include "Util.h"

template<class T> class LevelArray {
  BoxArray ba;
  ndarray<ndarray<T, SPACE_DIM, global>, 1> arrs;

public:
  LevelArray() {};

  /**
   * constructor
   */
  LevelArray(BoxArray Ba) : ba(Ba) {
    // set fields
    // ba = Ba;
    rectdomain<1> indices = ba.indices();
    arrs = ndarray<ndarray<T, SPACE_DIM, global>, 1>(indices);

    // process number stuff
    int myProc = MYTHREAD;
    int numProcs = THREADS;
    rectdomain<1> Dprocs(0, numProcs);

    {
      // set up arrays of references to grids, and exchange references
      ndarray<ndarray<T, SPACE_DIM, global>, 1, global> arrlistloc(indices);

      ndarray<ndarray<ndarray<T, SPACE_DIM, global>, 1, global>, 1> arrlist(Dprocs);

      arrlist.exchange(arrlistloc);

      // define new grids living on process myProc
      foreach (ibox, ba.procboxes(myProc))
	arrlist[myProc][ibox] = ndarray<T, SPACE_DIM, global>(ba[ibox]);;

      barrier();

      // get references from the appropriate processes
      foreach (ibox, indices) arrs[ibox] = arrlist[ba.proc(ibox)][ibox];

      barrier();

      arrlistloc.destroy();
      arrlist.destroy();
    }
  }

  void destroy() {
    arrs.destroy();
  }

  // methods

  BoxArray boxArray() {
    return ba;
  }

  ndarray<T, SPACE_DIM, global> &operator[](point<1> p) {
    return arrs[p];
  }

  ndarray<T, SPACE_DIM, global> &operator[](int ip) {
    return arrs[ip];
  }

  rectdomain<1> domain() {
    return ba.indices();
  }

  void copy(LevelArray &U) {
    foreach (lba, ba.procboxes(MYTHREAD))
      arrs[lba].copy(U[lba]);
  }

  void checkNaN(const string &str) {
    ostringstream oss;
    foreach (lba, ba.procboxes(MYTHREAD)) {
      foreach (p, arrs[lba].domain()) {
        oss << str << lba << p;
        arrs[lba][p].checkNaN(oss.str());
        oss.str("");
      }
    }
  }

  static LevelArray Null() {
    return LevelArray();
  }
};
