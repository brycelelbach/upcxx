#pragma once

#include "ParmParseTokenizer.h"

class ParmParseFileTokenizer : public ParmParseTokenizer {
private:
  istream &input;

public:
  /**
   * Creates a stream tokenizer that parses the specified input
   * stream.
   * By default, it recognizes numbers, Strings quoted with
   * single and double quotes, and all the alphabetics.
   * @param I the input stream
   */
  ParmParseFileTokenizer(istream &I) :
    ParmParseTokenizer(), input(I) {
    LINENO = 1;
  }

protected:
  int readChar() {
    return input.get();
  }
};
