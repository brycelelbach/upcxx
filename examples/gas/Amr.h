#pragma once

/**
 * The Amr class is designed to manage parts of the computation
 * which do not belong on a single level, like establishing and updating
 * the hierarchy of levels, global timestepping, and managing the different
 * AmrLevels.
 *
 * @version  1 Sep 2000
 * @author   Peter McCorquodale, PWMcCorquodale@lbl.gov
 */

#include "AmrLevel.h"
#include "BoxArray.h"
#include "LevelArray.h"
#include "Logger.h"
#include "MyInputStream.h"
#include "ParmParse.h"
#include "Quantities.h"
#include "Util.h"

class Amr {
private:
  /** process used for broadcasts */
  enum { SPECIAL_PROC = 0 };

  /*
    fields that are set in constructor
  */

  /** dimensions of coarsest-level box */
  point<SPACE_DIM> extent;

  /** current finest level */
  int finest_level;

  /** number of iterations between regriddings */
  int regrid_int;

  /** number of iterations between writes to plotfiles */
  int plot_int;

  /**
   * array of refinement ratios [0:finest_level-1];
   * This is the ratio between this level and the next FINER level.
   * dt_level[k] = dt_level[k-1]/REAL(ref_ratio[k-1]);
   */
  ndarray<int, 1> ref_ratio;

  /** coordinates of the box ends */
  ndarray<ndarray<double, 1>, 1> ends; // [Util::DIRECTIONS][Util::DIMENSIONS]

  ndarray<AmrLevel *, 1> amr_level;

  /** physical time step at each level */
  ndarray<double, 1> dt_level;

  /** grid spacing at each level */
  ndarray<double, 1> dx_level;

  /** physical domain at each level */
  ndarray<rectdomain<SPACE_DIM>, 1> dProblem;

  /** number of steps made at each level */
  ndarray<int, 1> level_steps;

  string outputFilePrefix;

  /** maximum number of coarse-level steps */
  double maxSteps;

  /** maximum physical time to run the simulation */
  double maxCumtime;

  bool printElapsed, printAdvance;

  ParmParse *pp;

  /*
    other fields
  */

  bool saved;

  /** physical time variables.  initialized in getSaved() or run(). */
  double cumtime;
  double finetime;

public:
  /**
   * constructor
   */
  Amr();

  /*
    accessor methods
  */

  AmrLevel *getLevel(int lev) {
    if (0 <= lev && lev <= finest_level)
      return amr_level[lev];
    else
      return NULL;
  }

  AmrLevel *getLevel(point<1> lev) {
    return getLevel(lev[1]);
  }

  ndarray<ndarray<double, 1>, 1> getEnds() {
    return ends;
  }

  ndarray<ndarray<double, 1>, 1> getEndsNonsingle() {
    ndarray<ndarray<double, 1>, 1> endsNonsingle(Util::DIRECTIONS);
    foreach (pdir, Util::DIRECTIONS) {
      endsNonsingle[pdir].create(Util::DIMENSIONS);
      foreach (pdim, Util::DIMENSIONS) {
        endsNonsingle[pdir][pdim] = ends[pdir][pdim];
      }
    }
    return endsNonsingle;
  }

  AmrLevel *getLevelNonsingle(int lev) {
    if (0 <= lev && lev <= finest_level)
      return amr_level[lev];
    else
      return NULL;
  }

  AmrLevel *getLevelNonsingle(point<1> lev) {
    return getLevelNonsingle(lev[1]);
  }

  void setLevel(int lev, AmrLevel *amrlev) {
    amr_level[lev] = amrlev;
  }

  point<SPACE_DIM> getExtent() {
    return extent;
  }

  int getLevelSteps(int lev) {
    if (0 <= lev && lev <= finest_level) {
      return level_steps[lev];
    } else {
      println("Amr:  Trying to get steps for level " << lev);
      return 0;
    }
  }

  int getLevelSteps(point<1> lev) {
    return getLevelSteps(lev[1]);
  }

  int finestLevel() {
    return finest_level;
  }

  double getCumtime() {
    return cumtime;
  }

  double getFinetime() {
    return finetime;
  }

  int getRefRatio(int lev) {
    if (lev >= ref_ratio.domain().size()) {
      println("Amr: No refinement ratio given up to level " << lev);
      exit(0);
    }
    return ref_ratio[lev];
  }

  int getRefRatio(point<1> lev) {
    return getRefRatio(lev[1]);
  }

  double getDx(int lev) {
    return dx_level[lev];
  }

  double getDx(point<1> lev) {
    return dx_level[lev];
  }

  double getDt(int lev) {
    return dt_level[lev];
  }

  double getDt(point<1> lev) {
    return dt_level[lev];
  }

  rectdomain<SPACE_DIM> getDomain(int lev) {
    return dProblem[lev];
  }

  rectdomain<SPACE_DIM> getDomain(point<1> lev) {
    return dProblem[lev];
  }

  /*
    methods
  */

  /**
   * Read data from saved plotfile.
   * (1) Number of states (int)
   * (2) Name of state (aString)
   * (3) Number of dimensions (int)
   * (4) Time (REAL)
   * (5) Finest level written (int)
   * (6) Loside prob domain loc in each direction (BL_SPACEDIM*(REAL*))
   * (7) Hiside prob domain loc in each direction (BL_SPACEDIM*(REAL*))
   * (8) Refinement ratio ((numLevels-1)*(int))
   * (9) Domain of each level (numLevels* (BOX) )
   * (10) Steps taken on each level (numLevels*(int) )
   * (11) Grid spacing (numLevels*(BL_SPACEDIM*(int))) [\n at each level]
   * (12) Coordinate flag (int)
   * (13) Boundary info (int)
   */
  void getSaved(MyInputStream &in);

  /**
   * Get initial BoxArray at level 0.
   * Each process has one box at level 0.  Fill them in later.
   */
  BoxArray baseBoxes();

  /**
   * Advance a level in time using the level operator.
   *
   * @param level which level
   * @param time physical time before advancement
   */
  void timeStep(int level, double time);

  void run();

private:
  /**
   * Make a time step on the coarsest grid.
   * Called by run().
   */
  void coarseTimeStep() {
    checkForRegrid();

    // if (level_steps[0] > 0)
    Logger::append("computeNewDt");
    computeNewDt();  // sets dt_level
    timeStep(0, cumtime);
    cumtime += dt_level[0];

    if (amr_level[0]->isInitialStep())
      amr_level[0]->post_initial_step();

    checkForPlot();
  }

  void checkForRegrid() {
    if ((level_steps[0] > 0) && (level_steps[0] % regrid_int == 0))
      regrid();
  }

  void checkForPlot() {
    if ((level_steps[0] > 0) && (level_steps[0] % plot_int == 0))
      write();
  }

  void checkForPlotError() {
      // if ((level_steps[0] > 0) && (level_steps[0] % plot_int == 0))
      //   writeError();
      if (level_steps[0] % plot_int == 0)
        writeError();
  }

public:
  void write() {
    if (MYTHREAD == SPECIAL_PROC) {
      ostringstream oss;
      oss << outputFilePrefix << level_steps[0];
      amr_level[0]->write(oss.str());
    }
  }

  void writeError() {
    if (MYTHREAD == SPECIAL_PROC) {
      ostringstream oss;
      oss << "err" << outputFilePrefix << level_steps[0];
      amr_level[0]->writeError(oss.str());
    }
  }

private:
  void regrid();

public:
  /**
   * We are at the end of a coarse grid timecycle.
   * Compute the timesteps for the next iteration.
   * Called by coarseTimeStep().
   */
  void computeNewDt();
};
