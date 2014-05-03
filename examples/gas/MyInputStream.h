#pragma once

#include <fstream>
#include <sstream>
#include "Util.h"

class MyInputStream {
private:
  /** number of integers used to specify a domain:  low, high, centering */
  enum { DOMAIN_LENGTH = 3 * SPACE_DIM };

  static rectdomain<1> DIMENSIONS;
 
  istream &base;

public:
  /**
   * constructor
   */
  MyInputStream(istream &in) : base(in) {}

  /*
    methods
  */

  string readLine() {
    string line;
    if (!getline(base, line))
      println("Error: end of file");
    return line;
  }

private:
  int countTokens(const string &s) {
    istringstream iss(s);
    int count = 0;
    for (string w; iss >> w; count++);
    return count;
  }

  void filterParensAndCommas(string &s) {
    for (int i = 0; i < s.size(); i++) {
      if (s[i] == '(' || s[i] == ')' || s[i] == ',')
        s[i] = ' ';
    }
  }
public:
  void ignoreLine() {
    readLine();
  }

  int readInt1();

  ndarray<int, 1> readInts();

  double readDouble1();

  ndarray<double, 1> readDoubles();

  rectdomain<2> readDomain();

  ndarray<rectdomain<2>, 1> readDomains();
};
