#pragma once

/**
 * int [1d] proclist = LoadBalance.balance(int [1d] load, int numProcs)
 *
 * Given task loads and the number of processes, assign tasks to processes.
 *
 * Input
 * int [1d] load:  loads of some tasks.
 * int numProcs:  number of processes to which load must be distributed.
 *
 * Returns:
 * int [1d] proclist:  same domain as load, range [0:numProcs-1];
 * proclist[i] gives the ID number of the process to which the i'th
 * task is assigned.
 *
 * This algorithm comes from
 * William Y. Crutchfield, ``Load Balancing Irregular Algorithms''
 *
 * @version  15 Feb 2000
 * @author   Peter McCorquodale, PWMcCorquodale@lbl.gov
 */

#include "globals.h"

class LoadBalance {
public:
  static ndarray<int, 1> balance(ndarray<int, 1> &load,
                                 int numProcs);
};
