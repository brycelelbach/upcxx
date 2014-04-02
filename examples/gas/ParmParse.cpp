#include "ParmParse.h"

bool ParmParse::initialized = false;
boxedlist<ParmParseEntry *> *ParmParse::table;

void ParmParse::init(int argc, char **args) {
  if (initialized) {
    println("ParmParse:  table already built.");
    return;
  }
  table = new boxedlist<ParmParseEntry *>();

  ostringstream argString;
  for (int i = 0; i < argc; i++)
    argString << args[i] << ' ';
  readArgs(argString.str());

  initialized = true;
}

void ParmParse::tokenize(ParmParseTokenizer &tokenizer,
                         const string &inputfile) {
  int state = STATE_START;
  string head, temp;
  boxedlist<string> *list;
  int lineNumber;
  ostringstream oss;

  /*
    Let '"' pass through to WORD, since you get sval both ways.

    Can return with:
    WORD && option
    WORD && !option
    '='

    What if it's FILE?

    cases:
    STATE_START:
    - WORD && option:  addEntry(option, new()); START
    - WORD && !option:  head = word; list = new(); LHS
    - default:  "need to start with variable or option name"; ERROR
    LHS:
    - '=':  list = new(); RHS
    - default:  "no '=' sign given after " + head; ERROR
    RHS:
    - WORD && option:  if temp exists, list->push(temp);
    addEntry(head, list); addEntry(option, new()); START
    - WORD && !option:  if temp exists, list->push(temp); temp = word; RHS
    - '=':  new entry(head, list); head = temp; RHS
  */

  while (tokenizer.nextToken() != ParmParseTokenizer::TT_EOF) {
    string sval = tokenizer.sval;
    lineNumber = tokenizer.lineno();
    // oss << "token is " << sval;
    // println(oss.str());
    // oss.str("");

    switch (state) {

    case STATE_START:
      // System.out.println("state STATE_START");
      switch (tokenizer.ttype) {
      case ParmParseTokenizer::TT_WORD: case '"':
        if (isOption(sval)) {
          addEntry(sval.substr(1), new boxedlist<string>());
          state = STATE_START;
        } else {
          head = sval;
          list = new boxedlist<string>();
          state = STATE_LHS;
        }
        break;
      default:   // case '='
        oss << "ParmParse: missing variable name" << " in "
            << inputfile << " at line " << lineNumber;
        println(oss.str());
        exit(0);  // or exception
      }
      break;

    case STATE_LHS:
      // System.out.println("state STATE_LHS");
      switch (tokenizer.ttype) {
      case '=':
        state = STATE_RHS_FIRST;
        break;
      default:
        oss << "ParmParse: no '=' given after " << head << " in "
            << inputfile << " at line " << lineNumber;
        println(oss.str());
        exit(0);  // or exception
      }
      break;

    case STATE_RHS_FIRST:
      // System.out.println("state STATE_RHS_FIRST");
      switch (tokenizer.ttype) {
      case ParmParseTokenizer::TT_WORD: case '"':
        // OK if sval starts with '-'.
        // If someone says fname = -what, then they probably mean it.
        temp = sval;
        list = new boxedlist<string>();
        state = STATE_RHS_REST;
        break;
      case '=':
        // Another '='; might as well let it pass.
        state = STATE_RHS_FIRST;
      }
      break;

    case STATE_RHS_REST:
      // System.out.println("state STATE_RHS_REST");
      switch (tokenizer.ttype) {
      case ParmParseTokenizer::TT_WORD: case '"':
        if (isOption(sval)) {
          list->push(temp);
          addEntry(head, list);
          addEntry(sval.substr(1), new boxedlist<string>());
          state = STATE_START;
        } else {  // just another RHS word
          list->push(temp);
          temp = sval;
          state = STATE_RHS_REST;
        }
        break;
      case '=':
        addEntry(head, list);
        head = temp;
        state = STATE_RHS_FIRST;
      }
      break;

    default:
      oss << "ParmParse default state; something wrong" << " in "
          << inputfile << " at line " << lineNumber;
      println(oss.str());
      oss.str("");
    }
  }
  // Now wind down.
  switch (state) {
  case STATE_START:
    break;
  case STATE_LHS:
    oss << "ParmParse never assigned " << head << " in "
        << inputfile << " at line " << lineNumber;
    println(oss.str());
    oss.str("");
    break;
  case STATE_RHS_FIRST:
    oss << "ParmParse never got value for " << head << " in "
        << inputfile << " at line " << lineNumber;
    println(oss.str());
    oss.str("");
    break;
  case STATE_RHS_REST:
    list->push(temp);
    addEntry(head, list);
    break;
  default:
    oss << "ParmParse default state; something wrong" << " in "
        << inputfile << " at line " << lineNumber;
    println(oss.str());
    oss.str("");
  }
}

void ParmParse::addEntry(const string &head,
                         boxedlist<string> *list) {
  if (head == "FILE") {
    // Get the files in the right order.
    ndarray<string, 1> fileArray = list->toReversedArray();
    foreach (ind, fileArray.domain()) {
      readFile(fileArray[ind]);
    }
    fileArray.destroy();
  } else {
    ParmParseEntry *ppEntry = new ParmParseEntry(head, list);
    table->push(ppEntry);
  }
}

ParmParse::ParmParse(const string &prefix) : myPrefix(prefix) {
  if (!initialized) {
    println("ParmParse:  table not initialized");
    exit(0);  // or throw exception
  }
  // myPrefix = prefix;
  myParmSubset = findParmSubset(prefix, table);
  if (myParmSubset == NULL) {
    ostringstream oss;
    oss << "ParmParse warning:  no such prefix " << prefix;
    println(oss.str());
  }
}

boxedlist<ParmParseEntry *> *
ParmParse::findParmSubset(const string &prefix,
                          boxedlist<ParmParseEntry *> *tab) {
  boxedlist<ParmParseEntry *> *list =
    new boxedlist<ParmParseEntry *>();
  ostringstream oss;
  oss << prefix << '.';
  string prefixDot = oss.str();
  for (blist<ParmParseEntry *> *l = tab->tolist(); l != NULL;
       l = l->rest()) {
    ParmParseEntry *entry = l->first();
    string head = entry->head();
    if (head.find(prefixDot) == 0) {
      string suffix = head.substr(prefixDot.length());
      // Ignore it if it is already in the table.
      if (findEntry(suffix, list) == NULL) {
        list->push(new ParmParseEntry(suffix, entry->list()));
      }
    }
  }
  return list;
}

string ParmParse::retrieveString(const string &head,
                                 boxedlist<ParmParseEntry *> *tab,
                                 const string &name){
  boxedlist<string> *list = retrieveList(head, tab, name);
  ostringstream oss;
  if (list->length() == 0) {
    oss << "ParmParse:  no value for " << name;
    println(oss.str());
    exit(0);  // or exception?
  } else if (list->length() > 1) {
    oss << "ParmParse:  multiple values given for " << name;
    println(oss.str());
    exit(0);  // or exception?
  }
  return list->first();
}

int ParmParse::retrieveInteger(const string &head,
                               boxedlist<ParmParseEntry *> *tab,
                               const string &name) {
  string str = retrieveString(head, tab, name);
  int val = 0;
  istringstream iss(str);
  if (!(iss >> val)) {
    ostringstream oss;
    oss << "ParmParse:  " << name
        << " should be an integer, can't convert " << str;
    println(oss.str());
    exit(0);
  }
  return val;
}

double ParmParse::retrieveDouble(const string &head,
                                 boxedlist<ParmParseEntry *> *tab,
                                 const string &name) {
  string str = retrieveString(head, tab, name);
  double val = 0.0;
  istringstream iss(str);
  if (!(iss >> val)) {
    ostringstream oss;
    oss << "ParmParse:  " << name
        << " should be a double, can't convert " << str;
    println(oss.str());
    exit(0);
  }
  return val;
}

ndarray<int, 1>
ParmParse::retrieveIntegerArray(const string &head,
                                boxedlist<ParmParseEntry *> *tab,
                                const string &name) {
  ndarray<string, 1> stringArray =
    retrieveStringArray(head, tab, name);
  rectdomain<1> dom = stringArray.domain();
  ndarray<int, 1> integerArray(dom);
  foreach (ind, dom) {
    int val;
    istringstream iss(stringArray[ind]);
    if (!(iss >> val)) {
      ostringstream oss;
      oss << "ParmParse:  " << name
          << " should be an integer, can't convert "
          << stringArray[ind];
      println(oss.str());
      exit(0);
    }
    integerArray[ind] = val;
  }
  return integerArray;
}

ndarray<double, 1>
ParmParse::retrieveDoubleArray(const string &head,
                               boxedlist<ParmParseEntry *> *tab,
                               const string &name) {
  ndarray<string, 1> stringArray =
    retrieveStringArray(head, tab, name);
  rectdomain<1> dom = stringArray.domain();
  ndarray<double, 1> doubleArray(dom);
  foreach (ind, dom) {
    double val;
    istringstream iss(stringArray[ind]);
    if (!(iss >> val)) {
      ostringstream oss;
      oss << "ParmParse:  " << name
          << " should be a double, can't convert "
          << stringArray[ind];
      println(oss.str());
      exit(0);
    }
    doubleArray[ind] = val;
  }
  return doubleArray;
}
