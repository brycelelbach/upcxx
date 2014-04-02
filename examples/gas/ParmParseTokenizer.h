#pragma once

/*
 * @(#)StreamTokenizer.java	1.10 95/08/15
 *
 * Copyright (c) 1995 Sun Microsystems, Inc.  All Rights reserved Permission to
 * use, copy, modify, and distribute this software and its documentation for
 * NON-COMMERCIAL purposes and without fee is hereby granted provided that
 * this copyright notice appears in all copies. Please refer to the file
 * copyright.html for further important copyright and licensing information.
 *
 * SUN MAKES NO REPRESENTATIONS OR WARRANTIES ABOUT THE SUITABILITY OF THE
 * SOFTWARE, EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE,
 * OR NON-INFRINGEMENT. SUN SHALL NOT BE LIABLE FOR ANY DAMAGES SUFFERED BY
 * LICENSEE AS A RESULT OF USING, MODIFYING OR DISTRIBUTING THIS SOFTWARE OR
 * ITS DERIVATIVES.
 */

/**
 * A class to turn an input stream into a stream of tokens.
 * There are a number of methods that define the lexical
 * syntax of tokens.
 * @version 1.10, 15 Aug 1995
 * @author  James Gosling
 */

#include <string>
#include <sstream>
#include <locale>

class ParmParseTokenizer {
  // private string argString;
protected:
  typedef char byte;

  char *buf;
  size_t buflen;
  int peekc;
  bool pushedBack;
  bool forceLower;
  /** The line number of the last token read */
  int LINENO;

  bool eolIsSignificantP;
  bool slashSlashCommentsP;
  bool slashStarCommentsP;


  byte ctype[256];
  static const int CTYPE_LENGTH = 256;
  static const byte CT_WHITESPACE = 1;
  static const byte CT_DIGIT = 2;
  static const byte CT_ALPHA = 4;
  static const byte CT_QUOTE = 8;
  static const byte CT_COMMENT = 16;

public:
  /**
   * The type of the last token returned.  It's value will either
   * be one of the following TT_* constants, or a single
   * character.  For example, if '+' is encountered and is
   * not a valid word character, ttype will be '+'
   */
  int ttype;

  /**
   * The End-of-file token.
   */
  static const int TT_EOF = -1;

  /**
   * The End-of-line token.
   */
  static const int TT_EOL = '\n';

  /**
   * The number token.  This value is in nval.
   */
  static const int TT_NUMBER = -2;

  /**
   * The word token.  This value is in sval.
   */
  static const int TT_WORD = -3;

  /**
   * The Stream value.
   */
  std::string sval;

  /**
   * The number value.
   */
  double nval;

  /**
   * Creates a stream tokenizer that parses the specified input
   * stream.
   * By default, it recognizes numbers, Strings quoted with
   * single and double quotes, and all the alphabetics.
   * @param I the input stream
   */
  ParmParseTokenizer();

  /**
   * Resets the syntax table so that all characters are special.
   */
  void resetSyntax() {
    for (int i = CTYPE_LENGTH; --i >= 0;)
      ctype[i] = 0;
  }

  /**
   * Specifies that characters in this range are word characters.
   * @param low the low end of the range
   * @param hi the high end of the range
   */
  void wordChars(int low, int hi) {
    if (low < 0)
      low = 0;
    if (hi > CTYPE_LENGTH)
      hi = CTYPE_LENGTH-1;
    while (low <= hi)
      ctype[low++] |= CT_ALPHA;
  }

  /**
   * Specifies that characters in this range are whitespace
   * characters.
   * @param low the low end of the range
   * @param hi the high end of the range
   */
  void whitespaceChars(int low, int hi) {
    if (low < 0)
      low = 0;
    if (hi > CTYPE_LENGTH)
      hi = CTYPE_LENGTH-1;
    while (low <= hi)
      ctype[low++] = CT_WHITESPACE;
  }

  /**
   * Specifies that characters in this range are 'ordinary'.
   * Ordinary characters mean that any significance as words,
   * comments, strings, whitespaces or number characters are removed.
   * When these characters are encountered by the
   * parser, they return a ttype equal to the character.
   * @param low the low end of the range
   * @param hi the high end of the range
   */
  void ordinaryChars(int low, int hi) {
    if (low < 0)
      low = 0;
    if (hi > CTYPE_LENGTH)
      hi = CTYPE_LENGTH-1;
    while (low <= hi)
      ctype[low++] = 0;
  }

  /**
   * Specifies that this character is 'ordinary': it removes any
   * significance as a word, comment, string, whitespace or number
   * character.  When encountered by the parser, it returns a ttype
   * equal to the character.
   * @param ch the character
   */
  void ordinaryChar(int ch) {
    ctype[ch] = 0;
  }

  /**
   * Specifies that this character starts a single line comment.
   * @param ch the character
   */
  void commentChar(int ch) {
    ctype[ch] = CT_COMMENT;
  }

  /**
   * Specifies that matching pairs of this character delimit string
   * constants.  When a string constant is recognized, ttype will be
   * the character that delimits the string, and sval will have
   * the body of the string.
   * @param ch the character
   */
  void quoteChar(int ch) {
    ctype[ch] = CT_QUOTE;
  }

  /**
   * Specifies that numbers should be parsed.  This method accepts
   * double precision floating point numbers and returns a ttype of
   * TT_NUMBER with the value in nval.
   */
  void parseNumbers() {
    for (int i = '0'; i <= '9'; i++)
      ctype[i] |= CT_DIGIT;
    ctype['.'] |= CT_DIGIT;
    ctype['-'] |= CT_DIGIT;
  }

  /**
   * If the flag is true, end-of-lines are significant (TT_EOL will
   * be returned by nexttoken).  If false, they will be treated
   * as whitespace.
   */
  void eolIsSignificant(bool flag) {
    eolIsSignificantP = flag;
  }

  /**
   * If the flag is true, recognize C style( /* ) comments.
   */
  void slashStarComments(bool flag) {
    slashStarCommentsP = flag;
  }

  /**
   * If the flag is true, recognize C++ style( // ) comments.
   */
  void slashSlashComments(bool flag) {
    slashSlashCommentsP = flag;
  }

  /**
   * Examines a bool to decide whether TT_WORD tokens are
   * forced to be lower case.
   * @param fl the bool flag
   */
  void lowerCaseMode(bool fl) {
    forceLower = fl;
  }

  virtual int readChar() = 0;

  /**
   * Parses a token from the input stream.  The return value is
   * the same as the value of ttype.  Typical clients of this
   * class first set up the syntax tables and then sit in a loop
   * calling nextToken to parse successive tokens until TT_EOF
   * is returned.
   */
  int nextToken();

  /**
   * Pushes back a stream token.
   */
  void pushBack() {
    pushedBack = true;
  }

  /** Return the current line number. */
  int lineno() {
    return LINENO;
  }

  /**
   * Returns the string representation of the stream token.
   */
  std::string toString();

private:
  void toLowerCase(std::string &s) {
    std::locale loc;
    for (int i = 0; i < s.length(); i++)
      s[i] = std::tolower(s[i], loc);
  }
};
