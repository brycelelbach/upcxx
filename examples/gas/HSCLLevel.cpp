#include "HSCLLevel.h"
#include "LoadBalance.h"
#include "MeshRefine.h"

point<SPACE_DIM> HSCLLevel::TWO = point<SPACE_DIM>::all(2);

HSCLLevel::HSCLLevel(Amr *Parent, int Level, BoxArray Ba,
                     BoxArray BaCoarse,
                     rectdomain<SPACE_DIM> DProblem,
                     int NRefineCoarser, double Dx, PhysBC *BC) :
  AmrLevel(Parent, Level, Ba, BaCoarse, DProblem, NRefineCoarser,
           Dx),
  evenParity(true), coarsePerimeter(0), coarseInterface(Util::EMPTY),
  coarseInterfaceOff(Util::EMPTY), fineInterface(Util::EMPTY),
  finerLevel(NULL) {
  bc = BC;
  ostringstream oss;
  oss << "getting LevelGodunov object for level " << Level;
  Logger::append(oss.str());
  oss.str("");
  lg = new LevelGodunov(ba, baCoarse, dProblem, nRefineCoarser, dx,
                        bc);
  oss << "got LevelGodunov object for level " << Level;
  Logger::append(oss.str());
  ParmParse pp("hscl");
  if (MYTHREAD == SPECIAL_PROC) {
    cflmax = pp.getDouble("cfl");
    useGradient = pp.contains("gradient");
  }
  cflmax = broadcast(cflmax, SPECIAL_PROC);
  useGradient = broadcast(useGradient, SPECIAL_PROC);
  if (useGradient) {
    minRelDiff = pp.getDouble("tagthresh");
  } else {
    minRelTrunc = pp.getDouble("truncthresh");
  }
  tagvar = pp.getInteger("tagvar");
  computePerimeter();
  if (MYTHREAD == SPECIAL_PROC) {
    useNoFineInterface = pp.contains("no_fine_interface");
    useNoCoarseInterface = pp.contains("no_coarse_interface");
  }
  useNoFineInterface = broadcast(useNoFineInterface, SPECIAL_PROC);
  useNoCoarseInterface = broadcast(useNoCoarseInterface,
                                   SPECIAL_PROC);
}

domain<SPACE_DIM> HSCLLevel::computeUnion(domain<SPACE_DIM> &unionLocal) {
  ndarray<rectdomain<SPACE_DIM>,
          1> unionLocalA(RD(0, unionLocal.num_rectdomains()));
  unionLocal.copy_rectdomains_to(unionLocalA.base_ptr());

  // kludge
  rectdomain<1> procTeam(0, THREADS);
  ndarray<ndarray<rectdomain<SPACE_DIM>, 1>, 1> unionAll(procTeam);
  unionAll.exchange(unionLocalA);
  domain<SPACE_DIM> unionSpecial = Util::EMPTY;
  ndarray<rectdomain<SPACE_DIM>, 1, global> unionSpecialA;
  if (MYTHREAD == SPECIAL_PROC) {
    size_t max_size = 0;
    foreach (proc, procTeam) {
      if (unionAll[proc].size() > max_size)
        max_size = unionAll[proc].size();
    }
    ndarray<rectdomain<SPACE_DIM>, 1> dd(RD(0, max_size));
    foreach (proc, procTeam) {
      dd.copy(unionAll[proc]);
      domain<SPACE_DIM> dd_local =
        domain<SPACE_DIM>::to_domain(dd.base_ptr(),
                                     unionAll[proc].size());
      unionSpecial += dd_local;
    }
    dd.destroy();
    unionSpecialA =
      ndarray<rectdomain<SPACE_DIM>,
              1>(RD(0, unionSpecial.num_rectdomains()));
    unionSpecial.copy_rectdomains_to(unionSpecialA.base_ptr());
  }
  Logger::barrier();
  unionLocalA.destroy();
  unionSpecialA = broadcast(unionSpecialA, SPECIAL_PROC);
  if (MYTHREAD != SPECIAL_PROC) {
    unionLocalA =
      ndarray<rectdomain<SPACE_DIM>,
              1>(RD(0, unionSpecialA.size()));
    unionLocalA.copy(unionSpecialA);
    unionSpecial =
      domain<SPACE_DIM>::to_domain(unionLocalA.base_ptr(),
                                   unionLocalA.size());
  } else {
    unionLocalA = (ndarray<rectdomain<SPACE_DIM>, 1>) unionSpecialA;
  }
  Logger::barrier();
  unionLocalA.destroy();
  return unionSpecial;
}

void HSCLLevel::computePerimeter() {
  if (level == 0) return;  // there is no coarser level

  /*
    Find union = domain of all cells in grids at this level
  */

  domain<SPACE_DIM> unionLocal = Util::EMPTY;
  foreach (lba, myBoxes)
    unionLocal += ba[lba];
  domain<SPACE_DIM> uniond = computeUnion(unionLocal);

  /*
    Find coarseInterface, which consists of the cells on this level that
    are adjacent (including diagonally) to cells on next coarser level.
    coarseInterface is everything in the domain that is NOT in the
    proper nesting domain.

    coarseInterfaceOff = domain just _outside_ coarseInterface
  */
  domain<SPACE_DIM> PND = uniond, unionPlus = uniond;
  foreach (pdim, Util::DIMENSIONS) {
    foreach (pdir, Util::DIRECTIONS) {
      // extend one cell in each direction
      point<SPACE_DIM> shift =
        point<SPACE_DIM>::direction(pdim[1]*pdir[1], 1);

      domain<SPACE_DIM> remove =
        (((PND + shift) - PND) * dProblem) - shift;
      PND -= remove;

      domain<SPACE_DIM> newSide = (unionPlus + shift) * dProblem;
      unionPlus += newSide;
    }
  }
  coarseInterface = uniond - PND;
  coarseInterfaceOff = unionPlus - uniond;

  /*
  coarseInterface = Util::EMPTY;
  foreach (pdim in Util::DIMENSIONS) foreach (pdir in Util::DIRECTIONS) {
    // extend one cell in each direction
    point<SPACE_DIM> shift = point<SPACE_DIM>::direction(pdim[1]*pdir[1], 1);
    domain<SPACE_DIM> newSide =
      (((union + shift) - union) * dProblem) - shift;
    coarseInterface = coarseInterface + newSide;
  }
  */
  coarsePerimeter = coarseInterface.size();
}

void HSCLLevel::getSaved(MyInputStream &in) {
  // (a) level number and # of grids per level (2*int)
  ndarray<int, 1> savedLevelStuff = in.readInts();
  int numGrids = savedLevelStuff[1];

  ndarray<rectdomain<SPACE_DIM>, 1> domainlist(RD(0, numGrids));
  /*
    The plot file doesn't specify where each grid is stored.
    (Just as well:  you shouldn't have to continue with the same
    number of processes.)
    You'll need to call the load balancer.
  */

  for (int ind = 0; ind < numGrids; ind++) {
    // For each grid,
    // (1) Grid indices (BOX)
    domainlist[ind] = in.readDomain();

    // (2) Level (int)
    int savedLevel = in.readInt1();
    if (savedLevel != level) {
      println("Plot file has miscounted grids.");
      exit(0);
    }

    // (3) Steps taken on this grid (int)
    in.ignoreLine();

    // (4) Time for this grid (REAL)
    in.ignoreLine();

    // (5) Physical location of grid corners (BL_SPACEDIM*(REAL))
    foreach (pdim, Util::DIMENSIONS)
      in.ignoreLine();

    // (6) writeOn data dump of each FAB of data on that grid
  }
}

double HSCLLevel::advance(double Time, double dt) {
  swapTimeLevels(dt);
  // println("calling level " << level +
  // " lg->step from " << Time << " by " << dt);
  double cflout;
  // Recall initialization evenParity = true;

  LevelArray<Quantities> UCOld = (level == 0) ?
    LevelArray<Quantities>::Null() : amrlevel_coarser->getUOld();
  LevelArray<Quantities> UCNew = (level == 0) ?
    LevelArray<Quantities>::Null() : amrlevel_coarser->getUNew();

  // If level == 0 then coarse times aren't used.
  double TCOld =
    (level == 0) ? 0.0 : amrlevel_coarser->getOldTime();
  double TCNew =
    (level == 0) ? 0.0 : amrlevel_coarser->getNewTime();

  FluxRegister *Fr = (level == 0) ? NULL : fReg;
  FluxRegister *FrFine = (level == parent->finestLevel()) ?
    NULL : parent->getLevel(level + 1)->getfReg();

  cflout =
    lg->step(UNew, UCOld, UCNew,
            Time, TCOld, TCNew, dt,
            Fr, FrFine, evenParity, true, true);

  evenParity = !evenParity;
  return cflout;
}

domain<SPACE_DIM> HSCLLevel::tag() {
  domain<SPACE_DIM> myTagged = Util::EMPTY;
  rectdomain<SPACE_DIM> baseSquare(Util::MINUS_ONE,
                                   Util::ONE + Util::ONE);
  ostringstream oss;
  oss << "tagging for " << myProc;
  Logger::append(oss.str());
  // println("tagging for proc " << myProc << " at level " << level);

  // println("HSCLLevel.tag():  new tagRegion");
  // SharedRegion tagRegion = new SharedRegion(); // REGION
  // Make sure there are no problems with tagRegion vs. myRegion
  { // scope for new objects using tagRegion

    // println("repeat:  tagging for proc " << myProc << " at level " << level);
    if (useGradient) {
      // use gradient

      println("Proc " << myProc << " useGradient, level " << level);
      foreach (lba, myBoxes)
        myTagged += tagBoxGradient(UNew[lba], ba[lba]);

    } else /* not useGradient */ {

      // use truncation error estimate from Richardson extrapolation

      if (initial_step) {
        // println("Proc " << myProc << " initial step, level " <<
        //         level);
        // Level 0 differs from higher levels in that there is no
        // coarser level.
        // Hence no interpolation from a coarser level.

        // if (level == 0)
        // Should really set dt only at level 0, because otherwise
        // there may be inconsistencies.
        // If not initial step, then you can use Amr.computeNewDt(),
        // which goes through every level.
        double dt = estTimeStep();

        /*
          Run stepSymmetric three times:
          1.  U(0) to U(dt), dx=h.
          2.  U(dt/2) to U(3*dt/2), dx=h,
              where U(dt/2) ~ (U(0) + U(dt))/2.
          3.  U(0) to U(2*dt), dx=2*h.

                                     2*dt  =
                                           |
                      1.5*dt  =            |
                              |            |
             dt  =            |            |
                 |            |            |
         0.5*dt  -  - - - ->  =            |
                 |            h            |
              0  =  - - - - - - - - - ->   =
                 h                        2h

               UInter      UTempHalf     UTemp2
        */

        // UOld has not been set yet.
        // Perhaps I should rewrite it so that UOld does get set,
        // and UNew gets set to UInter.
        LevelArray<Quantities> UCOld = (level == 0) ?
          LevelArray<Quantities>::Null() :
          amrlevel_coarser->getUNew();
        LevelArray<Quantities> UCNew = (level == 0) ?
          LevelArray<Quantities>::Null() :
          amrlevel_coarser->getUInter();

        /*
          1.
          UNew at this level has been initialized by initGrids().

          Put copy of UNew into UInter, and
          advance UInter symmetrically from 0 to dt.
        */

        UInter = LevelArray<Quantities>(ba);

        UInter.copy(UNew);

        LevelGodunov *lgTag =
          new LevelGodunov(ba, baCoarse, dProblem, nRefineCoarser,
                           dx, bc);

        // advance
        lgTag->stepSymmetric(UInter, UCOld, UCNew,
                             0.0, 0.0, nRefineCoarser * dt, dt);

        /*
          2.
          Interpolate from UNew (t = 0) and UInter (t = dt)
          to UTempHalf (t = 0.5*dt).
          Then advance UTempHalf from 0.5*dt to 1.5*dt.

          We want dUdt = (U(1.5*dt) - U(0.5*dt)) / dt
          instead of the new solution.
          In LevelGodunov.update():
          stateNew = stateOld - fluxDiff * dt/dx
          (stateNew - stateOld) / dt = - fluxDiff / dx
        */

        LevelArray<Quantities> UTempHalf(ba);

        // interpolate
        foreach (lba, myBoxes) {
          foreach (p, ba[lba]) {
            UTempHalf[lba][p] =
              (UNew[lba][p] + UInter[lba][p]) * 0.5;
          }
        }

        delete lgTag;
        lgTag = new LevelGodunov(ba, baCoarse, dProblem,
                                 nRefineCoarser, dx, bc);

        LevelArray<Quantities> dUdt(ba);

        // advance
        lgTag->stepSymmetricDiff(dUdt, UTempHalf, UCOld, UCNew,
                                 dt * 0.5, 0.0, dt * nRefineCoarser,
                                 dt);

        /*
          3.
          Average UNew to a coarsened version, UTemp2, and
          then advance UTemp2 from 0 to 2*dt.
        */

        ndarray<rectdomain<SPACE_DIM>, 1> domainlist2(ba.indices());

        ndarray<int, 1> proclist2(ba.indices());

        proclist2.copy(ba.proclist());

        foreach (ind, ba.indices())
          domainlist2[ind] = ba[ind] / TWO;

        BoxArray ba2(domainlist2, proclist2);

        // my2Boxes should be the same as myBoxes
        domain<1> my2Boxes = ba2.procboxes(myProc);

        LevelArray<Quantities> UTemp2(ba2);

        foreach (lba, my2Boxes) {
          foreach (p, ba2[lba]) {
            UTemp2[lba][p] =
              Quantities::Average(UNew[lba].constrict(Util::subcells(p, 2)));
          }
        }

        // println("ba2 = " << ba2);

        delete lgTag;
        lgTag = new LevelGodunov(ba2, baCoarse, dProblem / TWO,
                                 nRefineCoarser / 2, dx * 2.0, bc);

        LevelArray<Quantities> dU2dt(ba2);

        // advance
        lgTag->stepSymmetricDiff(dU2dt, UTemp2, UCOld, UCNew, 0.0,
                                 0.0, dt * nRefineCoarser,
                                 dt * 2.0);

        /*
          Find maximum of dU2dt
        */

        double maxdU2dtLocal = 0.0;
        foreach (lba, my2Boxes) {
          foreach (p, ba2[lba]) {
            double thisnorm = fabs(dU2dt[lba][p][tagvar]);
            if (thisnorm > maxdU2dtLocal)
              maxdU2dtLocal = thisnorm;
          }
        }
        // println("maxdU2dtLocal = " << maxdU2dtLocal);
        double maxdU2dt = reduce::max(maxdU2dtLocal);

        /*
          Truncation error estimate is average(dUdt) - dU2dt.
          We take the norm to make things simpler.
        */

        LevelArray<double> truncError(ba2);

        // mixing my2Boxes and myBoxes, but they should be the same
        foreach (lba, my2Boxes) {
          foreach (p, ba2[lba]) {
            truncError[lba][p] =
              fabs((Quantities::Average(dUdt[lba].constrict(Util::subcells(p, 2)))
                    - dU2dt[lba][p])[tagvar]);
          }
        }

        barrier();

        /*
          Copy truncation error.
        */

        truncErrorCopied = LevelArray<double>(ba2);
        foreach (lba, myBoxes) {
          foreach (p, ba2[lba]) {
            truncErrorCopied[lba][p] = truncError[lba][p];
          }
        }
        barrier();
        // println("level " << level <<
        //         " err boxes:  " << truncErrorCopied.boxArray());


        /*
          Copy adjusted truncation error.
        */

        truncErrorCopied2 = LevelArray<double>(ba2);
        foreach (lba, myBoxes) {
          foreach (p, ba2[lba]) {
            truncErrorCopied2[lba][p] = truncError[lba][p];
          }
        }
        barrier();
        // println("level " << level <<
        //         " err boxes:  " << truncErrorCopied2.boxArray());


        /*
          Tag where truncError >= epsilon * maxdU2dt
        */

        foreach (lba, my2Boxes) {
          foreach (p, ba2[lba]) {
            if (truncError[lba][p] > minRelTrunc * maxdU2dt)
              foreach (p2, (baseSquare + p)) {
                myTagged = myTagged + Util::subcells(p2, 2);
              }
          }
        }
        // myTagged = myTagged + Util::subcells(p, 2);

        domainlist2.destroy();
        proclist2.destroy();
      } else /* not initial_step */ {

        // println("Proc " << myProc << " not initial step, level "
        //         << level);
        // Level 0 differs from higher levels in that there is no
        // coarser level.
        // Hence no interpolation from a coarser level.

        // if (level == 0)
        // Should really set dt only at level 0, because otherwise
        // there may be inconsistencies.
        // If not initial step, then you can use Amr.computeNewDt(),
        // which goes through every level.
        // double dt = estTimeStep();

        // if (level == 0) parent->computeNewDt();
        // double dt = parent->getDt(level);

        // Use old dt, because you're using UOld and UNew.
        double dt = parent->getDt(level);

        /*
          Run stepSymmetric twice:
          1.  U(t-dt/2) to U(t+dt/2), dx=h,
              where U(t-dt/2) ~ (U(t)+U(t-dt))/2.
          2.  U(t-dt) to U(t+dt), dx=2*h.

                                   t + dt  =
                                           |
                    t + dt/2  =            |
                              |            |
              t  =            |            |
                 |            |            |
       t - dt/2  -  - - - ->  =            |
                 |            h            |
         t - dt  =  - - - - - - - - - ->   =
                 h                        2h

           UOld -> UNew    UTempHalf     UTemp2
        */

        LevelArray<Quantities> UCOld = (level == 0) ?
          LevelArray<Quantities>::Null() :
          amrlevel_coarser->getUOld();
        LevelArray<Quantities> UCNew = (level == 0) ?
          LevelArray<Quantities>::Null() :
          amrlevel_coarser->getUNew();

        // If level == 0 then coarse times aren't used.
        double TCOld = (level == 0) ?
          0.0 : amrlevel_coarser->getOldTime();
        double TCNew = (level == 0) ?
          0.0 : amrlevel_coarser->getNewTime();

        /*
          1.
          Interpolate from UOld and UNew into UTempHalf.
          Then advance UTempHalf from t - 0.5*dt to t + 0.5*dt.

          We want dUdt = (U(t - 0.5*dt) - U(t + 0.5*dt)) / dt
          instead of the new solution.
          In LevelGodunov.update():
          stateNew = stateOld - fluxDiff * dt/dx
          (stateNew - stateOld) / dt = - fluxDiff / dx
        */

        LevelArray<Quantities> UTempHalf(ba);

        foreach (lba, myBoxes) {
          foreach (p, ba[lba]) {
            UTempHalf[lba][p] = (UOld[lba][p] + UNew[lba][p]) * 0.5;
          }
        }

        LevelGodunov *lgTag =
          new LevelGodunov(ba, baCoarse, dProblem, nRefineCoarser,
                           dx, bc);

        LevelArray<Quantities> dUdt(ba);

        // advance
        lgTag->stepSymmetricDiff(dUdt, UTempHalf, UCOld, UCNew,
                                 dt * 0.5, TCOld, TCNew, dt);

        /*
          2.
          Average UOld to a coarsened version, UTemp2, and
          then advance UTemp2 from  t - dt  to  t + dt.
        */


        ndarray<rectdomain<SPACE_DIM>, 1> domainlist2(ba.indices());

        ndarray<int, 1> proclist2(ba.indices());

        proclist2.copy(ba.proclist());

        point<SPACE_DIM> TWO = point<SPACE_DIM>::all(2);

        foreach (ind, ba.indices())
          domainlist2[ind] = ba[ind] / TWO;

        BoxArray ba2(domainlist2, proclist2);

        // my2Boxes should be the same as myBoxes
        domain<1> my2Boxes = ba2.procboxes(myProc);

        LevelArray<Quantities> UTemp2(ba2);

        foreach (lba, my2Boxes) {
          foreach (p, ba2[lba]) {
            UTemp2[lba][p] =
              Quantities::Average(UOld[lba].constrict(Util::subcells(p, 2)));
          }
        }

        // println("ba2 = " << ba2);

        delete lgTag;
        lgTag = new LevelGodunov(ba2, baCoarse, dProblem / TWO,
                                 nRefineCoarser / 2, dx * 2.0, bc);

        LevelArray<Quantities> dU2dt(ba2);

        // advance
        lgTag->stepSymmetricDiff(dU2dt, UTemp2, UCOld, UCNew,
                                 oldTime, TCOld, TCNew, dt * 2.0);
        // is oldTime correct?


        /*
          Find maximum of dU2dt
        */

        double maxdU2dtLocal = 0.0;
        foreach (lba, my2Boxes) {
          foreach (p, ba2[lba]) {
            double thisnorm = fabs(dU2dt[lba][p][tagvar]);
            if (thisnorm > maxdU2dtLocal)
              maxdU2dtLocal = thisnorm;
          }
        }
        // println("maxdU2dtLocal = " << maxdU2dtLocal);
        double maxdU2dt = reduce::max(maxdU2dtLocal);

        // find max of dUdt

        double maxdUdtLocal = 0.0;
        foreach (lba, myBoxes) {
          foreach (p, ba[lba]) {
            double thisnorm = fabs(dUdt[lba][p][tagvar]);
            if (thisnorm > maxdUdtLocal)
              maxdUdtLocal = thisnorm;
          }
        }
        // println("maxdUdtLocal = " << maxdUdtLocal);

        /*
          Truncation error estimate is average(dUdt) - dU2dt.
          We take the norm to make things simpler.
        */

        LevelArray<double> truncError(ba2);

        // mixing my2Boxes and myBoxes, but they should be the same
        foreach (lba, my2Boxes) {
          foreach (p, ba2[lba]) {
            truncError[lba][p] =
              fabs((Quantities::Average(dUdt[lba].constrict(Util::subcells(p, 2)))
                    - dU2dt[lba][p])[tagvar]);
          }
        }

        barrier();

        /*
          Copy truncation error.
        */

        truncErrorCopied = LevelArray<double>(ba2);
        foreach (lba, myBoxes) {
          foreach (p, ba2[lba]) {
            truncErrorCopied[lba][p] = truncError[lba][p];
          }
        }
        barrier();
        // println("level " << level <<
        //         " err boxes:  " << truncErrorCopied.boxArray());

        /*
          At coarse-fine boundaries, multiply truncation error by
          perimeter / volume.

          Do the same at physical boundaries?
        */

        // Find total number of cells in the coarsened grids.
        int volumeLocal = 0;
        foreach (lba, myBoxes)
          volumeLocal += ba[lba].size();
        int volume = reduce::add(volumeLocal);

        /*
          Adjust truncation error at cells on interface with the
          next _finer_ level, if any (level < finest_level).
        */

        // println("Proc " << myProc << " fine adjust, level " <<
        //         level);
        if (!useNoFineInterface && finerLevel != NULL) {
          finePerimeter = finerLevel->getCoarsePerimeter();
          // convert to ba2 scale
          fineInterface = finerLevel->getCoarseInterfaceOff();
          int coarsenRatio = parent->getRefRatio(level);
          domain<SPACE_DIM> fineInterface2 =
            Util::coarsen(fineInterface, coarsenRatio * 2);
          double adjustment =
            ((double) finePerimeter) / ((double) volume);
          if (myProc == SPECIAL_PROC)
            println("level " << level << " fine adjustment == " <<
                    adjustment);

          int affectedLocal = 0;
          domain<SPACE_DIM> affectedDomain = Util::EMPTY;
          foreach (lba, my2Boxes) {
            affectedDomain =
              affectedDomain + (fineInterface2 * ba2[lba]);
            foreach (p, fineInterface2 * ba2[lba]) {
              truncError[lba][p] = truncError[lba][p] * adjustment;
              affectedLocal++;
            }
          }
          int affected = reduce::add(affectedLocal);
          /*
            if (myProc == SPECIAL_PROC) {
              println("level " << level << " fine adjustment on " <<
                      affected << " boxes:");
              println(affectedDomain);
            }
          */

        } else {
          // println("Finer level than " << level << " is null");
        }

        /*
          Adjust truncation error at cells on interface with the
          next _coarser_ level, if any (level > 0).
        */

        // println("Proc " << myProc << " coarse adjust, level " <<
        //         level);
        if (!useNoCoarseInterface && level > 0) {
          // Should have coarsePerimeter, coarseInterface.
          domain<SPACE_DIM> coarseInterface2 =
            Util::coarsen(coarseInterface, 2);
          double adjustment =
            ((double) coarsePerimeter) / ((double) volume);
          if (myProc == SPECIAL_PROC)
            println("level " << level << " coarse adjustment == " <<
                    adjustment);

          int affectedLocal = 0;
          domain<SPACE_DIM> affectedDomain = Util::EMPTY;
          foreach (lba, my2Boxes) {
            affectedDomain += (coarseInterface2 * ba2[lba]);
            foreach (p, coarseInterface2 * ba2[lba]) {
              truncError[lba][p] = truncError[lba][p] * adjustment;
              affectedLocal++;
            }
          }
          int affected = reduce::add(affectedLocal);
          /*
            if (myProc == SPECIAL_PROC) {
              println("level " << level << " coarse adjustment on "
                      << affected << " boxes");
              println(affectedDomain);
            }
          */
        }

        barrier();

        /*
          Copy adjusted truncation error.
        */

        truncErrorCopied2 = LevelArray<double>(ba2);
        foreach (lba, myBoxes) {
          foreach (p, ba2[lba]) {
            truncErrorCopied2[lba][p] = truncError[lba][p];
          }
        }
        barrier();
        // println("level " << level <<
        //         " err boxes:  " << truncErrorCopied2.boxArray());

        /*
          Tag where truncError >= epsilon * maxdU2dt
        */

        foreach (lba, my2Boxes) {
          foreach (p, ba2[lba]) {
            if (truncError[lba][p] > minRelTrunc * maxdU2dt)
              foreach (p2, (baseSquare + p))
                myTagged = myTagged + Util::subcells(p2, 2);
          }
        }

        domainlist2.destroy();
        proclist2.destroy();
      } // if (initial_step)

    } // if (useGradient)
  } // scope for new objects using tagRegion
  // RegionUtil::delete(tagRegion); // REGION
  oss.str("");
  oss <<"got tagged for " << myProc << ": " << myTagged.size()
      << " points";
  Logger::append(oss.str());
  // println("got tagged for proc " << myProc << ": " <<
  //         myTagged.size() << " points");
  // println("-> " << myTagged);
  Logger::barrier();

  domain<SPACE_DIM> tagged = computeUnion(myTagged);

  Logger::append("got tagged");

  return tagged;
}

domain<SPACE_DIM> HSCLLevel::tagBoxGradient(ndarray<Quantities,
                                            SPACE_DIM, global> U,
                                            rectdomain<SPACE_DIM> box) {
  // println("Tagging cells in " << box);
  domain<SPACE_DIM> taggedDomain = Util::EMPTY;
  rectdomain<SPACE_DIM> baseSquare =
    RD(Util::MINUS_ONE, Util::ONE + Util::ONE);
  /*
    Look at every point  p  in every box.
    If the relative difference in variable tagvar
    between  p  and one of its neighbors (two in each direction)
    exceeds the threshold minRelDiff,
    then tag p and all of the neighbors of p, including diagonal
    ones.
  */
  foreach (p, box) {
    double var0 = U[p][tagvar];  // conserved component tagvar=0 is density
    // println("density" << p << " = " << var0);
    foreach (pdim, Util::DIMENSIONS) {
      foreach (pdir, Util::DIRECTIONS) {
        point<SPACE_DIM> pnew =
          p + point<SPACE_DIM>::direction(pdim[1] * pdir[1]);
        if (box.contains(pnew)) {
          double var1 = U[pnew][tagvar];
          double relDiff = 2.0 * fabs(var0 - var1) /
            (fabs(var0) + fabs(var1));
          // if (relDiff > 0.0) println("relDiff = " << relDiff);
          if (relDiff > minRelDiff)
            taggedDomain = taggedDomain + (baseSquare + p);
        }
      }
    }
  }
  return taggedDomain;
}

void HSCLLevel::initGrids(InitialPatch &initializer) {
  initData(initializer);
  BoxArray baNewCoarse = ba;
  point<SPACE_DIM> extentLevel = parent->getExtent();
  HSCLLevel *coarserLevel = this;
  ostringstream oss;
  for (int lev = 1; lev <= parent->finestLevel(); lev++) {
    int levelm1 = lev - 1;

    ndarray<domain<SPACE_DIM>, 1> tags(RD(levelm1, levelm1+1));

    tags[levelm1] = parent->getLevel(levelm1)->tag();
    ndarray<ndarray<rectdomain<SPACE_DIM>, 1>, 1> boxes =
      MeshRefine::refine(parent, tags);

    rectdomain<SPACE_DIM> finerDomain = parent->getDomain(lev);
    BoxArray baNew = assignBoxes(boxes[lev], finerDomain);
    // println("************ Level " << lev << " boxes: " << baNew);

    oss.str("");
    oss << "getting new HSCLLevel " << lev;
    Logger::append(oss.str());
    // println("new HSCLLevel " << lev);
    HSCLLevel *newLevel =
      new HSCLLevel(parent, lev, baNew, baNewCoarse,
                    finerDomain, parent->getRefRatio(lev-1),
                    parent->getDx(lev), bc);
    // println("initData");
    newLevel->initData(initializer);
    Logger::barrier();
    coarserLevel->setFinerLevel(newLevel);
    coarserLevel = newLevel; // for next iteration
    // println("set finerLevel for level " << lev);
    // println("parent->setLevel");
    parent->setLevel(lev, newLevel);
    Logger::barrier();
    baNewCoarse = baNew;
  }
  parent->writeError();
}

BoxArray HSCLLevel::assignBoxes(ndarray<rectdomain<SPACE_DIM>,
                                1> &boxes,
                                rectdomain<SPACE_DIM> dProblemLevel) {
  BoxArray baNew;

  // println("HSCLLevel.assignBoxes():  new tempRegion");
  // SharedRegion tempRegion = new SharedRegion(); // REGION
  // { // scope for new objects using tempRegion
  int numProcs = THREADS;
  rectdomain<1> procTeam = RD(0, numProcs);

  ndarray<domain<1>, 1> procBoxes(procTeam);

  rectdomain<1> boxindices = boxes.domain();

  ndarray<int, 1> proclist;
  if (myProc == SPECIAL_PROC) {
    /*
      Compute estimated load of each new patch.
      The load associated with a box is taken to be the size of
      box.accrete(2) * dProblemLevel
                  ^
                  2 ghost cells in each direction
    */
    ndarray<int, 1> load(boxindices);
    foreach (indbox, boxindices) {
      load[indbox] =
        (boxes[indbox].accrete(2) * dProblemLevel).size();
      // println("load" << indbox << " = " << load[indbox]);
    }
    proclist = LoadBalance::balance(load, numProcs);
    load.destroy();
  }

  Logger::barrier();
  Logger::append("balanced load");

  rectdomain<1> boxinds = broadcast(boxindices, SPECIAL_PROC);

  if (myProc != SPECIAL_PROC)
    proclist.create(boxinds);
  // proclistAll will be used in BoxArray, so don't put in tempRegion
  ndarray<int, 1> proclistAll(boxinds);
  foreach (indbox, boxinds)
    proclistAll[indbox] = broadcast(proclist[indbox], SPECIAL_PROC);

  // ndarray<rectdomain<SPACE_DIM>, 1> boxes,
  // ndarray<int, 1> proclistAll
  baNew = BoxArray(boxes, proclistAll);
  // }
  // println("HSCLLevel.assignBoxes():  deleting tempRegion");
  // RegionUtil::delete(tempRegion); // REGION
  procBoxes.destroy();
  proclist.destroy(); // only if not SPECIAL_PROC?
  return baNew;
}

LevelArray<Quantities> HSCLLevel::regrid(BoxArray &Ba,
                                         const BoxArray &BaCoarse,
                                         LevelArray<Quantities> &UTempCoarse) {
  // println("Entering HSCLLevel.regrid() for level " << level);

  // Save existing UNew in UTemp.
  // Note that UNew.boxArray() isn't stored in a region.
  // println("Making UTemp");
  LevelArray<Quantities> UTemp(UNew.boxArray());
  // println("Copying UTemp");
  UTemp.copy(UNew);
  // println("Copied UTemp");

  ba = Ba;
  baCoarse = BaCoarse;
  // println("Level " << level << " new coarse grids " << baCoarse);
  // println("Level " << level << " new fine grids " << ba);

  // discard previous UOld
  UOld.destroy();
  UOld = LevelArray<Quantities>(ba);
  // LevelArray<Quantities> UTemp = UNew;  // save UNew
  UNew.destroy();
  UNew = LevelArray<Quantities>(ba);
  fReg = baCoarse.isNull() ? NULL :
    new FluxRegister(ba, baCoarse, dProblem, nRefineCoarser);
  myBoxes = ba.procboxes(myProc);

  // Special to HSCL.

  /*
    To update lg, you need only ba and baCoarse.
    But we also use the old PWLFillPatch object in lg to get new
    UNew. Why? Because it contains the CoarseSlopes method, which is
    in PWLFillPatch because it uses adjacentCoarseBoxes.
  */
  // println("lg->regrid()");
  if (level == 0) {
    UNew = UTemp;
  } else {
    lg->regrid(UNew, UTemp, UTempCoarse);  // fill in UNew
  }

  // println("setting new lg");
  delete lg;
  lg = new LevelGodunov(ba, baCoarse, dProblem, nRefineCoarser, dx,
                        bc);

  // println("Level " << level <<
  //         " reassigning myRegion to newRegion");

  computePerimeter();

  // println("Done with HSCLLevel.regrid()");
  return UTemp;
}

void HSCLLevel::write(const string &outputfile) {
  // println("HSCLLevel.write():  new writeRegion");
  // PrivateRegion writeRegion = new PrivateRegion(); // REGION

  // { // scope for new objects using writeRegion
  /*
    Open output file.
  */

  println("Opening file " << outputfile << " for output");
  ofstream out(outputfile);

  /*
    Given an output stream, we dump the following
    (1 per line):
    (1) Number of states (int)
    (2) Name of state (astring)
    (3) Number of dimensions (int)
    (4) Time (REAL)
    (5) Finest level written (int)
    (6) Loside prob domain loc in each direction (BL_SPACEDIM*(REAL*))
    (7) Hiside prob domain loc in each direction (BL_SPACEDIM*(REAL*))
    (8) Refinement ratio ((numLevels-1)*(int))
    (9) Domain of each level (numLevels* (BOX) )
    (10) Steps taken on each level (numLevels*(int) )
    (11) Grid spacing (numLevels*(BL_SPACEDIM*(int))) [\n at each level]
    (12) Coordinate flag (int)
    (13) Boundary info (int)
    (14) For each level,
         (a) level number and # of grids per level (2*int)
         For each grid,
             (1) Grid indices (BOX)
             (2) Level (int)
             (3) Steps taken on this grid (int)
             (4) Time for this grid (REAL)
             (5) Physical location of grid corners (BL_SPACEDIM*(REAL))
             (6) writeOn data dump of each FAB of data on that grid
  */

  // println("Writing output");

  // (1) Number of states (int)
  out << (Quantities::length() + 1) << endl;
  // also have level as a state

  // (2) Name of state (astring) [for each state]
  out << "density" << endl;
  if (SPACE_DIM >= 1) out << "x_velocity" << endl;
  if (SPACE_DIM >= 2) out << "y_velocity" << endl;
  if (SPACE_DIM >= 3) out << "z_velocity" << endl;
  out << "pressure" << endl;
  out << "level" << endl; // should be here?

  // (3) Number of dimensions (int)
  out << SPACE_DIM << endl;

  // (4) Time (REAL)
  double time = parent->getCumtime();
  out << time << endl;

  // (5) Finest level written (int)
  int finest_level = parent->finestLevel();
  out << finest_level << endl;

  // (6) Loside prob domain loc in each direction (BL_SPACEDIM*(REAL*))
  ndarray<ndarray<double, 1>, 1> ends = parent->getEndsNonsingle();
  ndarray<double, 1> endsLow = ends[-1];
  out << doubleArraystring(endsLow) << endl;

  // (7) Hiside prob domain loc in each direction (BL_SPACEDIM*(REAL*))
  out << doubleArraystring(ends[+1]) << endl;

  // (8) Refinement ratio ((numLevels-1)*(int))
  for (int h = 0; h <= finest_level - 1; h++) {
    out << parent->getRefRatio(h) << ' ';
  }
  out << endl;

  // (9) Domain of each level (numLevels* (BOX) )
  point<SPACE_DIM> extentLevel = parent->getExtent();
  out << domainstring(extentDomain(extentLevel));
  for (int h = 1; h <= finest_level; h++) {
    extentLevel =
      extentLevel * point<SPACE_DIM>::all(parent->getRefRatio(h-1));
    out << domainstring(extentDomain(extentLevel));
  }
  out << endl;

  // (10) Steps taken on each level (numLevels*(int) )
  for (int h = 0; h <= finest_level; h++) {
    out << parent->getLevelSteps(h) << ' ';
  }
  out << endl;

  // (11) Grid spacing (numLevels*(BL_SPACEDIM*(real))) [at each level]
  double dxLevel = dx;
  for (int h = 0; h < finest_level; h++) {
    for (int dim = 1; dim <= SPACE_DIM; dim++) {
      out << dxLevel << ' ';
    }
    out << endl;
    dxLevel /= (double) parent->getRefRatio(h);
  }
  // and now finest level
  for (int dim = 1; dim <= SPACE_DIM; dim++) {
    out << dxLevel << ' ';
  }
  out << endl;

  // (12) Coordinate flag (int)
  out << '0' << endl;  // 0 = cartesian

  // (13) Boundary info (int)
  out << '0' << endl;  // 0 = no boundary info

  // println("Going through all processes...");

  for (int height = 0; height <= finest_level; height++) {

    // (14) For each level,

    // AmrLevel myLevel = parent->getLevel(height);
    AmrLevel *myLevel = parent->getLevelNonsingle(height);
    // AmrLevel myLevel = amr_level[height];
    LevelArray<Quantities> U = myLevel->getUNew();
    BoxArray ba = myLevel->getBa();
    dxLevel = myLevel->getDx();
    int steps = parent->getLevelSteps(height);

    // (a) level number and # of grids per level (2*int)

    rectdomain<1> boxIndices = ba.indices();
    out << height << ' ';
    int totpatches = boxIndices.size();
    out << totpatches << endl;

    println("Writing patches at level " << height << ":");
    foreach (indbox, boxIndices) {
      // For each grid,
      ndarray<Quantities, SPACE_DIM, global> gridData = U[indbox];
      rectdomain<SPACE_DIM> box = ba[indbox];

      print(" " << domainstringShort(box));

      // (1) Grid indices (BOX)
      out << domainstring(box) << endl;

      // (2) Level (int)
      out << height << endl;

      // (3) Steps taken on this grid (int)
      out << steps << endl;  // Chombo plot files write 0

      // (4) Time for this grid (REAL)
      out << time << endl;

      // (5) Physical location of grid corners (BL_SPACEDIM*(REAL))
      for (int dim = 1; dim <= SPACE_DIM; dim++) {
        double loside = endsLow[dim] +
          dxLevel * (double) box.min()[dim];
        double hiside = endsLow[dim] +
          dxLevel * (double) (box.max()[dim] + 1);
        out << loside << ' ' << hiside << endl;
      }

      // (6) writeOn data dump of each FAB of data on that grid
      myLevel->writeOn(out, gridData, height);

    } // foreach (indbox in boxIndices)
    println("");
  } // for (int height = 0; height <= finest_level; height++)

  out.close();

  // } // scope for new objects using writeRegion
  // println("HSCLLevel.write():  deleting writeRegion");
  // RegionUtil::delete(writeRegion); // REGION
}

void HSCLLevel::writeOn(ostream &out,
                        ndarray<Quantities, SPACE_DIM,
                        global> &gridData,
                        int height) {
  // println("HSCLLevel.writeOn():  new writeOnRegion");
  // PrivateRegion writeOnRegion = new PrivateRegion(); // REGION
  // { // scope for new objects using writeOnRegion
  rectdomain<SPACE_DIM> D = gridData.domain();
  ostringstream oss;
  oss << domainstring(D) << " 1";
  point<SPACE_DIM> Dmin = D.min(), Dmax = D.max();
  ndarray<Quantities, SPACE_DIM> primData(D);
  lg->consToPrim(gridData, primData);

  for (int field = 0; field < Quantities::length(); field++) {

    out << oss.str() << endl;

    point<SPACE_DIM> p = Dmin;
    while (true) {
      out << primData[p][field] << endl;
      int dim;
      for (dim = 1; dim <= SPACE_DIM; dim++) {
        point<SPACE_DIM> unit = point<SPACE_DIM>::direction(dim);
        if (p[dim] + 1 <= Dmax[dim]) {
          p = p + unit;
          break;
        } else {
          // set p[dim] to Dmin[dim]
          p = (Util::ONE - unit) * p + unit * Dmin[dim];
          continue;
        }
      }
      if (dim == SPACE_DIM + 1) break;
    }
  }

  // height field is constant
  out << oss.str() << endl;
  foreach (p, D)
    out << height << endl;

  // } // scope for new objects using writeOnRegion
  // println("HSCLLevel.writeOn():  deleting writeOnRegion");
  // RegionUtil::delete(writeOnRegion); // REGION
  primData.destroy();
}

void HSCLLevel::writeError(const string &outputfile) {
    // println("HSCLLevel.write():  new writeRegion");
  // PrivateRegion writeRegion = new PrivateRegion(); // REGION

  // { // scope for new objects using writeRegion
  /*
    Open output file.
  */

  println("Opening file " << outputfile << " for output");
  ofstream out(outputfile);

  /*
    Given an output stream, we dump the following
    (1 per line):
    (1) Number of states (int)
    (2) Name of state (astring)
    (3) Number of dimensions (int)
    (4) Time (REAL)
    (5) Finest level written (int)
    (6) Loside prob domain loc in each direction (BL_SPACEDIM*(REAL*))
    (7) Hiside prob domain loc in each direction (BL_SPACEDIM*(REAL*))
    (8) Refinement ratio ((numLevels-1)*(int))
    (9) Domain of each level (numLevels* (BOX) )
    (10) Steps taken on each level (numLevels*(int) )
    (11) Grid spacing (numLevels*(BL_SPACEDIM*(int))) [\n at each level]
    (12) Coordinate flag (int)
    (13) Boundary info (int)
    (14) For each level,
         (a) level number and # of grids per level (2*int)
         For each grid,
             (1) Grid indices (BOX)
             (2) Level (int)
             (3) Steps taken on this grid (int)
             (4) Time for this grid (REAL)
             (5) Physical location of grid corners (BL_SPACEDIM*(REAL))
             (6) writeOn data dump of each FAB of data on that grid
  */

  // println("Writing output");

  // (1) Number of states (int)
  out << 2 << endl;  // truncation error before and after adjustment

  // (2) Name of state (astring) [for each state]
  out << "error" << endl;
  out << "error2" << endl;

  // (3) Number of dimensions (int)
  out << SPACE_DIM << endl;

  // (4) Time (REAL)
  double time = parent->getCumtime();
  out << time << endl;

  // (5) Finest level written (int)
  int finest_level = parent->finestLevel() - 1;
  // -1 because no error calculated at finest level
  out << finest_level << endl;

  // (6) Loside prob domain loc in each direction (BL_SPACEDIM*(REAL*))
  ndarray<ndarray<double, 1>, 1> ends = parent->getEndsNonsingle();
  ndarray<double, 1> endsLow = ends[-1];
  out << doubleArraystring(endsLow) << endl;

  // (7) Hiside prob domain loc in each direction (BL_SPACEDIM*(REAL*))
  out << doubleArraystring(ends[+1]) << endl;

  // (8) Refinement ratio ((numLevels-1)*(int))
  for (int h = 0; h <= finest_level - 1; h++) {
    out << parent->getRefRatio(h) << ' ';
  }
  out << endl;

  // (9) Domain of each level (numLevels* (BOX) )
  point<SPACE_DIM> extentLevel = parent->getExtent() / TWO;
  out << domainstring(extentDomain(extentLevel));
  for (int h = 1; h <= finest_level; h++) {
    extentLevel =
      (extentLevel * point<SPACE_DIM>::all(parent->getRefRatio(h-1)));
    out << domainstring(extentDomain(extentLevel));
  }
  out << endl;

  // (10) Steps taken on each level (numLevels*(int) )
  for (int h = 0; h <= finest_level; h++) {
    out << parent->getLevelSteps(h) << ' ';
  }
  out << endl;

  // (11) Grid spacing (numLevels*(BL_SPACEDIM*(real))) [at each level]
  double dxLevel = dx * 2.0;  // multiply by TWO
  for (int h = 0; h < finest_level; h++) {
    for (int dim = 1; dim <= SPACE_DIM; dim++) {
      out << dxLevel << ' ';
    }
    out << endl;
    dxLevel /= (double) parent->getRefRatio(h);
  }
  // and now finest level
  for (int dim = 1; dim <= SPACE_DIM; dim++) {
    out << dxLevel << ' ';
  }
  out << endl;

  // (12) Coordinate flag (int)
  out << '0' << endl;  // 0 = cartesian

  // (13) Boundary info (int)
  out << '0' << endl;  // 0 = no boundary info

  // println("Going through all processes...");

  for (int height = 0; height <= finest_level; height++) {

    // (14) For each level,

    // AmrLevel myLevel = parent->getLevel(height);
    AmrLevel *myLevel = parent->getLevelNonsingle(height);
    // AmrLevel myLevel = amr_level[height];
    LevelArray<double> err = myLevel->getTruncErrorCopied();
    LevelArray<double> err2 = myLevel->getTruncErrorCopied2();
    BoxArray ba = myLevel->getBa();
    dxLevel = myLevel->getDx() * 2.0;  // mult by TWO
    int steps = parent->getLevelSteps(height);

    // (a) level number and # of grids per level (2*int)

    rectdomain<1> boxIndices = ba.indices();
    out << height << ' ';
    int totpatches = boxIndices.size();
    out << totpatches << endl;

    println("Writing error patches at level " << height << ":");
    foreach (indbox, boxIndices) {
      // For each grid,
      ndarray<double, SPACE_DIM, global> errorData = err[indbox];
      ndarray<double, SPACE_DIM, global> errorData2 = err2[indbox];
      rectdomain<SPACE_DIM> box = ba[indbox] / TWO;

      print(" " << domainstringShort(box));

      // (1) Grid indices (BOX)
      out << domainstring(box) << endl;

      // (2) Level (int)
      out << height << endl;

      // (3) Steps taken on this grid (int)
      out << steps << endl;  // Chombo plot files write 0

      // (4) Time for this grid (REAL)
      out << time << endl;

      // (5) Physical location of grid corners (BL_SPACEDIM*(REAL))
      for (int dim = 1; dim <= SPACE_DIM; dim++) {
        double loside = endsLow[dim] +
          dxLevel * (double) box.min()[dim];
        double hiside = endsLow[dim] +
          dxLevel * (double) (box.max()[dim] + 1);
        out << loside << ' ' << hiside << endl;
      }

      // (6) writeOn data dump of each FAB of data on that grid
      myLevel->writeErrorOn(out, errorData, height);
      myLevel->writeErrorOn(out, errorData2, height);

    } // foreach (indbox in boxIndices)
    println("");
  } // for (int height = 0; height <= finest_level; height++)

  out.close();
  // } // scope for new objects using writeRegion
  // println("HSCLLevel.write():  deleting writeRegion");
  // RegionUtil::delete(writeRegion); // REGION
}

void HSCLLevel::writeErrorOn(ostream &out,
                             ndarray<double, SPACE_DIM,
                             global> &errorData,
                             int height) {
  // println("HSCLLevel.writeOn():  new writeOnRegion");
  // PrivateRegion writeOnRegion = new PrivateRegion(); // REGION
  // { // scope for new objects using writeOnRegion
  rectdomain<SPACE_DIM> D = errorData.domain();
  ostringstream oss;
  oss << domainstring(D) << " 1";
  point<SPACE_DIM> Dmin = D.min(), Dmax = D.max();

  out << oss.str() << endl;

  point<SPACE_DIM> p = Dmin;
  while (true) {
    out << errorData[p] << endl;
    int dim;
    for (dim = 1; dim <= SPACE_DIM; dim++) {
      point<SPACE_DIM> unit = point<SPACE_DIM>::direction(dim);
      if (p[dim] + 1 <= Dmax[dim]) {
        p = p + unit;
        break;
      } else {
        // set p[dim] to Dmin[dim]
        p = (Util::ONE - unit) * p + unit * Dmin[dim];
        continue;
      }
    }
    if (dim == SPACE_DIM + 1) break;
  }
  // } // scope for new objects using writeOnRegion
  // println("HSCLLevel.writeOn():  deleting writeOnRegion");
  // RegionUtil::delete(writeOnRegion); // REGION
}
