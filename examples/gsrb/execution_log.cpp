// The execution_log class.

/******************************************************************************
 CHANGE LOG.

 23 Nov 98:
  Creation. [pike]

******************************************************************************/

#include <fstream>
#include "execution_log.h"

timer execution_log::xtimer;
ndarray<histogram<long> *, 1> execution_log::h;

void execution_log::end(execution_log &e) {
  int n = THREADS;
  int me = MYTHREAD;
  bool die = false;

  if (e.loggingLevel == 0) return;

  e.end("log");

  for (int i = 0; i < n; i++) {
    if (MYTHREAD == i) {
      ofstream os;
      os.open("log", MYTHREAD == 0 ? ios::out : (ios::out | ios::app));
      e.out = &os;
      e.dump(e.strings);
      e.out = NULL;
      os.close();
    }
    barrier();
  }
}

void execution_log::xend(int kind) {
  double xtime;
  xtimer.stop();
  xtime = xtimer.secs();
  xtimer.start();
  if (h == NULL) {
    if (MYTHREAD == 0)
      println("Creating histogram of message times.");
    h = ndarray<histogram<long> *, 1>(RECTDOMAIN((0), (xkinds)));
    foreach (p, h.domain())
      h[p] = new histogram<long>();
  }
  h[kind]->add(xtime);
}

void execution_log::histDump() {
  int n = THREADS;
  int me = MYTHREAD;
  ostringstream os;

  for (int i = 0; i < n; i++) {
    barrier();
    if (i == me && h != NULL)
      foreach (p, h.domain()) {
        os.str("");
        os << "Histogram " << p << " for proc " << me;
        h[p]->dump(os.str());
      }
  }
}
