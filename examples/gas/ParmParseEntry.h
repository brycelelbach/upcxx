#pragma once

#include <sstream>
#include "globals.h"
#include "boxedlist.h"

class ParmParseEntry {
private:
  string ehead;
  boxedlist<string> *slist;

public:
  ParmParseEntry(const string &h, boxedlist<string> *l) :
    ehead(h), slist(l) {}

  string head() { 
    return ehead;
  }

  boxedlist<string> *list() {
    return slist;
  }

  void printout() {
    ostringstream output;
    output << ehead << " : ( ";
    for (blist<string> *l = slist->tolist(); l != NULL;
         l = l->rest()) {
      output << l->first() << ' ';
    }
    output << ')';
    println(output.str());
  }
};
