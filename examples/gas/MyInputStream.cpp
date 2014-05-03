#include "MyInputStream.h"

rectdomain<1> MyInputStream::DIMENSIONS(1, SPACE_DIM+1);

int MyInputStream::readInt1() {
  string line = readLine();
  istringstream iss(line);
  int result;
  if (!(iss >> result))
    println("Error: bad integer: " << line);
  return result;
}

ndarray<int, 1> MyInputStream::readInts() {
  string line = readLine();
  istringstream iss(line);
  int n = countTokens(line);
  ndarray<int, 1> vals(RD(0, n));
  int val;
  for (int i = 0; i < n; i++) {
    if (!(iss >> val))
      println("Error: bad integer in line: " << line);
    vals[i] = val;
  }
  return vals;
}

double MyInputStream::readDouble1() {
  string line = readLine();
  istringstream iss(line);
  double result;
  iss >> result;
  if (iss.fail())
    println("Error: bad floating point number: " << line);
  return result;
}

ndarray<double, 1> MyInputStream::readDoubles() {
  string line = readLine();
  istringstream iss(line);
  int n = countTokens(line);
  ndarray<double, 1> vals(RD(0, n));
  double val;
  for (int i = 0; i < n; i++) {
    if (!(iss >> val))
      println("Error: bad floating point number in line: " << line);
    vals[i] = val;
  }
  return vals;
}

rectdomain<2> MyInputStream::readDomain() {
  string line = readLine();
  filterParensAndCommas(line);
  istringstream iss(line);
  int n = countTokens(line);
  ndarray<int, 1> vals(RD(0, n));
  int val;
  for (int i = 0; i < n; i++) {
    if (!(iss >> val))
      println("Error: bad integer in domain: " << line);
    vals[i] = val;
  }
  point<2> lo = Util::ZERO, hi = Util::ONE;
  foreach (pdim, DIMENSIONS) {
    int dim = pdim[1];
    lo = lo + point<2>::direction(dim, vals[dim]);
    hi = hi + point<2>::direction(dim, vals[dim + SPACE_DIM]);
  }
  return RD(lo, hi);
}

ndarray<rectdomain<2>, 1> MyInputStream::readDomains() {
  string line = readLine();
  filterParensAndCommas(line);
  istringstream iss(line);
  int n = countTokens(line);
  ndarray<int, 1> vals(RD(0, n));
  int val;
  for (int i = 0; i < n; i++) {
    if (!(iss >> val))
      println("Error: bad integer in domain: " << line);
    vals[i] = val;
  }
  int numDomains = n / DOMAIN_LENGTH;
  ndarray<rectdomain<2>, 1> domains(RD(0, numDomains));
  for (int i = 0; i < numDomains; i++) {
    int base = i * DOMAIN_LENGTH;
    point<2> lo = Util::ZERO, hi = Util::ONE;
    foreach (pdim, DIMENSIONS) {
      int dim = pdim[1];
      lo = lo + point<2>::direction(dim, vals[base + dim]);
      hi = hi + point<2>::direction(dim, vals[base + dim + SPACE_DIM]);
    }
    domains[i] = RD(lo, hi);
  }
  return domains;
}
