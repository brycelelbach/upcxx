#include "LevelGodunov.h"

double LevelGodunov::RELATIVE_TOLERANCE = 1e-8;
double LevelGodunov::GAMMA = 1.4;
double LevelGodunov::MIN_DENSITY = 1e-6;
double LevelGodunov::MIN_PRESSURE = 1e-6;
double LevelGodunov::ART_VISC_COEFF = 0.1;

LevelGodunov::LevelGodunov(BoxArray Ba, BoxArray BaCoarse,
                           rectdomain<SPACE_DIM> DProblem,
                           int NRefine, double Dx, PhysBC *BC) {
  innerRadius = 1;
  outerRadius = 3;
  flapSize = outerRadius;
  nCons = SPACE_DIM + 2;
  nFluxes = nCons;
  nPrim = SPACE_DIM + 3;
  nSlopes = nCons;
  nRefine = NRefine;
  dRefine = (double) nRefine;
  dProblem = DProblem;
  dProblemEdges.create(Util::DIMENSIONS);
  dProblemAbutters.create(Util::DIMENSIONS);
  foreach (p, dProblemAbutters.domain()) {
    dProblemAbutters[p].create(Util::DIMENSIONS);
    foreach (q, dProblemAbutters[p].domain()) {
      dProblemAbutters[p][q].create(Util::DIRECTIONS);
    }
  }

  foreach (pdim, Util::DIMENSIONS) {
    int dim = pdim[1];
    point<SPACE_DIM> lower =
      dProblem.min() + Util::cell2edge[-dim];
    point<SPACE_DIM> upper =
      dProblem.max() + Util::cell2edge[+dim];
    point<SPACE_DIM> stride =
      point<SPACE_DIM>::direction(dim, upper[dim] - lower[dim] - 1)
      + Util::ONE;
    dProblemEdges[dim] = RD(lower, upper + Util::ONE, stride);
    foreach (pdir, Util::DIRECTIONS) {
      rectdomain<SPACE_DIM> faceCells =
        dProblem.border(1, pdir[1] * dim, 0);
      // foreach (pdimOther, Util::DIMENSIONS - [pdim:pdim])
      foreach (pdimOther, Util::DIMENSIONS)
        if (pdimOther != pdim)
          dProblemAbutters[pdim][pdimOther][pdir] =
            Util::innerEdges(faceCells, pdimOther[1]);
    }
  }

  dx = Dx;
  bc = BC;
  ba = Ba;
  baCoarse = BaCoarse;

  fp = PWLFillPatch(ba, baCoarse, dProblem, nRefine, innerRadius,
                    outerRadius);

  lc = LevequeCorrection(ba, dProblem, dx, nCons);
}

void LevelGodunov::stepSymmetric(LevelArray<Quantities> &U,
                                 LevelArray<Quantities> &UCOld,
                                 LevelArray<Quantities> &UCNew,
                                 double Time, double TCOld,
                                 double TCNew, double Dt) {
  // SharedRegion tempRegion = new SharedRegion(); // REGION
  // { // scope for new objects using tempRegion
    // Copy U to UTemp

  LevelArray<Quantities> UTemp(U.boxArray());

  int myProc = MYTHREAD;
  domain<1> myBoxes = ba.procboxes(myProc);

  foreach (lba, myBoxes)
    UTemp[lba].copy(U[lba]);

  // Do step with dimension 1 first.
  step(UTemp, UCOld, UCNew, Time, TCOld, TCNew, Dt, NULL, NULL,
       true, false, false);

  // Do step with dimension 1 last.
  step(U, UCOld, UCNew, Time, TCOld, TCNew, Dt, NULL, NULL, false,
       false, false);

  // Average the two.
  foreach (lba, myBoxes) foreach (p, U[lba].domain())
    U[lba][p] = (U[lba][p] + UTemp[lba][p]) * 0.5;
  // }
  UTemp.destroy();
}

void LevelGodunov::stepSymmetricDiff(LevelArray<Quantities> &dUdt,
                                     LevelArray<Quantities> &U,
                                     LevelArray<Quantities> &UCOld,
                                     LevelArray<Quantities> &UCNew,
                                     double Time, double TCOld,
                                     double TCNew, double Dt) {
  /*
    println("stepSymmetricDiff Time = " << Time <<
            ", TCOld = " << TCOld <<
            ", TCNew = " << TCNew <<
            ", Dt = " << Dt);
  */
  // SharedRegion tempRegion = new SharedRegion(); // REGION
  // { // scope for new objects using tempRegion
  int myProc = MYTHREAD;
  domain<1> myBoxes = ba.procboxes(myProc);

  // Copy U to UTemp1
  LevelArray<Quantities> UTemp1(U.boxArray());
  foreach (lba, myBoxes)
    UTemp1[lba].copy(U[lba]);

  // Copy U to UTemp2
  LevelArray<Quantities> UTemp2(U.boxArray());
  foreach (lba, myBoxes)
    UTemp2[lba].copy(U[lba]);

  /*
    { // diagnostic
    LevelArray<double> myNorm = new
    (tempRegion) // REGION
    LevelArray<double>(
    tempRegion, // REGION
    U.boxArray());

    foreach (lba, myBoxes) foreach (p, U[lba].domain())
    myNorm[lba][p] = UTemp1[lba][p].norm();

    double myNormLocal = 0.0;
    foreach (lba, myBoxes) foreach (p, U[lba].domain())
    if (myNorm[lba][p] > myNormLocal)
    myNormLocal = myNorm[lba][p];
    println("myNormLocal for initial UTemp1 = " << myNormLocal);
    }

    { // diagnostic
    LevelArray<double> myNorm = new
    (tempRegion) // REGION
    LevelArray<double>(
    tempRegion, // REGION
    U.boxArray());

    foreach (lba, myBoxes) foreach (p, U[lba].domain())
    myNorm[lba][p] = UTemp2[lba][p].norm();

    double myNormLocal = 0.0;
    foreach (lba, myBoxes) foreach (p, U[lba].domain())
    if (myNorm[lba][p] > myNormLocal)
    myNormLocal = myNorm[lba][p];
    println("myNormLocal for initial UTemp2 = " << myNormLocal);
    }
  */

  // No flux registers, no 4th-order slopes, no flattening.

  // Do step with dimension 1 first.
  step(UTemp1, UCOld, UCNew, Time, TCOld, TCNew, Dt, NULL, NULL,
       true, false, false);

  // Do step with dimension 1 last.
  step(UTemp2, UCOld, UCNew, Time, TCOld, TCNew, Dt, NULL, NULL,
       false, false, false);

  /*
    { // diagnostic
    LevelArray<double> myNorm = new
    (tempRegion) // REGION
    LevelArray<double>(
    tempRegion, // REGION
    U.boxArray());

    foreach (lba, myBoxes) foreach (p, U[lba].domain()) {
    myNorm[lba][p] = UTemp1[lba][p].norm();
    // println("final UTemp1" << lba << "" << p << " = " << UTemp1[lba][p]);
    } // almost all NaN for level == 1

    double myNormLocal = 0.0;
    foreach (lba, myBoxes) foreach (p, U[lba].domain())
    if (myNorm[lba][p] > myNormLocal)
    myNormLocal = myNorm[lba][p];
    println("myNormLocal for final UTemp1 = " << myNormLocal);
    }

    { // diagnostic
    LevelArray<double> myNorm = new
    (tempRegion) // REGION
    LevelArray<double>(
    tempRegion, // REGION
    U.boxArray());

    foreach (lba, myBoxes) foreach (p, U[lba].domain()) {
    myNorm[lba][p] = UTemp2[lba][p].norm();
    // println("final UTemp2" << lba << "" << p << " = " << UTemp2[lba][p]);
    } // almost all NaN for level == 1

    double myNormLocal = 0.0;
    foreach (lba, myBoxes) foreach (p, U[lba].domain())
    if (myNorm[lba][p] > myNormLocal)
    myNormLocal = myNorm[lba][p];
    println("myNormLocal for final UTemp2 = " << myNormLocal);
    }
  */

  // Average the two, then take the difference between that
  // and the original, and divide by Dt.
  foreach (lba, myBoxes) {
    foreach (p, U[lba].domain()) {
      dUdt[lba][p] =
        ((UTemp1[lba][p] + UTemp2[lba][p]) * 0.5 - U[lba][p]) / Dt;
    }
  }

  /*
    { // diagnostic
    LevelArray<double> myNorm = new
    (tempRegion) // REGION
    LevelArray<double>(
    tempRegion, // REGION
    U.boxArray());

    foreach (lba, myBoxes) foreach (p, U[lba].domain())
    myNorm[lba][p] = dUdt[lba][p].norm();

    double myNormLocal = 0.0;
    foreach (lba, myBoxes) foreach (p, U[lba].domain())
    if (myNorm[lba][p] > myNormLocal)
    myNormLocal = myNorm[lba][p];
    println("myNormLocal for our dUdt = " << myNormLocal);
    }
  */

  // }
  // RegionUtil::delete(tempRegion); // REGION
  UTemp1.destroy();
  UTemp2.destroy();
}

double LevelGodunov::step(LevelArray<Quantities> &U,
                          LevelArray<Quantities> &UCOld,
                          LevelArray<Quantities> &UCNew,
                          double Time, double TCOld, double TCNew,
                          double Dt, FluxRegister *Fr,
                          FluxRegister *FrFine, bool EvenParity,
                          bool doFourth, bool doFlattening) {
  ostringstream oss;
  oss << "LevelGodunov step dx=" << dx;
  Logger::append(oss.str());
  // println("In lg.step()");
  double cfl = Dt / estTimeStep(U);
  // println("lg.step() got cfl");

  // SharedRegion tempRegion = new SharedRegion(); // REGION
  // { // scope for new objects using tempRegion
  int myProc = MYTHREAD;
  domain<1> myBoxes = ba.procboxes(myProc);
  rectdomain<1> allBoxes = ba.indices();

  // println("step TCOld = " << TCOld << ", Time = " << Time <<
  //         ", TCNew = " << TCNew << ", Dt = " << Dt <<
  //         ", UCOld array = " << UCOld.boxArray());
  // U.checkNaN("step U");

  // println("*** " << myProc << " has boxes " << myBoxes <<
  //         " taking time " << Dt);

  /*
    Compute coefficients of artificial viscosity.
  */

  ndarray<LevelArray<double>, 1> difCoefs(Util::DIMENSIONS);

  foreach (pdim, Util::DIMENSIONS) {
    ndarray<rectdomain<SPACE_DIM>, 1> boxesVisc(allBoxes);

    foreach (lba, allBoxes)
      boxesVisc[lba] = Util::outerEdges(ba[lba], pdim[1]);

    Logger::barrier();

    BoxArray baVisc(boxesVisc, ba.proclist());

    // difCoefs[dimVisc][lba]:  outerEdges(ba[lba], dimVisc)

    difCoefs[pdim] = LevelArray<double>(baVisc);
  }

  LevelArray<Quantities> UTemp(U.boxArray());

  lc.clear();  // need this before lc is used
  if (FrFine != NULL)
    FrFine->clear();

  for (int passDim = 1; passDim <= SPACE_DIM; passDim++) {

    // Use Strang alternation of ordering of fractional steps
    // to preserve second-order accuracy in time.
    int sweepDim = EvenParity ? passDim : (SPACE_DIM+1) - passDim;

    /*
      Table shows sweepDim.    passDim
                                1 2
      EvenParity == true:       1 2
      EvenParity == false:      2 1
    */

    Logger::barrier();
    // Loop through grids.  Not!

    foreach (lba, myBoxes) {

      // println("****** " << myProc << " Look number " << passDim
      //         << " in dimension " << sweepDim << " at the box "
      //         << ba[lba]);
      // Form patch variable, and fill it with data.

      rectdomain<SPACE_DIM> stateBox = dProblem *
        fp.innerBox(lba).accrete(flapSize,
                                 +sweepDim).accrete(flapSize,
                                                    -sweepDim);

      // stateBox:  ba[lba] + 1 (isotropic) + 3 (sweepDim)

      ndarray<Quantities, SPACE_DIM> state(stateBox);

      /*
        Fill state with data from U as much as possible.
        Then within inner radius, interpolate from UCOld and UCNew.
        Some cells within outer radius may be left unfilled.
      */
      fp.fill(U, UCOld, UCNew, state, lba, Time, TCOld, TCNew);
      // println(myProc << " Done fp.fill");

      /*
        Apply correction to the state in the first row of ghost cells
        adjacent to ba[lba] in all of the normal directions
        that have not yet been updated.  (LevequeCorrection keeps
        track of that information internally.)
       */
      lc.correctBoundary(state, lba, sweepDim);
      // println(myProc << " Done lc.correctBoundary");

      /*
        Fill the unfilled cells (within outer radius) of state by copying.
      */
      fp.extend(state, lba, sweepDim, flapSize);
      // println(myProc << " Done fp.extend");

      /*
        If this is first time through the dimension loop,
        form artificial viscosity coefficients for all dimensions.
      */

      if (passDim == 1)
        for (int dimVisc = 1; dimVisc <= SPACE_DIM; dimVisc++)
          artVisc(state, difCoefs[dimVisc][lba], dimVisc);

      // difCoefs[dimVisc][lba]:  outerEdges(ba[lba], dimVisc)

      // println(myProc << " Done artVisc");

      /*
        Compute primitive variables.
      */

      rectdomain<SPACE_DIM> primBox = dProblem *
        ba[lba].accrete(flapSize + 1,
                        +sweepDim).accrete(flapSize + 1, -sweepDim);

      // primBox:  ba[lba] + (3+1) (sweepDim)

      ndarray<Quantities, SPACE_DIM> q(primBox);

      // println(myProc << " About to do consToPrim");
      consToPrim(state, q);

      /*
        Compute slopes.
      */

      rectdomain<SPACE_DIM> slopeBox = dProblem *
        ba[lba].accrete(1, +sweepDim).accrete(1, -sweepDim);

      // slopeBox:  ba[lba] + 1 (sweepDim)

      ndarray<Quantities, SPACE_DIM> dq(slopeBox);

      // q on primBox:  ba[lba] + (3+1) (sweepDim)
      // dq on slopeBox:  ba[lba] + 1 (sweepDim)
      slope(q, dq, sweepDim, doFourth, doFlattening);
      // println(myProc << " Done slope " << slopeBox);

      /*
        Compute predictor step to obtain extrapolated edge values.
      */

      ndarray<Quantities, SPACE_DIM> qlo(slopeBox);
      ndarray<Quantities, SPACE_DIM> qhi(slopeBox);

      normalPred(q, dq, qlo, qhi, Dt / dx, sweepDim);
      // println(myProc << " Done normalPred");

      /*
        Shift qlo, qhi to edges, and call Riemann solver on interior
        edges.
      */

      rectdomain<SPACE_DIM> riemannBox =
        Util::innerEdges(slopeBox, sweepDim);
      rectdomain<SPACE_DIM> edgeBox =
        Util::outerEdges(ba[lba], sweepDim);

      // riemannBox: inner edges of (ba[lba] + 1 (sweepDim)) in
      //             sweepDim
      // edgeBox:  outer edges of (ba[lba]) in sweepDim
      // So riemannBox should be the same as edgeBox unless
      // ba[lba] has a physical boundary in sweepDim.

      ndarray<Quantities, SPACE_DIM> qGdnv(riemannBox);
      // (qlo.translate(t))[p + t] == qlo[p];
      // (qlo.translate(t)) has domain qlo.domain() + t
      // On left is qhi[-1/2], on right is qlo[+1/2].
      riemann(qhi.translate(Util::cell2edge[+sweepDim]),
              qlo.translate(Util::cell2edge[-sweepDim]),
              qGdnv, sweepDim);
        // call with qhi[e + uce[-sdim]], qlo[e + uce[+sdim]]
      // println(myProc << " Done riemann");

      /*
        Compute fluxes, including effect of artificial viscosity and
        boundary conditions.
      */

      // edgeBox:  outer edges of (ba[lba]) in sweepDim
      ndarray<Quantities, SPACE_DIM> flux(edgeBox);
      fluxCalc(qGdnv, state, difCoefs[sweepDim][lba], flux,
               sweepDim);
      // println(myProc << " Done fluxCalc");

      /*
        Perform conservative update, incrementing LevequeCorrection
        object.

        The intermediate update is stored into UTemp, so as not to
        write over data in U that might be required as ghost cell
        data for other grid updates.
      */

      // get original U, copy into UTemp.
      UTemp[lba].copy(U[lba]);

      update(flux, UTemp[lba], Dt / dx, sweepDim);
      // println(myProc << " Done update");

      lc.increment(flux, Dt, lba, sweepDim);
      // println(myProc << " Done lc.increment");

      /*
        Increment flux registers in this dimension
        for interface with next coarser level, if any.
      */

      // println("Calling Fr->incrementFine for box " << ba[lba]);
      if (Fr != NULL)
        foreach (pdir, Util::DIRECTIONS) {
          // flux.domain() == edgeBox: outer edges of (ba[lba]) in
          // sweepDim
          Fr->incrementFine(flux, +pdir[1]*Dt/(dx*dRefine), lba,
                            sweepDim, pdir);
        }

      // println(myProc << " Done Fr->incrementFine");

      /*
        Increment (actually, fill in) flux registers in this dimension
        for interface with next finer level, if any.
      */

      // coarse:  fill in flux registers in this dimension

      if (FrFine != NULL)
        foreach (pdir, Util::DIRECTIONS)
          FrFine->incrementCoarse(flux, -pdir[1]*Dt/dx, lba,
                                  sweepDim, pdir);

      // println(myProc << " Done FrFine->incrementCoarse");

      state.destroy();
      q.destroy();
      dq.destroy();
      qhi.destroy();
      qlo.destroy();
      qGdnv.destroy();
      flux.destroy();
    } // end of loop over grids

    // println(myProc << " reached the end in dim " << sweepDim);
    Logger::barrier();

    foreach (lba, myBoxes)
      U[lba].copy(UTemp[lba]);
    // println(myProc << " copied UTemp to U");

    Logger::barrier();

  } // end of loop over sweep dimensions

  // println(myProc << " ---------- done lg.step");
  oss.str("");
  oss << "Done LevelGodunov step dx=" << dx;
  Logger::append(oss.str());
  // } // scope for new objects using tempRegion
  // RegionUtil::delete(tempRegion); // REGION
  foreach (p, difCoefs.domain())
    difCoefs[p].destroy();
  difCoefs.destroy();
  return cfl;
}

void LevelGodunov::artVisc(ndarray<Quantities, SPACE_DIM> state,
                           ndarray<double, SPACE_DIM,
                           global> difCoefs,
                           int dimVisc) {
  // for (int dimVisc = 1; dimVisc <= SPACE_DIM; dimVisc++)
  // artVisc(state, difCoefs[dimVisc][lba], dimVisc);
  // difCoefs[dimVisc][lba]:  outerEdges(ba[lba], dimVisc)
  // stateBox:  ba[lba] + 1 (isotropic) + 3 (sweepDim)
  rectdomain<SPACE_DIM> stateBox = state.domain();
  ndarray<Quantities, SPACE_DIM> statePrim(stateBox);
  consToPrim(state, statePrim);
  rectdomain<SPACE_DIM> outerE = difCoefs.domain();

  /*
    Normal component of divergence:
    calculate on all edges except on physical boundary.
  */
  int VELOCITY_N = velocityIndex(dimVisc);
  // foreach (e, outerE - dProblemEdges[dimVisc])
  foreach (e, outerE) {
    if (! dProblemEdges[dimVisc].contains(e))
      difCoefs[e] = ART_VISC_COEFF *
        ((statePrim[e + Util::edge2cell[+dimVisc]][VELOCITY_N] -
          statePrim[e + Util::edge2cell[-dimVisc]][VELOCITY_N]));
  }

  /*
    Tangential components of divergence:
    calculate on all edges except on physical boundary.
    Use one-sided differences if edge abuts physical boundary.
  */

  // foreach (pdimOther, Util::DIMENSIONS - [dimVisc:dimVisc])
  foreach (pdimOther, Util::DIMENSIONS) {
    if (pdimOther[1] != dimVisc) {
      int VELOCITY_T = velocityIndex(pdimOther);
      point<SPACE_DIM> unitT =
        point<SPACE_DIM>::direction(pdimOther[1]);

      // abutters[dir]: edges (in dimension dimVisc) that abut the
      // side dir*dimOther of the physical domain.
      ndarray<rectdomain<SPACE_DIM>, 1> &abutters =
        dProblemAbutters[pdimOther][dimVisc];

      /*
        Aliased versions of statePrim:
        -|----|----|-
         |    |    |
         | LU | RU |
         |    |    |
        -|----|----|-
         |    |    |
         | L  e  R |
         |    |    |
        -|----|----|-
         |    |    |
         | LD | RD |
         |    |    |
        -|----|----|-
      */
      ndarray<Quantities, SPACE_DIM> stateR =
        statePrim.translate(Util::ZERO - Util::edge2cell[+dimVisc]);
      ndarray<Quantities, SPACE_DIM> stateRU =
        stateR.translate(Util::ZERO - unitT);
      ndarray<Quantities, SPACE_DIM> stateRD =
        stateR.translate(Util::ZERO + unitT);
      ndarray<Quantities, SPACE_DIM> stateL =
        statePrim.translate(Util::ZERO - Util::edge2cell[-dimVisc]);
      ndarray<Quantities, SPACE_DIM> stateLU =
        stateL.translate(Util::ZERO - unitT);
      ndarray<Quantities, SPACE_DIM> stateLD =
        stateL.translate(Util::ZERO + unitT);

      // foreach (e, outerE -
      // (dProblemEdges[dimVisc] + abutters[-1] + abutters[+1]))
      foreach (e, outerE)
        if (! dProblemEdges[dimVisc].contains(e))
          if (! abutters[-1].contains(e))
            if (! abutters[+1].contains(e))
              difCoefs[e] = difCoefs[e] + (ART_VISC_COEFF * 0.25) *
                ((stateRU[e][VELOCITY_T] - stateRD[e][VELOCITY_T]) +
                 (stateLU[e][VELOCITY_T] - stateLD[e][VELOCITY_T]));

      // Use one-sided differences when edges abut physical boundaries.

      // Bottom:  stateLD, stateRD are off the grid.
      foreach (e, outerE * abutters[-1])
        difCoefs[e] = difCoefs[e] + (ART_VISC_COEFF * 0.5) *
        ((stateRU[e][VELOCITY_T] - stateR[e][VELOCITY_T]) +
         (stateLU[e][VELOCITY_T] - stateL[e][VELOCITY_T]));

      // Top:  stateLU, stateRU are off the grid.
      foreach (e, outerE * abutters[+1])
        difCoefs[e] = difCoefs[e] + (ART_VISC_COEFF * 0.5) *
        ((stateR[e][VELOCITY_T] - stateRD[e][VELOCITY_T]) +
         (stateL[e][VELOCITY_T] - stateLD[e][VELOCITY_T]));
    }
  }

  /*
    Zero out difCoefs wherever positive.
  */
  // foreach (e, outerE - dProblemEdges[dimVisc])
  foreach (e, outerE) {
    if (! dProblemEdges[dimVisc].contains(e))
      if (difCoefs[e] > 0.0)
        difCoefs[e] = 0.0;
  }

  /*
    On physical boundary, copy from neighbor.
  */
  foreach (pdir, Util::DIRECTIONS) {
    int dimdir = pdir[1] * dimVisc;
    point<SPACE_DIM> unitBack =
      point<SPACE_DIM>::direction(-dimdir);
    foreach (e, outerE *
             (dProblem.border(1, dimdir, 0) +
              Util::cell2edge[dimdir]))
      difCoefs[e] = difCoefs[e + unitBack];
  }

  statePrim.destroy();
}

void LevelGodunov::slope(ndarray<Quantities, SPACE_DIM> &q,
                         ndarray<Quantities, SPACE_DIM> &dq,
                         int sweepDim, bool doFourth,
                         bool doFlatten) {
  /*
    rectdomain<SPACE_DIM> slopeBox = dProblem * ba[lba]
    .accrete(1, +sweepDim)
    .accrete(1, -sweepDim);

    ndarray<Quantities, SPACE_DIM> dq(slopeBox);

    // q on primBox:  ba[lba] + (3+1) (sweepDim)
    // dq on slopeBox:  ba[lba] + 1 (sweepDim)

    slope(q, dq, sweepDim);
  */
  point<SPACE_DIM> unit = point<SPACE_DIM>::direction(sweepDim);
  rectdomain<SPACE_DIM> primBox = q.domain();
  rectdomain<SPACE_DIM> slopeBox = dq.domain();

  ndarray<Quantities, SPACE_DIM> dq2(slopeBox);

  // Two-sided differences in interior.
  rectdomain<SPACE_DIM> DInner =
    slopeBox * primBox.shrink(1, +sweepDim).shrink(1, -sweepDim);

  foreach (p, DInner) {
    dq2[p] = Quantities::vanLeer(q[p - unit], q[p], q[p + unit]);
  }

  // One-sided differences at ends where physical boundaries lie.
  // Do something different on solid wall boundaries.

  bc->slope(q, slopeBox.border(1, -sweepDim, 0) - DInner, -sweepDim,
            dq2, dx);

  bc->slope(q, slopeBox.border(1, +sweepDim, 0) - DInner, +sweepDim,
            dq2, dx);

  rectdomain<SPACE_DIM> DFourth;
  if (doFourth) {
    DFourth = DInner.shrink(1, +sweepDim).shrink(1, -sweepDim);
  } else {
    DFourth = Util::EMPTY;
  }

  // foreach (p, slopeBox - DFourth) dq[p] = dq2[p];
  foreach (p, slopeBox) {
    if (! DFourth.contains(p))
      dq[p] = dq2[p];
  }
  foreach (p, DFourth) {
    dq[p] = Quantities::slope4(q[p - unit], q[p], q[p + unit],
                               dq2[p - unit] + dq2[p + unit]);
  }

  if (doFlatten) {
    flatten(q, dq, sweepDim);
  }

  dq2.destroy();
}

void LevelGodunov::flatten(ndarray<Quantities, SPACE_DIM> &q,
                           ndarray<Quantities, SPACE_DIM> &dq,
                           int sweepDim) {
  point<SPACE_DIM> unit = point<SPACE_DIM>::direction(sweepDim);
  rectdomain<SPACE_DIM> primBox = q.domain();
  rectdomain<SPACE_DIM> slopeBox = dq.domain();

  double DELTA = 0.33, Z0 = 0.75, Z1 = 0.85;

  rectdomain<SPACE_DIM> flattenedBox =
    primBox.shrink(2, +sweepDim).shrink(2, -sweepDim);

  ndarray<ndarray<double, 1>, SPACE_DIM> chiBar(flattenedBox);
  foreach (p, flattenedBox) {
    chiBar[p].create(Util::DIMENSIONS);
  }

  foreach (p, flattenedBox) {
    double pNext = q[p + unit][PRESSURE];
    double pNextNext = q[p + unit + unit][PRESSURE];
    double pPrev = q[p - unit][PRESSURE];
    double pPrevPrev = q[p - unit - unit][PRESSURE];
    if (fabs(pNext - pPrev) > DELTA * fmin(pNext, pPrev)) {
      double ratio =
        fabs((pNext - pPrev) / (pNextNext - pPrevPrev));
      double eta =
        fmin(1.0, fmax(0.0, 1.0 - (ratio - Z0) / (Z1 - Z0)));
      foreach (pdim, Util::DIMENSIONS) {
        int VELOCITY_N = velocityIndex(pdim);
        if (q[p - unit][VELOCITY_N] > q[p + unit][VELOCITY_N]) {
          chiBar[p][pdim] = eta;
        } else {
          chiBar[p][pdim] = 1.0;
        }
      }
    } else {
      foreach (pdim, Util::DIMENSIONS)
        chiBar[p][pdim] = 1.0;
    }
  }

  foreach (p, slopeBox * flattenedBox) {
    double chi = 1.0;
    foreach (pdim, Util::DIMENSIONS) {
      chi = fmin(chi, chiBar[p][pdim]);
    }
    point<SPACE_DIM> adj =
      (q[p + unit][PRESSURE] > q[p - unit][PRESSURE]) ?
      (p - unit) : (p + unit);
    if (flattenedBox.contains(adj)) {
      foreach (pdim, Util::DIMENSIONS) {
        chi = fmin(chi, chiBar[adj][pdim]);
      }
    }
    dq[p] = dq[p] * chi;
  }

  foreach (p, flattenedBox) {
    chiBar[p].destroy();
  }
  chiBar.destroy();
}

void LevelGodunov::normalPred(ndarray<Quantities, SPACE_DIM> &q,
                              ndarray<Quantities, SPACE_DIM> &dq,
                              ndarray<Quantities, SPACE_DIM> &qlo,
                              ndarray<Quantities, SPACE_DIM> &qhi,
                              double DtDx, int sweepDim) {
  double halfDtDx = 0.5 * DtDx;
  rectdomain<SPACE_DIM> slopeBox = dq.domain();

  int VELOCITY_N = velocityIndex(sweepDim);

  foreach (p, slopeBox) {
    double c = soundspeed(q[p]);
    double vN = q[p][VELOCITY_N];
    Quantities eig0;

    double dqpp = dq[p][DENSITY] - dq[p][PRESSURE] / (c * c);

    eig0 = Quantities(DENSITY, dqpp);
    // foreach (pdim, Util::DIMENSIONS - RD(sweepDim,sweepDim+1)) {
    foreach (pdim, Util::DIMENSIONS) {
      if (pdim[1] != sweepDim) {
        int VELOCITY_T = velocityIndex(pdim);
        eig0 += Quantities(VELOCITY_T, dq[p][VELOCITY_T]);
      }
    }

    qlo[p] = q[p] - dq[p] * (0.5 + halfDtDx * fmin(vN - c, 0.0));
    if (vN < 0.0) {
      qlo[p] = qlo[p] - eig0 * (halfDtDx * c);
      if (vN < -c)
        qlo[p] =
          qlo[p] - eigs(+sweepDim, q[p], dq[p], c) * halfDtDx * c;
    }

    qhi[p] = q[p] + dq[p] * (0.5 - halfDtDx * fmax(vN + c, 0.0));
    if (vN > 0.0) {
      qhi[p] = qhi[p] + eig0 * (halfDtDx * c);
      if (vN > c)
        qhi[p] =
          qhi[p] + eigs(-sweepDim, q[p], dq[p], c) * halfDtDx * c;
    }
  }
}

void LevelGodunov::riemann(ndarray<Quantities, SPACE_DIM> qleft,
                           ndarray<Quantities, SPACE_DIM> qright,
                           ndarray<Quantities, SPACE_DIM> qGdnv,
                           int sweepDim) {
  int VELOCITY_N = velocityIndex(sweepDim);
  int VELOCITY_T = velocityOther(sweepDim);

  foreach (e, qGdnv.domain()) {
    Quantities qL = qleft[e], qR = qright[e];
    double cL = soundspeed(qL), cR = soundspeed(qR);

    double rhoL = qL[DENSITY], rhoR = qR[DENSITY];
    double vNL = qL[VELOCITY_N], vNR = qR[VELOCITY_N];
    double vTL = qL[VELOCITY_T], vTR = qR[VELOCITY_T];
    double pL = qL[PRESSURE], pR = qR[PRESSURE];

    double wL = cL * rhoL, wR = cR * rhoR;

    double p =
      (pL * wR + pR * wL + wL * wR * (vNL - vNR)) / (wL + wR);
    p = fmax(p, MIN_PRESSURE);

    double u = (vNR * wR + vNL * wL + pL - pR) / (wL + wR);

    double usgn, cside, rhoside, vNside, vTside, pside;
    Quantities qside;
    if (u >= 0.0) {
      usgn = +1.0; qside = qL; cside = cL;
      rhoside = rhoL; vNside = vNL; vTside = vTL; pside = pL;
    } else {
      usgn = -1.0; qside = qR; cside = cR;
      rhoside = rhoR; vNside = vNR; vTside = vTR; pside = pR;
    }

    double rho = rhoside + (p - pside) / (cside * cside);
    rho = fmax(rho, MIN_DENSITY);

    Quantities qstar(DENSITY, rho,
                     VELOCITY_N, u,
                     VELOCITY_T, vTside,
                     PRESSURE, p);

    double ustar =
      u - usgn * soundspeed(qstar);  // hence |ustar| <= |u|
    double v = vNside - usgn * cside;

    Quantities qmid;
    if (p > pside) {
      // Shock at this side.
      if (usgn * (ustar + v) >= 0.0)
        qmid = qside;
      else
        qmid = qstar;
    } else {
      // Rarefaction at this side.
      if (usgn * v >= 0.0)
        qmid = qside;
      else if (usgn * ustar <= 0.0)
        qmid = qstar;
      else {
        double vustardif = v - ustar;
        // usgn * v < 0, usgn * ustar > 0
        // hence usgn * (v - ustar) < 0
        double frac;
        if (-usgn * vustardif < RELATIVE_TOLERANCE * (-usgn * v))
          frac = 1.0;
        else
          frac = fmax(0.0, fmin(v / vustardif, 1.0));
        qmid = qstar * frac + qside * (1.0 - frac);
      }
    }
    qGdnv[e] = qmid;
  } // foreach (e, riemannBox)
}

void LevelGodunov::fluxCalc(ndarray<Quantities, SPACE_DIM> qGdnv,
                            ndarray<Quantities, SPACE_DIM> state,
                            ndarray<double, SPACE_DIM,
                            global> difCoefs,
                            ndarray<Quantities, SPACE_DIM> flux,
                            int sweepDim) {
  // qGdnv.domain() ==
  // riemannBox:  inner edges of (ba[lba] + 1 (sweepDim)) in sweepDim
  // state.domain() ==
  // stateBox:  ba[lba] + 1 (isotropic) + 3 (sweepDim)
  // difCoefs.domain() == outer edges of (ba[lba]) in sweepDim
  // flux.domain() ==
  // edgeBox:  outer edges of (ba[lba]) in sweepDim

  // Hence flux.domain() - qGdnv.domain() ==
  // outer edges of (ba[lba]) in sweepDim that lie on physical boundary

  foreach (e, qGdnv.domain()) {
    flux[e] = interiorFlux(qGdnv[e], sweepDim) +
      (state[e + Util::edge2cell[+sweepDim]] -
       state[e + Util::edge2cell[-sweepDim]]) * difCoefs[e];
  }

  // Get flux at physical boundaries, using PhysBC bc.

  // flux.domain() - qGdnv.domain() ==
  // outer edges of (ba[lba]) in sweepDim that lie on physical boundary.
  // foreach (e, flux.domain() - qGdnv.domain())
  // flux[e] = new Quantities::zero();

  // qGdnv is not defined on the physical boundary.  Use qlo, qhi?  Or state?
  foreach (pdir, Util::DIRECTIONS) {
    int dimdir = sweepDim * pdir[1];
    rectdomain<SPACE_DIM> physBorder =
      dProblemEdges[sweepDim] * flux.domain().border(1, dimdir, 0);
    if (! physBorder.is_empty())
      bc->f(state.constrict(physBorder + Util::edge2cell[-dimdir]),
            difCoefs.constrict(physBorder), dimdir,
            flux.constrict(physBorder), dx);
  }
}
