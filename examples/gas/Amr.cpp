#include "Amr.h"
#include "MeshRefine.h"

Amr::Amr() : saved(false) {
  Util::set();

  // println("get pp for amr");
  pp = new ParmParse("amr");
  // println("read pp for amr");

  ndarray<int, 1> extentRead = pp->getIntegerArray("n_cell");
  point<SPACE_DIM> extentLocal = Util::ZERO;
  foreach (pdim, Util::DIMENSIONS) {
    int dim = pdim[1];
    extentLocal = extentLocal +
      point<SPACE_DIM>::direction(dim, extentRead[dim-1]);
  }
  extent = broadcast(extentLocal, SPECIAL_PROC);

  if (MYTHREAD == SPECIAL_PROC)
    finest_level = pp->getInteger("max_level");
  finest_level = broadcast(finest_level, SPECIAL_PROC);

  // Change ParmParse so that you can call it with reference to an
  // array that gets modified?
  ndarray<int, 1> inRefRatio = pp->getIntegerArray("ref_ratio");
  if (inRefRatio.domain().size() < finest_level) {
    println("Amr:  amr.ref_ratio needs refinement ratios " <<
            "up to level " << finest_level);
    exit(0);
  }
  ref_ratio.create(RD(0, finest_level));
  foreach (ind, ref_ratio.domain()) {
    ref_ratio[ind] = broadcast(inRefRatio[ind], SPECIAL_PROC);
  }

  // println("getting prob_lo");
  ndarray<double, 1> prob_lo = pp->getDoubleArray("prob_lo");
  if (prob_lo.domain().size() != SPACE_DIM) {
    println("Amr: See amr.prob_lo. Problem is " << SPACE_DIM <<
            "-dimensional");
  }

  // println("getting prob_hi");
  ndarray<double, 1> prob_hi = pp->getDoubleArray("prob_hi");
  if (prob_hi.domain().size() != SPACE_DIM) {
    println("Amr: See amr.prob_hi. Problem is " << SPACE_DIM <<
            "-dimensional");
  }

  // println("getting ends");
  ends.create(Util::DIRECTIONS);
  foreach (pdir, Util::DIRECTIONS) {
    ends[pdir].create(Util::DIMENSIONS);
  }
  foreach (pdim, Util::DIMENSIONS) {
    ends[-1][pdim] = broadcast(prob_lo[pdim - PT(1)], SPECIAL_PROC);
    ends[+1][pdim] = broadcast(prob_hi[pdim - PT(1)], SPECIAL_PROC);
    // println("ends" << pdim << ": " << ends[-1][pdim] << " to " <<
    //         ends[+1][pdim]);
  }

  // println("got pp stuff for amr");
  // println("finest_level = " << finest_level);
  /*
    Global arrays of Amr parameters.
  */

  // println("defining amr_level, dt_level");
  rectdomain<1> levs(0, finest_level+1);
  amr_level.create(levs);
  dt_level.create(levs);
  dx_level.create(levs);
  dProblem.create(levs);

  /*
    Find domain at each level.
  */
  point<SPACE_DIM> extentLevel = extent;
  dProblem[0] = RD(Util::ZERO, extentLevel);
  for (int lev = 1; lev <= finest_level; lev++) {
    extentLevel = extentLevel * ref_ratio[lev-1];
    dProblem[lev] = RD(Util::ZERO, extentLevel);
  }

  /*
    Find grid spacing at each level.
  */
  int dim = 1;
  dx_level[0] =
    (ends[+1][dim] - ends[-1][dim]) / (double) extent[dim];
  // Check that other dimensions agree.
  for (dim = 2; dim <= SPACE_DIM; dim++) {
    double dxdim =
      (ends[+1][dim] - ends[-1][dim]) / (double) extent[dim];
    if (dxdim != dx_level[0]) {
      println("Amr:  Aspect ratio of problem box is incorrect.");
      println("Ratio " << dx_level[0] << " in dimension 1 and " <<
              dxdim << " in dimension " << dim);
      exit(0);
    }
  }
  for (int lev = 1; lev <= finest_level; lev++) {
    dx_level[lev] = dx_level[lev-1] / (double) ref_ratio[lev-1];
  }

  level_steps.create(levs);
  foreach (lev, levs)
    level_steps[lev] = 0;

  /*
    Variables listed below are not saved in plotfile.
  */

  string plot_file = pp->getString("plot_file");

  // println("getting outputfileprefix");
  ostringstream oss;
  oss << plot_file << '.';
  for (int d = 1; d <= SPACE_DIM; d++)
    oss << extent[d] << ".";
  oss << finest_level << '.' << THREADS << '.';
  outputFilePrefix = oss.str();

  // println("getting max_step, stop_time");
  if (MYTHREAD == SPECIAL_PROC) {
    regrid_int = pp->getInteger("regrid_int");
    plot_int = pp->getInteger("plot_int");
    maxSteps = pp->getDouble("max_step");
    maxCumtime = pp->getDouble("stop_time");
    printElapsed = pp->contains("print_elapsed");
    printAdvance = pp->contains("print_advance");
  }

  regrid_int = broadcast(regrid_int, SPECIAL_PROC);
  plot_int = broadcast(plot_int, SPECIAL_PROC);

  maxSteps = broadcast(maxSteps, SPECIAL_PROC);
  maxCumtime = broadcast(maxCumtime, SPECIAL_PROC);
  // println("got max_step, stop_time");

  printElapsed = broadcast(printElapsed, SPECIAL_PROC);
  printAdvance = broadcast(printAdvance, SPECIAL_PROC);
}

void Amr::getSaved(MyInputStream &in) {
  saved = true;
  println("in getSaved");

  // (1) Number of states (int)
  int savedStates = in.readInt1();
  println("saved states: " << savedStates);

  // (2) Name of state (aString)
  for (int i = 0; i < savedStates; i++)
    in.ignoreLine();

  // (3) Number of dimensions (int)
  int savedDims = in.readInt1();
  if (savedDims != SPACE_DIM) {
    println("Plot file is for " << savedDims <<
            "-dimensional problem instead of " << SPACE_DIM);
    exit(0);
  }
  println("saved dimensions: " << savedDims);

  // (4) Time (REAL)
  // double savedTime = broadcast in.readDouble1() from SPECIAL_PROC;
  // cumtime = savedTime;
  // finetime = savedTime;
  double savedTime = in.readDouble1();
  println("saved time: " << savedTime);
  // FIX cumtime = broadcast savedTime from SPECIAL_PROC;
  // FIX finetime = cumtime;

  // (5) Finest level written (int)
  int savedFinest = in.readInt1();
  println("saved finest level: " << savedFinest);
  if (savedFinest != finest_level) {
    println("Plot file has finest level " << savedFinest <<
            " instead of " << finest_level);
    exit(0);
  }

  // (6) Loside prob domain loc in each direction (BL_SPACEDIM*(REAL*))
  ndarray<double, 1> savedProbLo = in.readDoubles();
  foreach (pdim, Util::DIMENSIONS) {
    double edge = savedProbLo[pdim-PT(1)];
    if (edge != ends[-1][pdim]) {
      println("Plot file has edge at " << edge << " instead of " <<
              ends[-1][pdim]);
      exit(0);
    }
  }

  // (7) Hiside prob domain loc in each direction (BL_SPACEDIM*(REAL*))
  ndarray<double, 1> savedProbHi = in.readDoubles();
  foreach (pdim, Util::DIMENSIONS) {
    double edge = savedProbHi[pdim-PT(1)];
    if (edge != ends[+1][pdim]) {
      println("Plot file has edge at " << edge << " instead of " <<
              ends[+1][pdim]);
      exit(0);
    }
  }

  // (8) Refinement ratio ((numLevels-1)*(int))
  ndarray<int, 1> savedRefine = in.readInts();
  foreach (ind, ref_ratio.domain()) {
    if (savedRefine[ind] != ref_ratio[ind]) {
      println("Plot file has refinement ratio of " <<
              savedRefine[ind] << " instead of " << ref_ratio[ind]);
      exit(0);
    }
  }

  // (9) Domain of each level (numLevels*(BOX) )
  ndarray<rectdomain<SPACE_DIM>, 1> savedDomains = in.readDomains();
  point<SPACE_DIM> extentLevel = extent;
  if (savedDomains[0] != RD(Util::ZERO, extentLevel)) {
    println("Plot file has level 0 extent " <<
            savedDomains[0] << " instead of " << extentLevel);
    exit(0);
  }
  for (int h = 1; h <= finest_level; h++) {
    extentLevel = extentLevel * point<SPACE_DIM>::all(ref_ratio[h-1]);
    if (savedDomains[h] != RD(Util::ZERO, extentLevel)) {
      println("Plot file has level " << h << " extent " <<
              savedDomains[h] << " instead of " << extentLevel);
      exit(0);
    }
  }

  // (10) Steps taken on each level (numLevels*(int) )
  ndarray<int, 1> savedSteps = in.readInts();
  /* FIX
     for (int h = 0; h <= finest_level; h++)
     level_steps[h] = broadcast(savedSteps[h], SPECIAL_PROC);
  */

  // (11) Grid spacing (numLevels*(BL_SPACEDIM*(int))) [\n at each level]
  double dxLevel = 0.0; // dx;
  for (int h = 0; h <= finest_level; h++) {
    ndarray<double, 1> savedDx = in.readDoubles();
    if (savedDx.domain().size() != SPACE_DIM) {
      println("Not right number of dimensions with grid spacing");
      exit(0);
    }
    for (int dim = 1; dim <= SPACE_DIM; dim++) {
      println("grid spacing " << savedDx[dim]);
      if (savedDx[dim] != dxLevel) {
        println("Plot file has level " << h << " grid spacing " <<
                savedDx[dim] << " instead of " << dxLevel);
        exit(0);
      }
    }
    if (h < finest_level)
      dxLevel /= (double) ref_ratio[h];
  }

  // (12) Coordinate flag (int)
  in.ignoreLine();

  // (13) Boundary info (int)
  in.ignoreLine();

  // Now getSaved for each level.
  // for (int lev = 0; lev <= finest_level; lev++)
  // amr_level[height].getSaved(in);
  println("Now getSaved for each level.");
}

BoxArray Amr::baseBoxes() {
  int numProcs = THREADS;
  bool useDefaultDist = true;
  point<SPACE_DIM> base = Util::ZERO;
  bool contains_base;
  if (MYTHREAD == SPECIAL_PROC)
    contains_base = pp->contains("base");

  if (broadcast(contains_base, SPECIAL_PROC)) {
    ndarray<int, 1> baseRead = pp->getIntegerArray("base");
    int baseProduct = 1;
    foreach (pdim, Util::DIMENSIONS) {
      int dim = pdim[1];
      int length = broadcast(baseRead[dim-1], SPECIAL_PROC);
      baseProduct *= length;
      base = base + point<SPACE_DIM>::direction(dim, length);
    }
    if (baseProduct == numProcs) {
      useDefaultDist = false;
    } else {
      if (numProcs == 1) {
        println("Ignored amr.base for " << baseProduct <<
                " processes, because using only 1 process.");
      } else {
        if (MYTHREAD == SPECIAL_PROC) {
          println("Ignored amr.base for " << baseProduct <<
                  " processes, because using " << numProcs <<
                  " processes.");
          println("Will split along longest dimension instead.");
        }
      }
      useDefaultDist = true;
    }
  }

  if (useDefaultDist) {
    // Split along longest dimension.  First find which is longest.
    int longestDim = 1;
    for (int dim = 2; dim <= SPACE_DIM; dim++)
      if (extent[dim] > extent[longestDim])
        longestDim = dim;

    base = point<SPACE_DIM>::direction(longestDim, numProcs - 1) +
      Util::ONE;
    // e.g. [4-1, 0] + [1, 1] == [4, 1]
  }

  point<SPACE_DIM> fracExtent = extent / base;
  // Check that each dimension can be split as requested.
  foreach (pdim, Util::DIMENSIONS) {
    int dim = pdim[1];
    if (base[dim] * fracExtent[dim] != extent[dim]) {
      println("Amr: Length " << extent[dim] << " in dimension " <<
              dim << " not divisible by " << base[dim]);
      exit(0);
    }
  }

  rectdomain<SPACE_DIM> fracDomain(Util::ZERO, fracExtent);

  /*
    Assign boxes to processes, and return BoxArray.
  */

  rectdomain<1> procTeam(0, numProcs);
  ndarray<rectdomain<SPACE_DIM>, 1> domainlist(procTeam);
  ndarray<int, 1> proclist(procTeam);

  int proc = 0;
  foreach (section, RD(Util::ZERO, base)) {
    proclist[proc] = proc;
    domainlist[proc] = fracDomain + fracExtent * section;
    proc++;
  }

  return BoxArray(domainlist, proclist);
  // Can't set amr_level[0] = new AmrLevel() right here because
  // you need to do new HSCLLevel().
}

void Amr::timeStep(int level, double time) {
  if (MYTHREAD == SPECIAL_PROC && printAdvance) {
    ostringstream oss;
    for (int space = 0; space < level; space++)
      oss << " ";
    println(oss.str() << "ADVANCE grids at level " << level <<
            " with dt = " << dt_level[level] << " from " <<
            time);
  }
  double dt_new = amr_level[level]->advance(time, dt_level[level]);
  // println("done advance");
  level_steps[level]++;

  if (level < finest_level) {
    for (int i = 0; i < ref_ratio[level]; i++) {
      timeStep(level + 1, time);
      time += dt_level[level + 1];
    }

    // println("doing post_timestep");
    // Use new  amr_level[level + 1]  to update  amr_level[level].
    amr_level[level + 1]->post_timestep();
    // println("done post_timestep");
  } else {
    finetime += dt_level[level];
  }
}

void Amr::run() {
  // int MaxSteps, double MaxCumtime
  if (!saved) {
    cumtime = 0.0;
    finetime = 0.0;
  }

  timer t;

  for (int step = 1;
       step <= maxSteps || cumtime < maxCumtime;
       step++) {
    t.start();
    coarseTimeStep();
    t.stop();
    if (MYTHREAD == SPECIAL_PROC) {
      if (printElapsed) {
        println("elapsed seconds:  " << t.secs());
      }
    }
    t.reset();
  }
}

void Amr::regrid() {
  // If finest_level == 0, you still want to reuse memory.
  // if (finest_level == 0) return;
  if (MYTHREAD == SPECIAL_PROC && printAdvance)
    println("Regridding after step " << level_steps[0]);

  // SharedRegion oldRegion = amr_level[0].getRegion(); // REGION
  //     // println("Amr.regrid():  new SharedRegion, Domain setRegion");
  // SharedRegion newRegion = new SharedRegion(); // REGION
  // domain<1>.setRegion(newRegion); // REGION

  // { // scope to prevent Region problems
  rectdomain<1> levinds(0, finest_level);
  rectdomain<1> levindsnew = levinds + PT(1);
  ndarray<domain<SPACE_DIM>, 1> tags(levinds);

  foreach (lev, levinds)
    tags[lev] = amr_level[lev]->tag();
  checkForPlotError();

  // Doing MeshRefine.refine for levels levinds.
  // Note that you are sending one region for multiple levels.
  ndarray<ndarray<rectdomain<SPACE_DIM>, 1>, 1> boxes =
    MeshRefine::refine(this, tags);

  ndarray<BoxArray, 1> baArray(RD(0, finest_level+1));
  baArray[0] = amr_level[0]->getBa();
  foreach (lev, levindsnew) {
    baArray[lev] =
      amr_level[lev]->assignBoxes(boxes[lev], dProblem[lev]);
  }

  /*
    Reset hierarchy.
    Interpolate data from old hierarchy onto new hierarchy.
  */

  Logger::append("resetting hierarchy");

  /*
    At level 0, patches do not change, but you need to copy in order
    to put them into different regions.

    AmrLevel.regrid() returns old grids at the same level.
  */
  LevelArray<Quantities> QNull = LevelArray<Quantities>::Null();
  LevelArray<Quantities> UTempCoarse =
    amr_level[0]->regrid(baArray[0], BoxArray::Null(), QNull);

  for (int lev = 1; lev <= finest_level; lev++) {
    // println("Regridding level " << lev);
    UTempCoarse =
      amr_level[lev]->regrid(baArray[lev], baArray[lev-1],
                             UTempCoarse);
  }

  // } // scope to prevent Region problems

  tags.destroy();

  // println("Amr.regrid():  deleting oldRegion");
  // RegionUtil::delete(oldRegion); // REGION

  Logger::append("done resetting hierarchy");
  // println("Done regridding");
}

void Amr::computeNewDt() {
  int n_factor = 1;
  double dt_0 = amr_level[0]->estTimeStep();
  for (int lev = 1; lev <= finest_level; lev++) {
    n_factor *= ref_ratio[lev-1];
    double dt_scaled =
      (double) n_factor * amr_level[lev]->estTimeStep();
    if (dt_scaled < dt_0) dt_0 = dt_scaled;
  }

  if (maxCumtime > 0.0 && cumtime + dt_0 > maxCumtime)
    dt_0 = maxCumtime - cumtime;

  n_factor = 1;
  dt_level[0] = dt_0;
  for (int lev = 1; lev <= finest_level; lev++) {
    n_factor *= ref_ratio[lev-1];
    dt_level[lev] = dt_0 / (double) n_factor;
  }
}
