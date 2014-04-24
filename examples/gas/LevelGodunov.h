#pragma once

/**
 * LevelGodunov implements Godunov method at one level.
 *
 * @version  7 Aug 2000
 * @author   Peter McCorquodale, PWMcCorquodale@lbl.gov
 */

#include <cmath>
#include "BoxArray.h"
#include "FluxRegister.h"
#include "LevelArray.h"
#include "LevequeCorrection.h"
#include "Logger.h"
#include "PhysBC.h"
#include "PWLFillPatch.h"

class LevelGodunov {
private:
  /*
    constants
  */

  enum { DENSITY=0, VELOCITY_X=1, VELOCITY_Y=2, PRESSURE=3 };

  /** relative tolerance used in Riemann solver */
  static double RELATIVE_TOLERANCE;

  static double GAMMA;

  /** threshold for density */
  static double MIN_DENSITY;

  /** threshold for pressure */
  static double MIN_PRESSURE;

  /** coefficient of artificial viscosity */
  static double ART_VISC_COEFF;

  /*
    fields that are arguments to constructor
  */

  /** array of boxes at this level and at the next coarser level */
  BoxArray ba, baCoarse;

  /** physical domain */
  rectdomain<SPACE_DIM> dProblem;

  /** refinement ratio between this level and next coarser level */
  int nRefine;

  /** mesh spacing */
  double dx;

  PhysBC *bc;

  /*
    other fields that are set in constructor
  */

  int innerRadius, outerRadius, flapSize;

  int nCons, nPrim, nSlopes, nFluxes;

  double dRefine;

  /** edges at ends of physical domain in each dimension */
  ndarray<rectdomain<SPACE_DIM>, 1> dProblemEdges;

  /**
   * dProblemAbutters[dim1][dim2][dir] contains the edges in dimension dim2
   * that abut the face dir*dim1 of the physical domain.
   * indexed by [Util::DIMENSIONS][Util::DIMENSIONS][Util::DIRECTIONS]
   */
  ndarray<ndarray<ndarray<rectdomain<SPACE_DIM>, 1>, 1>,
          1> dProblemAbutters;

  PWLFillPatch fp;

  LevequeCorrection lc;

public:
  /**
   * constructor
   */
  LevelGodunov(BoxArray Ba, BoxArray BaCoarse,
               rectdomain<SPACE_DIM> DProblem, int NRefine,
               double Dx, PhysBC *BC);

  /*
    methods
  */

  void stepSymmetric(LevelArray<Quantities> &U,
                     LevelArray<Quantities> &UCOld,
                     LevelArray<Quantities> &UCNew,
                     double Time, double TCOld, double TCNew,
                     double Dt);

  /**
   * One step of symmetrized Godunov method, from Time to Time + Dt,
   * TCOld <= Time < Time + Dt <= TCNew
   * Called from HSCLLevel.tag()
   *
   * @param U (cell) conserved variables at this level, input at Time.
   * @param UCOld (cell) conserved variables, next coarser level, at TCOld
   * @param UCNew (cell) conserved variables, next coarser level, at TCNew
   * @param dUdt (cell) estimate of dU/dt over the interval
   */
  void stepSymmetricDiff(LevelArray<Quantities> &dUdt,
                         LevelArray<Quantities> &U,
                         LevelArray<Quantities> &UCOld,
                         LevelArray<Quantities> &UCNew,
                         double Time, double TCOld, double TCNew,
                         double Dt);

  /**
   * One step of Godunov method, from Time to Time + Dt.
   * TCOld <= Time < Time + Dt <= TCNew
   * Called from HSCLLevel.advance()
   *
   * @param U (cell) conserved variables at this level;
   *                 input at Time, output at Time + Dt.
   * @param UCOld (cell) conserved variables, next coarser level, at TCOld
   * @param UCNew (cell) conserved variables, next coarser level, at TCNew
   * @param Fr flux register between this level and next coarser level; modified
   * @param FrFine flux register between this level and next finer level; modified
   * @param EvenParity true if dimension 1 first, false if dimension 1 last
   */
  double step(LevelArray<Quantities> &U,
              LevelArray<Quantities> &UCOld,
              LevelArray<Quantities> &UCNew,
              double Time, double TCOld, double TCNew, double Dt,
              FluxRegister *Fr, FluxRegister *FrFine,
              bool EvenParity, bool doFourth, bool doFlattening);

  /**
   * Get coefficients of artificial viscosity.
   */
  void artVisc(ndarray<Quantities, SPACE_DIM> state,
               ndarray<double, SPACE_DIM, global> difCoefs,
               int dimVisc);

  void consToPrim(ndarray<Quantities, SPACE_DIM, global> state,
                  ndarray<Quantities, SPACE_DIM, global> q) {
    foreach (p, q.domain())
      q[p] = consToPrim(state[p]);
  }

  Quantities consToPrim(Quantities U) {
    double density = fmax(U[0], MIN_DENSITY);
    double vX = U[1] / density;
    double vY = U[2] / density;

    double pressure =
      fmax(MIN_PRESSURE,
           (GAMMA - 1.0) *
           (U[3] - density * 0.5 * (vX * vX + vY * vY)));
    return Quantities(density, vX, vY, pressure);
  }

  void primToCons(ndarray<Quantities, SPACE_DIM> &qarr,
                  ndarray<Quantities, SPACE_DIM> &U) {
    foreach (p, U.domain())
      U[p] = primToCons(qarr[p]);
  }

  Quantities primToCons(Quantities q) {
    double rho = fmax(q[DENSITY], MIN_DENSITY);
    return Quantities(rho, rho * q[VELOCITY_X], rho * q[VELOCITY_Y],
                      q[PRESSURE] / (GAMMA - 1.0) + rho * 0.5 *
                      (q[VELOCITY_X]*q[VELOCITY_X] +
                       q[VELOCITY_Y]*q[VELOCITY_Y]));
  }

private:
  void slope(ndarray<Quantities, SPACE_DIM> &q,
             ndarray<Quantities, SPACE_DIM> &dq,
             int sweepDim, bool doFourth, bool doFlatten);

  void flatten(ndarray<Quantities, SPACE_DIM> &q,
               ndarray<Quantities, SPACE_DIM> &dq,
               int sweepDim);

  /**
   * Given q and dq, return qlo and qhi.
   * dq, qlo, qhi are all defined on slopeBox.
   */
  void normalPred(ndarray<Quantities, SPACE_DIM> &q,
                  ndarray<Quantities, SPACE_DIM> &dq,
                  ndarray<Quantities, SPACE_DIM> &qlo,
                  ndarray<Quantities, SPACE_DIM> &qhi,
                  double DtDx, int sweepDim);

  static Quantities eigs(int sdim, Quantities q, Quantities dq,
                         double c) {
    int dim = (sdim > 0) ? sdim : -sdim;

    int VELOCITY_N = velocityIndex(dim);

    double sdensity = (sdim > 0) ? q[DENSITY] : -q[DENSITY];
    double dvN = dq[VELOCITY_N];
    double dp = dq[PRESSURE];
    Quantities eigDensity(DENSITY, (dvN * sdensity + dp / c) / c);
    Quantities eigVelocity(VELOCITY_N, dvN + dp / (sdensity * c));
    Quantities eigPressure(PRESSURE, dvN * sdensity * c + dp);
    return (eigDensity + eigVelocity + eigPressure);
  }

  /**
   * Solve Riemann problems, and store results in qGdnv.
   */
  void riemann(ndarray<Quantities, SPACE_DIM> qleft,
               ndarray<Quantities, SPACE_DIM> qright,
               ndarray<Quantities, SPACE_DIM> qGdnv,
               int sweepDim);

  /**
   * Calculate flux.
   * @param qGdnv (edge) primitive variables
   * @param state (cell) conserved variables
   * @param difCoefs (edge) artificial viscosity
   * @param flux (edge) flux, output
   */
  void fluxCalc(ndarray<Quantities, SPACE_DIM> qGdnv,
                ndarray<Quantities, SPACE_DIM> state,
                ndarray<double, SPACE_DIM, global> difCoefs,
                ndarray<Quantities, SPACE_DIM> flux,
                int sweepDim);

public:
  Quantities interiorFlux(Quantities qedge, int sweepDim) {
    int VELOCITY_N = velocityIndex(sweepDim);
    int VELOCITY_T = velocityOther(sweepDim);

    double rho = qedge[DENSITY];
    double vN = qedge[VELOCITY_N];
    double vT = qedge[VELOCITY_T];
    double p = qedge[PRESSURE];
    double bigterm =
      p / (1.0 - 1.0 / GAMMA) + rho * 0.5 * (vN*vN + vT*vT);
    return Quantities(DENSITY, rho * vN,
                      VELOCITY_N, rho * vN * vN + p,
                      VELOCITY_T, rho * vN * vT,
                      PRESSURE, vN * bigterm);
  }

  Quantities solidBndryFlux(Quantities qedge, int signdim) {
    int dim = (signdim > 0) ? signdim : -signdim;

    int VELOCITY_N = velocityIndex(dim);

    double dir = (signdim > 0 ) ? 1.0 : -1.0;
    double pstar = qedge[PRESSURE] +
      dir * qedge[DENSITY] * soundspeed(qedge) * qedge[VELOCITY_N];
    return Quantities(VELOCITY_N, pstar);
  }

private:
  /**
   * Perform conservative update of state using flux.
   */
  void update(ndarray<Quantities, SPACE_DIM> flux,
              ndarray<Quantities, SPACE_DIM, global> state,
              double DtDx, int sweepDim) {
    // println("Updating " << state.domain() << " from " <<
    // flux.domain() << " with dt/dx = " << DtDx);
    point<SPACE_DIM> nextEdge = Util::cell2edge[+sweepDim];
    point<SPACE_DIM> prevEdge = Util::cell2edge[-sweepDim];
    foreach (p, state.domain()) {
      Quantities fluxDiff = flux[p + nextEdge] - flux[p + prevEdge];
      state[p] = state[p] - fluxDiff * DtDx;
    }
  }

  /**
   * If q contains primitive variables, then
   * q[velocityIndex(dim)] is velocity in dimension dim.
   */
  static int velocityIndex(int dim) {
    return dim;
  }

  static int velocityIndex(point<1> pdim) {
    return pdim[1];
  }

  static int velocityOther(int dim) {
    return (3 - dim);
  }

  /**
   * If q contains primitive variables, then
   * soundspeed2(q) returns squared speed of sound.
   */
  static double soundspeed2(Quantities q) {
    return (GAMMA * fabs(q[PRESSURE] / q[DENSITY]));
  }


  /**
   * If q contains primitive variables, then
   * soundspeed(q) returns speed of sound.
   */
  static double soundspeed(Quantities q) {
    return sqrt(soundspeed2(q));
  }

public:
  /**
   * Return maximum possible timestep.
   */
  double estTimeStep(LevelArray<Quantities> &U) {
    double maxv = 0.0;  // local
    int myProc = MYTHREAD;
    foreach (lba, ba.procboxes(myProc))
      foreach (p, ba[lba]) {
      Quantities qpt = consToPrim(U[lba][p]);
      double c = soundspeed(qpt);
      // Doesn't use constants VELOCITY_X, VELOCITY_Y to retrieve velocities.
      foreach (pdim, Util::DIMENSIONS)
        maxv = fmax(maxv, fabs(qpt[pdim[1]]) + c);
    }
    return (dx / reduce::max(maxv));
  }


  /**
   * Fill UNew by copying from UTemp and interpolating from UTempCoarse.
   * Use PWLFillPatch object fp.
   *
   * @param UNew (cell) new patches at this level, to be filled
   * @param UTemp (cell) old patches at this level
   * @param UTempCoarse (cell) old patches at next coarser level
   */
  void regrid(LevelArray<Quantities> &UNew,
              LevelArray<Quantities> &UTemp,
              LevelArray<Quantities> &UTempCoarse) {
    fp.regrid(UNew, UTemp, UTempCoarse);
  }
};
