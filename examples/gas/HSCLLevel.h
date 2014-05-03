#pragma once

/**
 * HSCLLevel is for hyperbolic systems.
 *
 * @version  25 Aug 2000
 * @author   Peter McCorquodale, PWMcCorquodale@lbl.gov
 */

#include <sstream>
#include "Amr.h"
#include "AmrLevel.h"
#include "LevelArray.h"
#include "LevelGodunov.h"
#include "Logger.h"
#include "MyInputStream.h"
#include "PhysBC.h"
#include "Util.h"

class HSCLLevel : public AmrLevel {
private:
  /*
    constants
  */

  enum { SPECIAL_PROC = 0 };

public:
  static point<SPACE_DIM> TWO;

private:
  /*
    fields that are arguments to constructor
  */

  /** physical boundary conditions */
  PhysBC *bc;

  /*
    other fields that are set in constructor
  */

  /** maximum Courant-Friedrichs-Levy number */
  double cflmax;

  /** conserved variable to use for gradient to decide tagging */
  int tagvar;

  /** whether to use gradient to tag cells */
  bool useGradient;

  /** whether to adjust at interfaces with coarser level */
  bool useNoCoarseInterface;

  /** whether to adjust at interfaces with finer level */
  bool useNoFineInterface;

  /** minimum relative difference to tag a cell when using gradient method */
  double minRelDiff;

  /** minimum relative difference in truncation error to tag a cell */
  double minRelTrunc;

  LevelGodunov *lg;

  /*
    other fields
  */

  bool evenParity;

  /** perimeter of grids at this level */
  int coarsePerimeter;

  /** perimeter of grids at next finer level */
  int finePerimeter;

  domain<SPACE_DIM> coarseInterface;

  domain<SPACE_DIM> coarseInterfaceOff;

  domain<SPACE_DIM> fineInterface;

  HSCLLevel *finerLevel;

public:
  /**
   * constructor
   */
  HSCLLevel(Amr *Parent, int Level, BoxArray Ba, BoxArray BaCoarse,
            rectdomain<SPACE_DIM> DProblem, int NRefineCoarser,
            double Dx, PhysBC *BC);

  ~HSCLLevel() {
    delete lg;
  }

private:
  /*
    accessor methods
  */

  int getCoarsePerimeter() {
    return coarsePerimeter;
  }

  domain<SPACE_DIM> getCoarseInterfaceOff() {
    return coarseInterfaceOff;
  }

  /*
    methods
  */

  void setFinerLevel(HSCLLevel *newLevel) {
    finerLevel = newLevel;
  }

  static domain<SPACE_DIM> computeUnion(domain<SPACE_DIM> &unionLocal);

  /**
   * Find coarsePerimeter and coarseInterface.
   */
  void computePerimeter();

public:
  virtual void getSaved(MyInputStream &in);

  double estTimeStep() {
    double lgest = lg->estTimeStep(UNew);
    // println("Level " << level << " max v = " << (dx/lgest) +
    // " estimated timestep:  " << (cflmax*lgest));
    return (cflmax * lgest);
  }

  /**
   * Use new solution at this level to update solution at coarser level.
   */
  void post_timestep() {
    initial_step = false;
    if (level > 0) {
      Logger::barrier();
      Logger::append("reflux");
      // println("Trying reflux at level " << level);
      reflux();
      Logger::barrier();
      Logger::append("avgDown");
      avgDown();
      Logger::barrier();
      Logger::append("end post_timestep");
    }
  }

  double advance(double Time, double dt);

private:
  /**
   * Update the coarser level using flux registers living on this level.
   * Called from post_timestep().
   */
  void reflux() {
    LevelArray<Quantities> laq = amrlevel_coarser->getUNew();
    fReg->reflux(laq);
  }

public:
  /**
   * Tag cells at this level.
   *
   * Called by:
   * initGrids() for initialization
   * Amr.regrid() for regridding
   * Amr.getInitialBoxes() for initialization -- obsolete
   */
  domain<SPACE_DIM> tag();

private:
  /**
   * Tag cells in a particular box at this level, using gradient.
   */
  domain<SPACE_DIM> tagBoxGradient(ndarray<Quantities, SPACE_DIM,
                                   global> U,
                                   rectdomain<SPACE_DIM> box);

public:
  /**
   * initGrids() is called from the HSCLLevel object at level 0 only.
   * It creates the HSCLLevel objects for all higher levels,
   * and calls the routines to fill them in.
   *
   * In main program:
   * amrAll = new Amr();
   * ba0 = amrAll->baseBoxes();
   * HSCLLevel *level0 = new HSCLLevel(amrAll, 0, ba0, ...);
   * amrAll->setLevel(0, level0);
   * level0->initGrids();
   */
  void initGrids(InitialPatch &initializer);

  /**
   * Assign boxes to procs.
   * The assignment is for the next finer level, not this one.
   */
  BoxArray assignBoxes(ndarray<rectdomain<SPACE_DIM>, 1> &boxes,
                       rectdomain<SPACE_DIM> dProblemLevel);

  /**
   * Return a copy of UNew on the original grids.
   * Called from Amr.regrid().
   *
   * @param Ba new BoxArray at this level
   * @param BaCoarse new BoxArray at next coarser level
   * @param UTempCoarse (cell) old patches at next coarser level
   * @return (cell) old patches at this level
   */
  LevelArray<Quantities> regrid(BoxArray &Ba,
                                const BoxArray &BaCoarse,
                                LevelArray<Quantities> &UTempCoarse);

  /*
    output routines
  */

  /**
   * Write the whole hierarchy of levels to a file.
   * Call from level 0.
   */
  void write(const string &outputfile);

protected:
  /**
   * Write out data for a particular grid.
   */
  void writeOn(ostream &out,
               ndarray<Quantities, SPACE_DIM, global> &gridData,
               int height);

private:
  static string domainstring(rectdomain<SPACE_DIM> D) {
    ostringstream oss;
    oss << '(' << pointstring(D.min()) <<
      ' ' << pointstring(D.max()) <<
      ' ' << pointstring(Util::ZERO) << ')';
    return oss.str();
  }

  static string domainstringShort(rectdomain<SPACE_DIM> D) {
    // format [0:127, 0:31]
    point<SPACE_DIM> Dmin = D.min(), Dmax = D.max();
    ostringstream oss;
    oss << "[" << Dmin[1] << ":" << Dmax[1];
    for (int dim = 2; dim <= SPACE_DIM; dim++) {
      oss << ", " << Dmin[dim] << ':' << Dmax[dim];
    }
    oss << ']';
    return oss.str();
  }

  static string pointstring(point<SPACE_DIM> p) {
    ostringstream oss;
    oss << "(" << p[1];
    for (int dim = 2; dim <= p.arity; dim++) {
      oss << "," << p[dim];
    }
    oss << ')';
    return oss.str();
  }

  static string doubleArraystring(ndarray<double, 1> &arr) {
    ostringstream oss;
    oss << arr[1];
    for (int dim = 2; dim <= arr.domain().size(); dim++) {
      oss << " " << arr[dim];
    }
    return oss.str();
  }

  static rectdomain<SPACE_DIM> extentDomain(point<SPACE_DIM> ext) {
    return RD(Util::ZERO, ext);
  }

public:
  /**
   * Write the whole hierarchy of levels to a file.
   * Call from level 0.
   */
  void writeError(const string &outputfile);

protected:
  /**
   * Write out data for a particular grid.
   */
  void writeErrorOn(ostream &out,
                    ndarray<double, SPACE_DIM, global> &errorData,
                    int height);
};
