#pragma once

// the Histogram class.

/******************************************************************************
 CHANGE LOG.

 16 Mar 1999:
  Creation. [pike]

******************************************************************************/

#include <iostream>
#include <string>
#include "globals.h"

template<class T> class histogram {
private:
  T key;
  int count;
  histogram *left, *right;

public:
  histogram() : count(0), left(NULL), right(NULL) {}

  int add(T k) {
    histogram *z = this;
    println("A " << k);
    while (z->count != 0) {
      if (k == key) 
	return ++z->count;
      else if (k < key) {
	if (z->left == NULL)
	  z->left = new histogram();
	z = z->left;
      } else {
	if (z->right == NULL)
	  z->right = new histogram();
	z = z->right;
      }
    }
    z->key = k;
    z->count = 1;
    return 1;
  }
  
  int size() {
    return (count +
	    (left == NULL ? 0 : left->size()) +
	    (right == NULL ? 0 : right->size()));
  }

  void dump() {
    dump(cout);
  }

  void dump(ostream &s) {
    dump(s, "");
  }

  void dump(const string &label) {
    dump(cout, label);
  }

  void dump(ostream &s, const string &label) {
    if (count == 0) 
      s << label << "\nEmpty." << endl;
    else {
      s << " " << size() << " total." << endl;
      dumpNoCount(s, label);
    }
  }

  void dumpNoCount(ostream &s, const string &label) {
    if (left == NULL) 
      s << label << endl;
    else
      left->dumpNoCount(s, label);

    s << " " << key << ": " << count << endl;
    
    if (right != NULL)
      right->dumpNoCount(s, label);
  }
};
