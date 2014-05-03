#pragma once

#include "ParmParseTokenizer.h"

class ParmParseStringTokenizer : public ParmParseTokenizer {
private:
  string argString;
  int currentIndex;  // index into string being parsed

public:
  /**
   * Creates a stream tokenizer that parses the specified input
   * stream.
   * By default, it recognizes numbers, Strings quoted with
   * single and double quotes, and all the alphabetics.
   * @param I the input stream
   */
  ParmParseStringTokenizer(const string &argStringGiven) :
    ParmParseTokenizer(), argString(argStringGiven), currentIndex(0) {
    LINENO = 0;
  }

protected:
  int readChar() {
    // currentIndex is pointer to a location in argString
    if (currentIndex >= argString.length()) return TT_EOF;
    int c = argString[currentIndex];
    currentIndex++;
    return c;
  }
};
