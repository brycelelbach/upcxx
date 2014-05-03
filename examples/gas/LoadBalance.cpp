#include <cstdlib>
#include <sstream>
#include "LoadBalance.h"
#include "ParmParse.h"

ndarray<int, 1> LoadBalance::balance(ndarray<int, 1> &load,
                                     int numProcs) {
  rectdomain<1> procTeam(0, numProcs);

  rectdomain<1> taskindices = load.domain();
  // proclist is what is returned
  ndarray<int, 1> proclist(taskindices);
  if (taskindices.size() == 0) {
    println("LoadBalance:  no tasks");
    return proclist;
  }

  ndarray<domain<1>, 1> procTasks(procTeam);

  // procLoad[proc] is total load assigned to process proc.
  ndarray<int, 1> procLoad(procTeam);
  foreach (proc, procTeam) procLoad[proc] = 0;

  // foreach (task, taskindices)
  //   println("Task " << task << " has load " << load[task]);

  /*
    Figure out how to allocate to processes.
  */

  domain<1> unassigned = taskindices;
  // procTasks[proc] containing ind
  // means that tasks[ind] is assigned to process proc.
  foreach (proc, procTeam)
    procTasks[proc] = RD(0, 0);

  // Find heaviest task and assign it to the lightest process.
  // println("find heaviest task");
  while (! unassigned.is_empty()) {
    // find heaviest task among those that are as yet unassigned
    int maxloadTask = 0;
    point<1> heaviestTask;
    foreach (taskp, unassigned) {
      if (load[taskp] > maxloadTask) {
        heaviestTask = taskp;
        maxloadTask = load[taskp];
      }
    }

    if (maxloadTask == 0) {
      println("LoadBalance:  no task with positive load");
      exit(0);
    }

    // find lightest process
    int lightestProc = 0;
    foreach (proc, procTeam)
      if (procLoad[proc] < procLoad[lightestProc])
        lightestProc = proc[1];

    // assign heaviest task to lightest process
    domain<1> heaviestTaskD = RD(heaviestTask, heaviestTask+1);
    procTasks[lightestProc] = procTasks[lightestProc] + heaviestTaskD;
    procLoad[lightestProc] += load[heaviestTask];
    unassigned = unassigned - heaviestTaskD;
  }

  // println("now finding heaviest");

  bool unbalanced = true;
  while (unbalanced) {
    unbalanced = false;

    // procTasksPrint(procTasks);

    // find heaviest process
    point<1> heaviestProc = PT(0);
    foreach (proc, procTeam)
      if (procLoad[proc] > procLoad[heaviestProc])
        heaviestProc = proc;

    // for each task in heaviest process
    foreach (taskph, procTasks[heaviestProc]) {
      int wth = load[taskph];

      // for each task NOT in heaviest process
      // foreach (otherProc, procTeam - [heaviestProc:heaviestProc]) {
      foreach (otherProc, procTeam) {
        if (otherProc != heaviestProc) {
          domain<1> &procTasksOther = procTasks[otherProc];
          foreach (taskpo, procTasksOther) {

            int wto = load[taskpo];  // weight of the other task
            int loadhnew = procLoad[heaviestProc] - wth + wto;
            int loadonew = procLoad[otherProc] - wto + wth;

            // if interchanging taskph and taskpo improves load balance
            // (i.e., difference in process loads goes down)
            if (abs(loadhnew - loadonew) <
                procLoad[heaviestProc] - procLoad[otherProc]) {

              // println("interchanging " << taskph[1] +
              // " in [" << heaviestProc << "] with " <<
              //                 taskpo[1] << " in " << otherProc);

              // perform interchange
              procTasks[heaviestProc] =
                procTasks[heaviestProc] - RD(taskph, taskph+1) +
                RD(taskpo, taskpo);
              procLoad[heaviestProc] = loadhnew;
              procTasks[otherProc] =
                procTasks[otherProc] - RD(taskpo, taskpo+1) +
                RD(taskph, taskph+1);
              procLoad[otherProc] = loadonew;
              // find heaviest process again...
              unbalanced = true;
              goto findswaps;
            }
          }
        }
      }
    }
  findswaps:;
  } // while (unbalanced)

  /*
    If input contains a line -balance.print,
    then print out loads.
  */

  ParmParse pp("balance");
  if (pp.contains("print")) {
    // print out loads
    ostringstream oss;
    oss << "Loads:";
    for (int proc = 0; proc < numProcs; proc++) {
      int numTasks = procTasks[proc].size();
      oss << " " << procLoad[proc] << " (" << numTasks << " task"
          << ((numTasks == 1) ? "" : "s") << ")";
    }
    println(oss.str());
  }

  /*
    Now assign tasks to processes.
  */

  // rectdomain<1> taskindices = tasks.domain();
  foreach (proc, procTeam)
    foreach (indtask, procTasks[proc]) {
    proclist[indtask] = proc[1];
    // println("Task " << indtask << " assigned to " << proc[1]);
  }

  procTasks.destroy();
  procLoad.destroy();
  return proclist;
}
