#pragma once

#include "Util.h"

class BoxArray {

  static BoxArray BoxArrayNull;

  ndarray<rectdomain<SPACE_DIM>, 1> domains;

  // procs[i] is process where data for box i are kept
  ndarray<int, 1> procs;

  // pa[proc] is the set of indices of boxes living on process proc
  ndarray<domain<1>, 1> pa;

public:
  BoxArray() {};

  // constructor
  BoxArray(ndarray<rectdomain<SPACE_DIM>, 1> domainlist,
           ndarray<int, 1> proclist) {
    domains = domainlist;
    procs = proclist;

    rectdomain<1> Dprocs(0, THREADS);

    // Set pa.
    pa = ndarray<domain<1>, 1>(Dprocs);
    foreach (indproc, Dprocs) {
      pa[indproc] = RD(0, 0);
    }

    if (domains != NULL) {
      foreach (indbox, domains.domain()) {
	int procbox = procs[indbox];
	pa[procbox] = pa[procbox] + RD(indbox, indbox);
      }
    }
  }

  // methods

  // Returns the domain of a particular box.
  rectdomain<SPACE_DIM> operator[](point<1> p) {
    return domains[p];
  }

  rectdomain<SPACE_DIM> operator[](int ip) {
    return domains[ip];
  }

  // Returns all indices of boxes.
  rectdomain<1> indices() const { 
    if (domains != NULL) {
      return domains.domain();
    } else {
      return RD(0, 0);
    }
  }

  // Returns the number of the process where data for this box reside.
  int proc(point<1> index) {
    return procs[index];
  }

  int proc(int index) {
    return procs[index];
  }

  // Returns the indices of boxes living on process.
  domain<1> procboxes(int process) {
    return pa[process];
  }

  // Returns the mapping of boxes to processes.
  ndarray<int, 1> proclist() {
    return procs;
  }

  // Returns the boxes.
  ndarray<rectdomain<SPACE_DIM>, 1> boxes() {
    return domains;
  }

  bool isNull() {
    return (domains == NULL);
  }

  // return the indices of the boxes that contain the cell.
  domain<1> findCell(point<SPACE_DIM> cell) {
    domain<1> D = RD(0, 0);
    foreach (indbox, indices())
      if (domains[indbox].contains(cell))
        D = D + RD(indbox, indbox+1);
    return D;
  }

  // Print a string representation to the given stream.
  friend std::ostream& operator<<(std::ostream& os,
                                  const BoxArray& ba) {
    foreach (indbox, ba.indices())
      os << "  " << ba.domains[indbox];
    return os;
  }

  static BoxArray Null() {
    return BoxArrayNull;
  }
};
