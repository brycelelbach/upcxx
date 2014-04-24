#include <cmath>
#include "Ramp.h"
#include "HSCLLevel.h"
#include "RampBC.h"
#include "RampInit.h"

double Ramp::shockLocBottom;
double Ramp::shockLocTop;
double Ramp::shockSpeed;
double Ramp::slope;
Quantities Ramp::U0, Ramp::U1;
ndarray<Quantities, 1> Ramp::q0flux, Ramp::q1flux;
Amr *Ramp::amrAll;
ndarray<ndarray<double, 1>, 1> Ramp::ends;
double Ramp::gamma = 1.4;
LevelGodunov *Ramp::lg0;

void Ramp::main(int argc, char **args) {
  // println("starting program");
  int myProc = MYTHREAD;
  // println(myProc << " running");

  ParmParse::init(argc, args);
  // printOut:  whether to print out results
  bool printOut, saved;
  if (MYTHREAD == SPECIAL_PROC) {
    printOut = ParmParse::containsGeneral("print");
    saved = ParmParse::containsGeneral("saved");
  }
  printOut = broadcast(printOut, SPECIAL_PROC);
  saved = broadcast(saved, SPECIAL_PROC);

  ParmParse pp("prob");

  /*
    Define Amr object.
  */

  // println(myProc << " defining Amr object");
  amrAll = new Amr();

  // MyInputStream extends DataInputStream with functions.
  // You could move the declaration inside Amr and then
  // use an accessor function in HSCLLevel.
#if 0 // FIX!!! <<<<<<
  MyInputStream in;
  if (saved) {
    try {
      in = new MyInputStream(System.in);
      println("opening saved file");
      amrAll->getSaved(in);
    } catch (IOException iox) {
      println("Error opening input");
      System.exit(0);
    }
  }
#endif

  barrier();

  /*
    Define HSCLLevel object at level 0.
  */

  bool logging;
  if (MYTHREAD == SPECIAL_PROC)
    logging = ParmParse::containsGeneral("log");
  logging = broadcast(logging, SPECIAL_PROC);
  if (logging)
    Logger::begin();

  // println(myProc << " defining HSCLLevel 0");

  // Need to construct HSCLLevel from Ramp, because you can't do it
  // from Amr, which recognizes only AmrLevel, not HSCLLevel.
  HSCLLevel level0(amrAll, 0, amrAll->baseBoxes(), BoxArray::Null(),
                   amrAll->getDomain(0), 0, amrAll->getDx(0),
                   new RampBC());
  // println(myProc << " setLevel 0");
  barrier();
  amrAll->setLevel(0, &level0);
  barrier();

  /*
    Get data.  Need LevelGodunov object to do primitive-conservative
    conversions.  Also need gamma.
  */

  // println(myProc << " getData");
  ends = amrAll->getEnds();

  lg0 = new LevelGodunov(BoxArray::Null(), BoxArray::Null(),
                         amrAll->getDomain(0), 1, 0.0, NULL);
  // don't need ba, baCoarse, nRefine, dx, bc
  // lg0 = level0.getLevelGodunov();  // causes region problems

  getData(pp);

  /*
    Initialize patches at all levels.
    This will also define HSCLLevel objects at finer levels.
  */

  RampInit initializer = RampInit();

  // println(myProc << " calling initGrids");
  level0.initGrids(initializer);
  // println(myProc << " called initGrids");

  // println(myProc << " defined HSCLLevels");

  Logger::barrier();

  /*
    Process the patches.
  */

  // println(myProc << " *********** running ***********");
  // Logger::begin();
  timer t;
  t.start();

  amrAll->run();

  Logger::end();
  t.stop();
  barrier();

  /*
    Output
  */

  println("time = " << t.secs() << " sec");

  if (printOut)
    amrAll->write();
}

void Ramp::getData(ParmParse &pp) {
  double shockang = pp.getDouble("shockang");
  double shockAngle = shockang * (PI / 180.0);
  slope = 1.0 / tan(shockAngle);

  double p0 = pp.getDouble("pressref");
  double shockMach = pp.getDouble("shockmach");
  double c0 = pp.getDouble("soundref");
  double rho0 = gamma * p0 / (c0 * c0);

  double s1 = shockMach * c0;
  double p1 =
    p0 * (2.0 * gamma * shockMach * shockMach - gamma + 1.0) /
    (gamma + 1.0);
  double v1 = (p1 - p0) / (s1 * rho0);
  double ux1 = v1 * cos(shockAngle);
  double uy1 = -v1 * sin(shockAngle);
  double rho1 = rho0 / (1.0 - v1 / s1);

  shockLocBottom = pp.getDouble("shockloc");

  /*
    Set initial primitive quantities.  Recall order rho, ux, uy, p.
    (We need lg0 for conversions and flux.)
  */

  Quantities q0 = Quantities(DENSITY, rho0) +
    Quantities(PRESSURE, p0);
  Quantities q1 = Quantities(DENSITY, rho1) +
    Quantities(velocityIndex(LENGTH_DIM), ux1) +
    Quantities(velocityIndex(HEIGHT_DIM), uy1) +
    Quantities(PRESSURE, p1);

  U0 = lg0->primToCons(q0);
  U1 = lg0->primToCons(q1);

  q0flux.create(Util::DIMENSIONS);
  q1flux.create(Util::DIMENSIONS);
  foreach (pdim, Util::DIMENSIONS) {
    q0flux[pdim] = lg0->solidBndryFlux(q0, pdim[1]);
    q1flux[pdim] = lg0->interiorFlux(q1, pdim[1]);
  }
  // ends[+1][HEIGHT_DIM] - ends[-1][HEIGHT_DIM] is height
  shockLocTop = shockLocBottom +
    (ends[+1][HEIGHT_DIM] - ends[-1][HEIGHT_DIM]) * tan(shockAngle);
  shockSpeed = s1 / cos(shockAngle);
}

int main(int argc, char ** argv) {
  init(&argc, &argv);
  Ramp::main(argc-1, argv+1);
  finalize();
  return 0;
}
