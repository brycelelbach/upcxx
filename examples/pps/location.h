#pragma once

#include <cmath>

/** This is a simple class to hold a 2-D location. */
struct location {
  double x;
  double y;

  location() : x(0), y(0) {}

  location(double xi, double yi) : x(xi), y(yi) {}

  double distance(location p) {
    return sqrt((x - p.x)*(x - p.x)
                + (y - p.y)*(y - p.y));
  }
};
