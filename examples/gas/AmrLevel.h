#pragma once

/**
 * AmrLevel functions both as a container for state data on a level
 * and also manages the advancement of data in time.
 *
 * @version  10 Feb 2000
 * @author   Peter McCorquodale, PWMcCorquodale@lbl.gov
 */

#include "globals.h"
#include "BoxArray.h"
#include "FluxRegister.h"
#include "InitialPatch.h"
#include "LevelArray.h"
#include "MyInputStream.h"
#include "Util.h"

class Amr;

class AmrLevel {
protected:
  /** process used for broadcasts */
  enum { SPECIAL_PROC = 0 };

  /*
    fields that are arguments to constructor
  */

  /** parent Amr object. */
  Amr *parent;

  /** height of this AmrLevel. */
  int level;

  /** arrays of boxes for this level and next coarser level. */
  BoxArray ba, baCoarse;

  /** physical domain in coordinates at this level. */
  rectdomain<SPACE_DIM> dProblem;

  /** refinement ratio between this level and next coarser level. */
  int nRefineCoarser;

  /** grid spacing. */
  double dx;

  /*
    other fields that are set in constructor
  */

  /** reference to AmrLevel at next coarser level. */
  AmrLevel *amrlevel_coarser;

  /** refinement ratio as point. */
  point<SPACE_DIM> nRefineCoarserP;

  /** are we on the initial timestep? */
  bool initial_step;

  double oldTime;
  double newTime;

  /** LevelArray<Quantities>'s of state data at old time and at new time. */
  LevelArray<Quantities> UOld, UNew, UInter;

  LevelArray<double> truncErrorCopied, truncErrorCopied2;

  /** FluxRegister for interface with the coarser level. */
  FluxRegister *fReg;

  /** these are NOT */
  int myProc;
  domain<1> myBoxes;

public:
  /**
   * constructor
   */
  AmrLevel(Amr *Parent, int Level, BoxArray &Ba, BoxArray &BaCoarse,
           rectdomain<SPACE_DIM> DProblem, int NRefineCoarser,
           double Dx) :
    initial_step(true), oldTime(0.0), newTime(0.0) {
    // println("constructing AmrLevel");
    parent = Parent;
    level = Level;
    ba = Ba;  // Ba lives in the region assigned to the next coarser level
    myProc = MYTHREAD;
    myBoxes = ba.procboxes(myProc);
    baCoarse = BaCoarse;
    nRefineCoarser = NRefineCoarser;
    nRefineCoarserP = point<SPACE_DIM>::all(nRefineCoarser);
    dx = Dx;
    dProblem = DProblem;

    amrlevel_coarser = getLevel(level-1);

    // Allocate states.
    UOld = LevelArray<Quantities>(ba);
    UNew = LevelArray<Quantities>(ba);

    // fReg is for interface between this level and next coarser level.
    fReg = ((bool) baCoarse.isNull()) ? NULL :
      new FluxRegister(ba, baCoarse, dProblem, nRefineCoarser);
  }

  /*
    accessor methods
  */

  bool isInitialStep() { return initial_step; }

  AmrLevel *getLevel(int lev);

  BoxArray getBa() { return ba; }

  LevelArray<Quantities> getUNew() { return UNew; }

  LevelArray<Quantities> getUOld() { return UOld; }

  LevelArray<Quantities> getUInter() { return UInter; }

  FluxRegister *getfReg() { return fReg; }

  double getOldTime() { return oldTime; }

  double getNewTime() { return newTime; }

  double getDx() { return dx; }

  rectdomain<SPACE_DIM> getDproblem() { return dProblem; }

  LevelArray<double> getTruncErrorCopied() { return truncErrorCopied; }

  LevelArray<double> getTruncErrorCopied2() { return truncErrorCopied2; }

  /*
    methods
  */

  /**
   * Read in saved plotfile.  This is incomplete.
   */
  virtual void getSaved(MyInputStream &in) = 0;

  void initData(InitialPatch &initializer) {
    foreach (lba, myBoxes) {
      // initializer.initData(ba[lba], UOld[lba], dx);
      initializer.initData(ba[lba], UNew[lba], dx);
    }
  }

  void post_initial_step() { initial_step = false; }

protected:
  /**
   * Average solution at this level to get new solution (UNew) at
   * next coarser level, on all coarse cells covered by this level.
   * Requires level > 0.
   * Called by post_timestep().
   */
  void avgDown() {
    LevelArray<Quantities> UC = amrlevel_coarser->getUNew();
    foreach (lba, myBoxes)
      foreach (lbaC, baCoarse.indices()) {
      rectdomain<SPACE_DIM> intersect =
        (ba[lba] / nRefineCoarserP) * baCoarse[lbaC];
      foreach (cellC, intersect) {
        // UC[lbaC] may live on another process.
        UC[lbaC][cellC] =
          Quantities::Average(UNew[lba].
                              constrict(Util::subcells(cellC,
                                                       nRefineCoarser)));
      }
    }
  }

public:
  /**
   * Compute estimated coarsest-level timestep for next iteration
   * -- must be defined in derived class.
   * Called by Amr.computeNewDt().
   *
   * @return timestep
   */
  virtual double estTimeStep() = 0;


  /**
   * Operations to be done after a timestep (like refluxing)
   * -- must be defined in derived class.
   * Called by Amr.timeStep().
   */
  virtual void post_timestep() = 0;


  /**
   * Advance by a timestep at this level.
   * Called by Amr.timeStep().
   *
   * @param time physical time before advancement
   * @param dt physical timestep
   * @return CFL number
   */
  virtual double advance(double time, double dt) = 0;

protected:
  /**
   * swap time levels (called by advance())
   */
  void swapTimeLevels(double dt) {
    oldTime = newTime;
    newTime = oldTime + dt;

    UOld.copy(UNew);
  }

public:
  /**
   * Tag cells at this level.
   */
  virtual domain<SPACE_DIM> tag() = 0;

  /**
   * Return a copy of UNew on the original grids.
   */
  virtual LevelArray<Quantities>
    regrid(BoxArray &Ba, const BoxArray &BaCoarse,
           LevelArray<Quantities> &UTempCoarse) = 0;

  virtual BoxArray
  assignBoxes(ndarray<rectdomain<SPACE_DIM>, 1> &boxes,
              rectdomain<SPACE_DIM> dProblemLevel) = 0;

  virtual void initGrids(InitialPatch &initializer) = 0;

  virtual void write(const string &outputfile) = 0;

  virtual void writeOn(ostream &out,
                       ndarray<Quantities, SPACE_DIM, global> &gridData,
                       int height) = 0;

public:
  virtual void writeError(const string &outputfile) = 0;

  virtual void writeErrorOn(ostream &out,
                            ndarray<double, SPACE_DIM, global> &gridData,
                            int height) = 0;
};
