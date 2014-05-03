#pragma once

#include <cmath>
#include "globals.h"

struct Quantities {
  double val0, val1, val2, val3;

  static Quantities qzero;

  /*
    constructors
  */

  Quantities() : val0(0), val1(0), val2(0), val3(0) {}

  Quantities(double a0, double a1, double a2, double a3) :
    val0(a0), val1(a1), val2(a2), val3(a3) {}

  Quantities(int i0, double a0,
             int i1, double a1,
             int i2, double a2,
             int i3, double a3) {
    /* Argh. */
    if (i0 == 0) val0 = a0;
    if (i1 == 0) val0 = a1;
    if (i2 == 0) val0 = a2;
    if (i3 == 0) val0 = a3;

    if (i0 == 1) val1 = a0;
    if (i1 == 1) val1 = a1;
    if (i2 == 1) val1 = a2;
    if (i3 == 1) val1 = a3;

    if (i0 == 2) val2 = a0;
    if (i1 == 2) val2 = a1;
    if (i2 == 2) val2 = a2;
    if (i3 == 2) val2 = a3;

    if (i0 == 3) val3 = a0;
    if (i1 == 3) val3 = a1;
    if (i2 == 3) val3 = a2;
    if (i3 == 3) val3 = a3;
  }

  Quantities(int i, double a) {
    val0 = (i == 0) ? a : 0.0;
    val1 = (i == 1) ? a : 0.0;
    val2 = (i == 2) ? a : 0.0;
    val3 = (i == 3) ? a : 0.0;
  }

  /*
    accessor methods
  */

  static int length() { return (SPACE_DIM + 2); }

  double operator[](int j) { 
    // return val[j];
    if (j == 0) return val0;
    if (j == 1) return val1;
    if (j == 2) return val2;
    if (j == 3) return val3;
    println("ERROR: asking for component " << j << " of Quantities");
    return 0.0;
  }

  /*
    arithmetic operations
  */

  Quantities operator+(Quantities v2) {
    /* this.op+(v2) same as this+v2 */
    return Quantities(val0 + v2.val0,
                      val1 + v2.val1,
                      val2 + v2.val2,
                      val3 + v2.val3);
  }

  Quantities operator-(Quantities v2) {
    /* this.op-(v2) same as this-v2 */
    return Quantities(val0 - v2.val0,
                      val1 - v2.val1,
                      val2 - v2.val2,
                      val3 - v2.val3);
  }

  Quantities operator*(Quantities v2) {
    /* this.op*(v2) same as this*v2 */
    return Quantities(val0 * v2.val0,
                      val1 * v2.val1,
                      val2 * v2.val2,
                      val3 * v2.val3);
  }

  Quantities operator*(double scalar) {
    /* this.op*(scalar) same as this*scalar */
    return Quantities(val0 * scalar,
                      val1 * scalar,
                      val2 * scalar,
                      val3 * scalar);
  }

  Quantities operator/(double scalar) {
    /* this.op/(scalar) same as this/scalar */
    return Quantities(val0 / scalar,
                      val1 / scalar,
                      val2 / scalar,
                      val3 / scalar);
  }

  /*
    arithmetic assignment operations
  */

  Quantities &operator+=(Quantities v2) {
    *this = operator+(v2);
    return *this;
  }

  Quantities &operator-=(Quantities v2) {
    *this = operator-(v2);
    return *this;
  }

  /*
    other math functions
  */

  double norm() {
    double result = val0*val0 + val1*val1 + val2*val2 + val3*val3;
    return sqrt(result);
  }

  static Quantities abs(Quantities v) {
    return Quantities(fabs(v.val0),
                      fabs(v.val1),
                      fabs(v.val2),
                      fabs(v.val3));
  }

  static Quantities sgn(Quantities v) {
    return Quantities(sgn(v.val0),
                      sgn(v.val1),
                      sgn(v.val2),
                      sgn(v.val3));
  }

  static Quantities min(Quantities v1, Quantities v2) {
    return Quantities(fmin(v1.val0, v2.val0),
                      fmin(v1.val1, v2.val1),
                      fmin(v1.val2, v2.val2),
                      fmin(v1.val3, v2.val3));
  }

  /*
    methods for slopes
  */

  static Quantities getdlim(Quantities v1, Quantities v2) {
    return Quantities(getdlim(v1.val0, v2.val0),
                      getdlim(v1.val1, v2.val1),
                      getdlim(v1.val2, v2.val2),
                      getdlim(v1.val3, v2.val3));
  }

  /*
    other methods
  */

  // Print a string representation to the given stream.
  friend std::ostream& operator<<(std::ostream& os,
                                  const Quantities& q) {
    os << "(" << q.val0 <<
      ", " << q.val1 <<
      ", " << q.val2 <<
      ", " << q.val3 << ")";
    return os;
  }

  /*
    dimension-independent methods
  */

  static Quantities zero() { return qzero; }

  /**
   * average over a RectDomain<SPACE_DIM>
   */
  static Quantities Average(const ndarray<Quantities, SPACE_DIM> &v) {
    Quantities sum = qzero;
    rectdomain<SPACE_DIM> D = v.domain();
    foreach (p, D) sum = sum + v[p];
    return (sum / (double) D.size());
  }

  static Quantities Average(const ndarray<Quantities, SPACE_DIM, global> &v) {
    Quantities sum = qzero;
    rectdomain<SPACE_DIM> D = v.domain();
    foreach (p, D) sum = sum + v[p];
    return (sum / (double) D.size());
  }

  /**
   * Calculate van Leer slope
   * @param a q[p - unit]
   * @param b q[p]
   * @param c q[p + unit]
   */
  static Quantities vanLeer(Quantities a, Quantities b, Quantities c) {
    Quantities dlim = getdlim(c - b, b - a);
    Quantities dif2 = c - a;
    return min(abs(dif2) * 0.5, dlim) * sgn(dif2);
  }

  /**
   * Calculate 4th-order slope
   * @param dq2sum dq2[p - unit] + dq2[p + unit]
   */
  static Quantities slope4(Quantities a, Quantities b, Quantities c,
                           Quantities dq2sum) {
    Quantities dlim = getdlim(c - b, b - a);
    Quantities dif2 = c - a;
    Quantities dq4 = (dif2 - (dq2sum * 0.25)) * (2.0/3.0);
    return min(abs(dq4), dlim) * sgn(dif2);
  }

  static double getdlim(double a, double b) {
    if (a * b > 0) {
      return fmin(fabs(a), fabs(b)) * 2.0;
    } else {
      return 0.0;
    }
  }

  static double sgn(double x) {
    double y;
    if (x > 0.0) {
      y = 1.0;
    } else if (x < 0.0) {
      y = -1.0;
    } else {
      y = 0.0;
    }
    return y;
  }

  /**
   * Print out error message if any component is NaN.
   */
  void checkNaN(const string &str) {
    if ((val0 != val0) || (val1 != val1) || (val2 != val2) || (val3 != val3))
      println(str << " = " << *this);
  }
};
