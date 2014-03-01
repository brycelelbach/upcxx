#pragma once

#include <string>
#include <iostream>
#include <fstream>
#include "globals.h"

/** Copied from code by Luigi and Geoff: this dumps an array to a
    file, plotfile, in a standard way. */
    
// A simple way to dump plot data.

class plot {
private:
  ofstream *out;

  plot() : out(NULL) {}

  void init_output_file() {
    init_output_file("plotfile");
  }

  void init_output_file(const string &filename) {
    out = new ofstream();
    out->open(filename.c_str());
  }

  void add(ndarray<double, 2, global> x,
           const string &filename) {
    if (out == NULL) {
      init_output_file(filename);
    }
    (*out) << (x.domain().max()[1] - x.domain().min()[1] + 1) 
           << " " << (x.domain().max()[2] - x.domain().min()[2] + 1)
           << " 1" << endl;
    foreach (p, x.domain()) {
      (*out) << x[p] << endl;
    }
  }

  void add_full(ndarray<double, 2, global> x,
                const string &filename) {
    if (out == NULL) {
      init_output_file(filename);
    }
    (*out) << "Domain: " << x.domain() << endl;
    foreach (p, x.domain()) {
      (*out) << " " << p << " " << x[p] << endl;
    }
  }

public:
  static void dump(ndarray<double, 2, global> x,
                   const string &filename) {
    plot p;
    p.add(x, filename);
    p.out->flush();
    cout.flush();
  }

  static void full_dump(ndarray<double, 2, global> x,
                        const string &filename) {
    plot p;
    p.add_full(x, filename);
    p.out->flush();
    cout.flush();
  }
};
