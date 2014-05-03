#pragma once

/**
 * FluxRegister contains flux registers between a level and the
 * next coarser level.  The object lives on the finer level.
 *
 * @version  15 Dec 1999
 * @author   Peter McCorquodale, PWMcCorquodale@lbl.gov
 */

#include "BoxArray.h"
#include "LevelArray.h"
#include "Quantities.h"
#include "Util.h"

class FluxRegister {
private:
  /** array of boxes at this level and at the next coarser level */
  BoxArray ba, baCoarse;

  /** physical domain */
  rectdomain<SPACE_DIM> dProblem;

  /** registers indexed by [Util::DIMENSIONS][Util::DIRECTIONS] */
  ndarray<ndarray<LevelArray<Quantities>, 1>, 1> registers;

  /**
   * map into _coarsened_ problem domain,
   * indexed by [Util::DIMENSIONS][Util::DIRECTIONS][indbox]
   */
  ndarray<ndarray<ndarray<domain<SPACE_DIM>, 1>, 1>, 1> domains;

  /**
   * fineBoxes[dim][dir][indcoarsebox] is the set of indices of all
   * fine boxes that cover the given coarse box.
   * indexed by [Util::DIMENSIONS][Util::DIRECTIONS][indcoarsebox]
   */
  ndarray<ndarray<ndarray<domain<1>, 1>, 1>, 1> fineBoxes;

  /** refinement ratio between the two levels */
  int nRefine;

  /** refinement ratio between the two levels, as point */
  point<SPACE_DIM> nRefineP;

  /** refinement ratio between the two levels, as double */
  double dRefine;

  double wtFace;

public:
  /**
   * constructor
  */
  FluxRegister(BoxArray &Ba, BoxArray &BaCoarse,
               rectdomain<SPACE_DIM> DProblem, int NRefine);

  ~FluxRegister() {
    foreach (p, registers.domain()) {
      registers[p].destroy();
    }
    registers.destroy();

    foreach (p, domains.domain()) {
      foreach (q, domains[p].domain()) {
        domains[p][q].destroy();
      }
      domains[p].destroy();
    }
    domains.destroy();

    foreach (p, fineBoxes.domain()) {
      foreach (q, fineBoxes[p].domain()) {
        fineBoxes[p][q].destroy();
      }
      fineBoxes[p].destroy();
    }
    fineBoxes.destroy();
  }

  // methods

  /**
   * Initialize registers to be zero.
   */
  void clear() {
    domain<1> myBoxes = ba.procboxes(MYTHREAD);
    foreach (pdim, Util::DIMENSIONS) {
      foreach (pdir, Util::DIRECTIONS) {
        foreach (lba, myBoxes) {
          registers[pdim][pdir][lba].set(Quantities::zero());
        }
      }
    }
  }

  /**
   * Increment (from zero) the register with data from CoarseFlux,
   * which contains fluxes for baCoarse[CoarsePatchIndex].
   *
   * Recall that the flux registers live on the finer patches, so
   * you use fineBoxes to find the finer patches that are affected.
   */
  void incrementCoarse(ndarray<Quantities, SPACE_DIM> &CoarseFlux,
                       double Scale, point<1> CoarsePatchIndex,
                       int Dim, point<1> Dir) {
    int dimdir = Dim * Dir[1];
    if (! fineBoxes[Dim][Dir].domain().contains(CoarsePatchIndex) ) {
      println("fineboxes of " << dimdir << " has domain " <<
              fineBoxes[Dim][Dir].domain() << " and you want " <<
              CoarsePatchIndex);
    }
    domain<1> myfineBoxes = fineBoxes[Dim][Dir][CoarsePatchIndex];
    // println("incrementCoarse myfineBoxes = " << myfineBoxes);
    // Recall that CoarseFlux is edge-centered.
    point<SPACE_DIM> fluxOffset = Util::cell2edge[-dimdir];
    // For pC in flux register domain, use coarse flux at edge pC+fluxOff.
    foreach (indF, myfineBoxes) {
      ndarray<Quantities, SPACE_DIM, global> myregs =
        registers[Dim][Dir][indF];
      domain<SPACE_DIM> myDomain = domains[Dim][Dir][indF];
      foreach (pC, myDomain * baCoarse[CoarsePatchIndex]) {
	myregs[pC] = myregs[pC] + CoarseFlux[pC + fluxOffset] * Scale;
      }
    }
  }

  /**
   * Increment the registers with data from FineFlux, which contains
   * flux for ba[FinePatchIndex].
   */
  void incrementFine(ndarray<Quantities, SPACE_DIM> &FineFlux,
                     double Scale, point<1> FinePatchIndex,
                     int Dim, point<1> Dir) {
    // FineFlux.domain() == edgeBox:  outer edges of (ba[lba]) in sweepDim
    int dimdir = Dim * Dir[1];
    double avgwtScale = Scale * wtFace;
    // myregs:  particular side of a particular box
    ndarray<Quantities, SPACE_DIM, global> myregs =
      registers[Dim][Dir][FinePatchIndex];
    // domains[][][]:  the cells in myregs.domain() that are actually used
    foreach (pC, domains[Dim][Dir][FinePatchIndex]) {
      foreach (eF, fineEdges(pC, dimdir)) {
        myregs[pC] = myregs[pC] + FineFlux[eF] * avgwtScale;
	// Recall that FineFlux is edge-centered.
      }
    }
  }

private:
  /**
   * Return indices of finer edges adjacent to cellC,
   * where cellC is in direction Dir from these edges.
   * (This may sound backward.)
   * Called by incrementFine.
   */
  rectdomain<SPACE_DIM> fineEdges(point<SPACE_DIM> cellC, int Dir) {
    return (Util::subcells(cellC, nRefine).border(-Dir) +
	    Util::cell2edge[Dir]);
  }

public:
  /**
   * Change the conserved quantities at the coarser level
   * in accordance with flux registers.
   */
  void reflux(LevelArray<Quantities> &UCoarse) {
    domain<1> myBoxesC = baCoarse.procboxes(MYTHREAD);
    foreach (pdim, Util::DIMENSIONS) {
      foreach (pdir, Util::DIRECTIONS) {
        foreach (lbaC, myBoxesC) {
          foreach (lba, fineBoxes[pdim][pdir][lbaC]) {
            foreach (pC, (domains[pdim][pdir][lba] * baCoarse[lbaC])) {
              UCoarse[lbaC][pC] =
                UCoarse[lbaC][pC] + registers[pdim][pdir][lba][pC];
            }
          }
        }
      }
    }
  }
};
