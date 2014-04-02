#include "PWLFillPatch.h"

PWLFillPatch::PWLFillPatch(BoxArray &Ba, BoxArray &BaCoarse,
                           rectdomain<SPACE_DIM> DProblem, int NRefine,
                           int InnerRadius, int OuterRadius) {
  ba = Ba;
  baCoarse = BaCoarse;
  dProblem = DProblem;
  nRefine = NRefine;
  nRefineP = point<SPACE_DIM>::all(nRefine);
  dRefine = (double) nRefine;
  innerRadius = InnerRadius;
  outerRadius = OuterRadius;

  rectdomain<1> boxInds = ba.indices();
  rectdomain<1> boxCInds = baCoarse.indices();

  /*
    Calculate BInner, BOuter.
  */
  BInner = ndarray<rectdomain<SPACE_DIM>, 1>(boxInds);
  BOuter = ndarray<rectdomain<SPACE_DIM>, 1>(boxInds);
  foreach (ind, boxInds) {
    BInner[ind] = ba[ind].accrete(innerRadius) * dProblem;
    BOuter[ind] = BInner[ind].accrete(outerRadius) * dProblem;
  }

  /*
    adjacentBoxes:  set of other boxes within outer radius.
  */
  adjacentBoxes = ndarray<domain<1>, 1>(boxInds);
  foreach (ind, boxInds) {
    adjacentBoxes[ind] = RD(0, 0);
    // foreach (indOther, boxInds - [ind:ind])
    foreach (indOther, boxInds)
      if (indOther != ind)
        if (! (ba[indOther] * BOuter[ind]).is_empty())
          adjacentBoxes[ind] = adjacentBoxes[ind] +
            RD(indOther, indOther+1);
  }

  /*
    adjacentCoarseBoxes:  set of other coarse boxes touching coarse boxes.
  */
  adjacentCoarseBoxes = ndarray<domain<1>, 1>(boxCInds);
  foreach (ind, boxCInds) {
    adjacentCoarseBoxes[ind] = RD(0, 0);
    rectdomain<SPACE_DIM> accreted = baCoarse[ind].accrete(1);
    // foreach (indOther, boxCInds - [ind:ind])
    foreach (indOther, boxCInds)
      if (indOther != ind)
        if (! (baCoarse[indOther] * accreted).is_empty())
          adjacentCoarseBoxes[ind] =
            adjacentCoarseBoxes[ind] + RD(indOther, indOther+1);
  }

  /*
    coarserBoxes:  set of coarser boxes within inner radius.
  */
  coarserBoxes = ndarray<domain<1>, 1>(boxInds);
  foreach (ind, boxInds) {
    // String slist = "coarserBoxes of " << ba[ind] << ":";
    coarserBoxes[ind] = RD(0, 0);
    foreach (indC, boxCInds)
      if (! (baCoarse[indC] * (BInner[ind] / nRefineP)).is_empty()) {
        coarserBoxes[ind] = coarserBoxes[ind] + RD(indC, indC+1);
        // slist = slist << " " << baCoarse[indC];
      }
    // println(slist);
  }
}

void PWLFillPatch::fill(LevelArray<Quantities> &U,
                        LevelArray<Quantities> &UCOld,
                        LevelArray<Quantities> &UCNew,
                        ndarray<Quantities, SPACE_DIM> &UPatch,
                        point<1> PatchIndex, double Time,
                        double TCOld, double TCNew) {

  rectdomain<SPACE_DIM> baNew = UPatch.domain();
  domain<SPACE_DIM> unfilled = baNew;

  /*
    Fill core by copying U[PatchIndex].
  */

  UPatch.copy(U[PatchIndex]);
  unfilled = unfilled - ba[PatchIndex];

  /*
    Fill within _outer_ radius by copying from adjacent boxes,
    as much as possible.
  */

  foreach (indadj, adjacentBoxes[PatchIndex]) {
    if (! (baNew * ba[indadj]).is_empty()) {
      // this may belong to another process
      ndarray<Quantities, SPACE_DIM, global> UAdj = U[indadj];
      UPatch.copy(UAdj);
      unfilled = unfilled - ba[indadj];
    }
  }

  // Sometimes get all.
  if (unfilled.is_empty()) return;

  /*
    Fill all unfilled cells within _inner_ radius by interpolating
    from coarser data.
  */

  // println("PWLFillPatch.fill(" << baNew << "):  Must fill in "
  //         << unfilled);

  domain<SPACE_DIM> unfilledC =
    (unfilled * BInner[PatchIndex]) / nRefineP;
  // BInner[PatchIndex] is contained in baNew
  rectdomain<SPACE_DIM> BInnerC = BInner[PatchIndex] / nRefineP;
  ndarray<Quantities, SPACE_DIM> UC(BInnerC);

  double wtNew = (TCNew == TCOld) ? 1.0 :
    (Time - TCOld) / (TCNew - TCOld);
  double wtOld = 1.0 - wtNew;
  // println("wtOld = " << wtOld << " and wtNew = " << wtNew);

  foreach (indc, coarserBoxes[PatchIndex]) {

    // UCOld[indc] and UCNew[indc] may belong to other processes

    foreach (cellC, unfilledC * baCoarse[indc]) {
      /*
      println("Filling " << cellC << " in " << baCoarse[indc]);
      println("UCOld" << indc << " domain = " << UCOld[indc].domain());
      println("UCNew" << indc << " domain = " << UCNew[indc].domain());
      println("UC domain = " << UC.domain());
      */
      UC[cellC] =
        UCOld[indc][cellC] * wtOld + UCNew[indc][cellC] * wtNew;

      // Find slopes of UC in each direction at each coarse cell.

      ndarray<Quantities, 1> UCslopes =
        coarseSlopes(UCOld, UCNew, indc, cellC, wtOld, wtNew);
      foreach (cell, unfilled * Util::subcells(cellC, nRefine)) {
        // Fill in cell.
        UPatch[cell] = UC[cellC];
        foreach (pdim, Util::DIMENSIONS) {
          int dim = pdim[1];
          UPatch[cell] = UPatch[cell] + UCslopes[pdim] *
            (((double) cell[dim] + 0.5) / dRefine -
             ((double) cellC[dim] + 0.5));
        }
      }
      UCslopes.destroy();
    }
  }

  UC.destroy();

  // There may still be unfilled cells within _outer_ radius.
  // These will be filled in with extend().
}

ndarray<Quantities, 1> PWLFillPatch::coarseSlopes(LevelArray<Quantities> &UCOld,
                                                  LevelArray<Quantities> &UCNew,
                                                  point<1> indc,
                                                  point<SPACE_DIM> cellC,
                                                  double wtOld,
                                                  double wtNew) {

  // From box indc of UCOld (weight wtOld) and UCNew (weight wtNew)
  // get slopes in cell cellC.
  ndarray<Quantities, 1> UCslopes(Util::DIMENSIONS);
  ndarray<Quantities, 1> UCadj(Util::DIRECTIONS);
  ndarray<bool, 1> foundAdjC(Util::DIRECTIONS);

  // Check all directions for neighbors.

  Quantities UCme =
    UCOld[indc][cellC] * wtOld + UCNew[indc][cellC] * wtNew;

  foreach (pdim, Util::DIMENSIONS) {
    foreach (pdir, Util::DIRECTIONS) { // pdir in {-1, 1}
      point<SPACE_DIM> adjC =
        cellC + point<SPACE_DIM>::direction(pdir[1] * pdim[1]);

      point<1> indAdj;

      foundAdjC[pdir] = false;
      // ERROR:  not baCoarse but UCNew.boxArray()
      if (baCoarse[indc].contains(adjC)) {
        foundAdjC[pdir] = true;
        indAdj = indc;
      } else {
        // Look in adjacent boxes.
        foreach (indAdjSearch, adjacentCoarseBoxes[indc])
          // ERROR:  not baCoarse but UCNew.boxArray()
          // You should have retained adjacentCoarseBoxes of it...
          if (baCoarse[indAdjSearch].contains(adjC)) {
            foundAdjC[pdir] = true;
            indAdj = indAdjSearch;
            break;
          }
      }

      if (foundAdjC[pdir])
        UCadj[pdir] =
          UCOld[indAdj][adjC]*wtOld + UCNew[indAdj][adjC]*wtNew;

    } // end pdir

    if (!foundAdjC[-1] && !foundAdjC[+1]) {
      // This shouldn't happen.
      println("Error:  Couldn't find either neighbor of " <<
              cellC << " in dimension " << pdim[1]);
      UCslopes[pdim] = Quantities::zero();
    } else if (foundAdjC[-1] && !foundAdjC[+1]) {
      UCslopes[pdim] = UCme - UCadj[-1];
    } else if (!foundAdjC[-1] && foundAdjC[+1]) {
      UCslopes[pdim] = UCadj[+1] - UCme;
    } else {
      UCslopes[pdim] =
        Quantities::vanLeer(UCadj[-1], UCme, UCadj[+1]);
    }
  } // end pdim

  UCadj.destroy();
  foundAdjC.destroy();
  return UCslopes;
}

void PWLFillPatch::extend(ndarray<Quantities, SPACE_DIM> &UPatch,
                          point<1> PatchIndex, int Dim,
                          int FlapRadius) {
  domain<SPACE_DIM> unfilled = UPatch.domain() - BInner[PatchIndex];

  foreach (indadj, adjacentBoxes[PatchIndex])
    unfilled = unfilled - ba[indadj];

  rectdomain<SPACE_DIM> flapBase(point<SPACE_DIM>::direction(Dim, 1),
                                 point<SPACE_DIM>::direction(Dim,
                                                             FlapRadius)
                                 + point<SPACE_DIM>::all(1));
  // = [[1 : FlapRadius], 0] or [0, [1 : FlapRadius]]

  foreach (pdir, Util::DIRECTIONS) {
    int sDim = pdir[1] * Dim;
    rectdomain<SPACE_DIM> flap =
      flapBase * point<SPACE_DIM>::direction(sDim, 1);
    // = [[1 : FlapRadius], 0] or [0, [1 : FlapRadius]]
    // or [[-FlapRadius : -1], 0] or [0, [-FlapRadius : -1]]
    foreach (borderCell, BInner[PatchIndex].border(1, sDim, 0))
      foreach (cell, unfilled * (flap + borderCell))
      UPatch[cell] = UPatch[borderCell];
  }
}

void PWLFillPatch::regrid(LevelArray<Quantities> &UNew,
                          LevelArray<Quantities> &UTemp,
                          LevelArray<Quantities> &UTempCoarse) {
  BoxArray baNew = UNew.boxArray();
  domain<1> myBoxes = baNew.procboxes(MYTHREAD);
  foreach (lba, myBoxes) {
    /*
      Fill UNew[lba].
    */
    domain<SPACE_DIM> unfilledC = baNew[lba] / nRefineP;
    /*
      First try to copy from UTemp.
    */
    foreach (lbaTemp, ba.indices()) {
      rectdomain<SPACE_DIM> intersect = baNew[lba] * ba[lbaTemp];
      if (! intersect.is_empty()) {
        UNew[lba].copy(UTemp[lbaTemp]);
        unfilledC = unfilledC - (intersect / nRefineP);
      }
    }
    // println("In " << baNew[lba] << " still unfilled:  refined " <<
    // unfilledC);
    /*
      Now fill in the rest by interpolation from UTempCoarse.
    */
    foreach (cellC, unfilledC) {
      // Find the box in baCoarse that contains cellC.
      domain<1> lbaTC = baCoarse.findCell(cellC);
      if (lbaTC.size() == 0) {
        println("Could not find " << cellC << " in baCoarse");
      } else if (lbaTC.size() > 1) {
        println("More than one box in baCoarse had " << cellC);
      } else {
        point<1> indbox = lbaTC.min();
        // Find slopes of UC in each direction at each coarse cell.
        ndarray<Quantities, 1> UCslopes =
          coarseSlopes(UTempCoarse, UTempCoarse, indbox, cellC, 1.0,
                       0.0);
        // println("Filling in subcells of " << cellC);
        foreach (cell, Util::subcells(cellC, nRefine)) {
          // Fill in cell.
          UNew[lba][cell] = UTempCoarse[indbox][cellC];
          foreach (pdim, Util::DIMENSIONS) {
            int dim = pdim[1];
            UNew[lba][cell] = UNew[lba][cell] + UCslopes[pdim] *
              (((double) cell[dim] + 0.5) / dRefine -
               ((double) cellC[dim] + 0.5));
          }
        }
        UCslopes.destroy();
      } // else:  lbaTC.size() == 1
    } // foreach (cellC, unfilledC)
  } // foreach (lba, myBoxes)
}
