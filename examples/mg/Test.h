#pragma once

#include "globals.h"
#include "Grid.h"

/**
 * This class initially populates the right-hand side grid.
 */
class Test {
 public:
  /**
   * Populates the right-hand side grid with +1 at ten randomly chosen
   * points, -1 at a different ten random points, and zero elsewhere.
   */
  static void populateRHSGrid(Grid &gridA, int startLevel);
};
