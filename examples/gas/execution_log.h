#pragma once

// The execution_log class.

/******************************************************************************
 CHANGE LOG.

 23 Nov 98:
  Creation. [pike]

******************************************************************************/

#include <iostream>
#include <string>
#include <sstream>
#include "globals.h"
#include "boxedlist.h"
#include "histogram.h"

class execution_log {
private:
  boxedlist<string> *strings;
  ostream *out;
  bool timestampByDefault;
  timer tmr;
  int loggingLevel;
  static const int defaultLevel = 1;

public:
  void init(int ll = defaultLevel) {
    loggingLevel = ll;
    strings = new boxedlist<string>();
    timestampByDefault = true;
    // time0 = System.currentTimeMillis();
    tmr.start();
    begin(string("log"));
  }

  void rawappend(int ll, const string &s) {
    if (ll <= loggingLevel)
      strings->push(s);
  }

  void append(const string &s) {
    append(defaultLevel, s);
  }

  void append(int ll, const string &s) {
    if (ll > loggingLevel) return;
    ostringstream os;
    os << MYTHREAD << ": ";
    if (timestampByDefault) {
      tmr.stop();
      os << tmr.secs() << ": ";
      tmr.start();
    }
    os << s;
    rawappend(ll, os.str());
  }

  void begin(const string &phase) {
    ostringstream os;
    os << "begin " << phase;
    append(os.str());
  }

  void end(const string &phase) {
    ostringstream os;
    os << "end " << phase;
    append(os.str());
  }

  static void printArray(ostream &out, const ndarray<string, 1> &a) {
    for (int i = a.domain().min()[1]; i <= a.domain().max()[1]; i++)
      out << a[i] << endl;
  }

  static void printReverseOrder(boxedlist<string> *s,
                                ostream &out) {
    printArray(out, s->toReversedArray());
  }

  void dump(boxedlist<string> *s) {
    // println("Dumping execution_log from proc " << MYTHREAD);
    printReverseOrder(s, *out);
  }

  static void end(execution_log &e);

  void end() {
    end(*this);
  }

private:
  // Methods below may be called from native code for logging purposes.
  static ndarray<histogram<long> *, 1> h;
  static timer xtimer;
  static int xkind;
  static const int xkinds = 3;

public:
  static void xbegin(int kind) {
    xtimer.start();
  }

  static void xend(int kind);

  static void histDump();
};
