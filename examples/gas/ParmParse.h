#pragma once

/**
 * ParmParse parses input from the command line and input files.
 *
 * Doesn't raise exceptions, but could raise:
 * IOException, NumberFormatException,
 * exceptions for bad input and could not find?
 *
 * @version  27 Oct 1999
 * @author   Peter McCorquodale, PWMcCorquodale@lbl.gov
 */

#include <fstream>
#include <sstream>
#include "globals.h"
#include "boxedlist.h"
#include "ParmParseEntry.h"
#include "ParmParseFileTokenizer.h"
#include "ParmParseStringTokenizer.h"
#include "ParmParseTokenizer.h"

class ParmParse {
private:
  /*
    constants representing states in parsing
  */

  static const int STATE_START = 0;
  static const int STATE_LHS = 1;
  static const int STATE_RHS_FIRST = 2;
  static const int STATE_RHS_REST = 3;

  static bool initialized;
  static boxedlist<ParmParseEntry *> *table;

  boxedlist<ParmParseEntry *> *myParmSubset;
  string myPrefix;

public:
  /*
    methods for initializing the table.
  */

  /**
   * Call init() first from main program with command-line args.
   */
  static void init(int argc, char **args);

private:
  /**
   * Read input file.
   * Called by addEntry() when "FILE = inputfile"
   * @param inputfile name of input file to open
   */
  static void readFile(const string &inputfile) {
    ifstream fin(inputfile);
    ParmParseFileTokenizer tokenizer(fin);
    tokenize(tokenizer, inputfile);
    fin.close();
  }

  /**
   * Parse string.
   * Called by init().
   */
  static void readArgs(const string &argString) {
    ParmParseStringTokenizer tokenizer(argString);
    tokenize(tokenizer, "command line");
  }

public:
  /**
   * Tokenize using previously initialized tokenizer.
   * Called by readFile(), readArgs().
   * @param inputfile name of input file (or "command line") for error messages
   */
  static void tokenize(ParmParseTokenizer &tokenizer,
                       const string &inputfile);

private:
  /**
   * Check whether a string has the form of an option:
   * -[string], where [string] does not begin with a digit.
   */
  static bool isOption(const string &word) {
    if (word.length() < 2) return false;
    if (word[0] != '-') return false;
    char first = word[1];
    return first <= '0' || first >= '9';
  }

  /**
   * Add an entry to the table, or read a new file.
   */
  static void addEntry(const string &head, boxedlist<string> *list);

public:
  /**
   * constructor
   */
  ParmParse(const string &prefix);

private:
  /*
    Methods for finding entries in myParmSubset or the table.
  */

  /**
   * Return list of variables, with the prefix removed.
   */
  boxedlist<ParmParseEntry *> *findParmSubset(const string &prefix,
                                              boxedlist<ParmParseEntry *> *tab);

  /**
   * Return the first entry in tab with this head.
   * If there are duplicates, this is the one that was entered last.
   */
  static ParmParseEntry *findEntry(const string &head,
                                   boxedlist<ParmParseEntry *> *tab) {
    for (blist<ParmParseEntry *> *l = tab->tolist(); l != NULL;
         l = l->rest()) {
      ParmParseEntry *entry = l->first();
      if (entry->head() == head)
        return entry;
    }
    return NULL;
  }

  static boxedlist<string> *retrieveList(const string &head,
                                         boxedlist<ParmParseEntry *> *tab,
                                         const string &name) {
    ParmParseEntry *entry = findEntry(head, tab);
    if (entry == NULL) {
      println("ParmParse:  no entry for " << name);
      exit(0);  // or exception?
    }
    boxedlist<string> *list = entry->list();
    return list;
  }

public:
  /**
   * Get a single String.
   */
  string getString(const string &suffix) {
    ostringstream oss;
    oss << myPrefix << '.' << suffix;
    return retrieveString(suffix, myParmSubset, oss.str());
  }

  static string getStringGeneral(const string &head) {
    return retrieveString(head, table, head);
  }

private:
  static string retrieveString(const string &head,
                               boxedlist<ParmParseEntry *> *tab,
                               const string &name);

public:
  /**
   * Get a single integer.
  */
  int getInteger(const string &suffix) {
    ostringstream oss;
    oss << myPrefix << '.' << suffix;
    return retrieveInteger(suffix, myParmSubset, oss.str());
  }

  static int getIntegerGeneral(const string &head) {
    return retrieveInteger(head, table, head);
  }

private:
  static int retrieveInteger(const string &head,
                             boxedlist<ParmParseEntry *> *tab,
                             const string &name);

public:
  /**
   * Get a single double.
  */
  double getDouble(const string &suffix) {
    ostringstream oss;
    oss << myPrefix << '.' << suffix;
    return retrieveDouble(suffix, myParmSubset, oss.str());
  }

  static double getDoubleGeneral(const string &head) {
    return retrieveDouble(head, table, head);
  }

private:
  static double retrieveDouble(const string &head,
                               boxedlist<ParmParseEntry *> *tab,
                               const string &name);

public:
  /**
   * Get an array of Strings.
   */
  ndarray<string, 1> getStringArray(const string &suffix) {
    ostringstream oss;
    oss << myPrefix << '.' << suffix;
    return retrieveStringArray(suffix, myParmSubset, oss.str());
  }

  static ndarray<string, 1> getStringArrayGeneral(const string &head) {
    return retrieveStringArray(head, table, head);
  }

private:
  static ndarray<string, 1> retrieveStringArray(const string &head,
                                                boxedlist<ParmParseEntry *> *tab,
                                                const string &name) {
    boxedlist<string> *list = retrieveList(head, tab, name);
    return list->toReversedArray();
  }

public:
  /**
   * Get an array of integers.
   */
  ndarray<int, 1> getIntegerArray(const string &suffix) {
    ostringstream oss;
    oss << myPrefix << '.' << suffix;
    return retrieveIntegerArray(suffix, myParmSubset, oss.str());
  }

  static ndarray<int, 1> getIntegerArrayGeneral(const string &head) {
    return retrieveIntegerArray(head, table, head);
  }

private:
  static ndarray<int, 1> retrieveIntegerArray(const string &head,
                                              boxedlist<ParmParseEntry *> *tab,
                                              const string &name);

public:
  /**
   * Get an array of doubles.
   */
  ndarray<double, 1> getDoubleArray(const string &suffix) {
    ostringstream oss;
    oss << myPrefix << '.' << suffix;
    return retrieveDoubleArray(suffix, myParmSubset, oss.str());
  }

  static ndarray<double, 1> getDoubleArrayGeneral(const string &head) {
    return retrieveDoubleArray(head, table, head);
  }

private:
  static ndarray<double, 1> retrieveDoubleArray(const string &head,
                                                boxedlist<ParmParseEntry *> *tab,
                                                const string &name);

public:
  /**
   * Check whether the table contains a particular header.
   */
  bool contains(const string &suffix) {
    ParmParseEntry *entry = findEntry(suffix, myParmSubset);
    return entry != NULL;
  }

  static bool containsGeneral(const string &head) {
    ParmParseEntry *entry = findEntry(head, table);
    return entry != NULL;
  }

  /*
    output methods
  */

  static void printTableGeneral() {
    printTableSpecified(table);
  }

  void printTable() {
    println("Table for " << myPrefix << ":");
    printTableSpecified(myParmSubset);
  }

private:
  static void printTableSpecified(boxedlist<ParmParseEntry *> *tab) {
    for (blist<ParmParseEntry *> *l = tab->tolist(); l != NULL;
         l = l->rest()) {
      l->first()->printout();
    }
  }
};
