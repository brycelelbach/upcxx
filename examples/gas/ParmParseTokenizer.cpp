#include "ParmParseTokenizer.h"

using namespace std;

ParmParseTokenizer::ParmParseTokenizer() :
  buf(new char[20]), buflen(20), peekc(' '), pushedBack(false),
  forceLower(false), LINENO(0), eolIsSignificantP(false),
  slashSlashCommentsP(false), slashStarCommentsP(false) {
  // buf = new char[20];
  byte *ct = ctype;
  int i;
  resetSyntax();
  wordChars(0, 255);
  // wordChars('a', 'z');
  // wordChars('A', 'Z');
  // wordChars(128 + 32, 255);
  whitespaceChars(0, ' ');
  // commentChar('/');
  quoteChar('"');
  // quoteChar('\'');
  // parseNumbers();
  ordinaryChar('=');
  lowerCaseMode(false);
  eolIsSignificant(false);
  commentChar('#');
}

int ParmParseTokenizer::nextToken() {
  if (pushedBack) {
    pushedBack = false;
    return ttype;
  }
  // InputStream is = input;
  byte *ct = ctype;
  int c = peekc;
  sval = "";

  if (c < 0)
    return ttype = TT_EOF;
  int ctype = c < 256 ? ct[c] : CT_ALPHA;
  while ((ctype & CT_WHITESPACE) != 0) {
    if (c == '\r') {
      LINENO++;
      c = readChar(); // input
      if (c == '\n')
        c = readChar(); // input
      if (eolIsSignificantP) {
        peekc = c;
        return ttype = TT_EOL;
      }
    } else {
      if (c == '\n') {
        LINENO++;
        if (eolIsSignificantP) {
          peekc = ' ';
          return ttype = TT_EOL;
        }
      }
      c = readChar(); // input
    }
    if (c < 0)
      return ttype = TT_EOF;
    ctype = c < 256 ? ct[c] : CT_ALPHA;
  }
  if ((ctype & CT_DIGIT) != 0) {
    bool neg = false;
    if (c == '-') {
      c = readChar(); // input
      if (c != '.' && (c < '0' || c > '9')) {
        peekc = c;
        return ttype = '-';
      }
      neg = true;
    }
    double v = 0;
    int decexp = 0;
    int seendot = 0;
    while (true) {
      if (c == '.' && seendot == 0)
        seendot = 1;
      else if ('0' <= c && c <= '9') {
        v = v * 10 + (c - '0');
        decexp += seendot;
      } else
        break;
      c = readChar(); // input
    }
    peekc = c;
    if (decexp != 0) {
      double denom = 10;
      decexp--;
      while (decexp > 0) {
        denom *= 10;
        decexp--;
      }
      /* do one division of a likely-to-be-more-accurate number */
      v = v / denom;
    }
    nval = neg ? -v : v;
    return ttype = TT_NUMBER;
  }
  if ((ctype & CT_ALPHA) != 0) {
    int i = 0;
    do {
      if (i >= buflen-1) {
        char *nb = new char[buflen * 2];
        memcpy(nb, buf, buflen*sizeof(char));
        delete[] buf;
        buf = nb;
        buflen *= 2;
      }
      buf[i++] = (char) c;
      c = readChar(); // input
      ctype = c < 0 ? CT_WHITESPACE : c < 256 ? ct[c] : CT_ALPHA;
    } while ((ctype & (CT_ALPHA | CT_DIGIT)) != 0);
    peekc = c;
    buf[i] = (char) 0;
    sval = buf; //String.copyValueOf(buf, 0, i);
    if (forceLower)
      toLowerCase(sval);
    return ttype = TT_WORD;
  }
  if ((ctype & CT_COMMENT) != 0) {
    while ((c = readChar()) != '\n' && c != '\r' && c >= 0); // input
    peekc = c;
    return nextToken();
  }
  if ((ctype & CT_QUOTE) != 0) {
    ttype = c;
    int i = 0;
    while ((c = readChar()) >= 0 && c != ttype && c != '\n' &&
           c != '\r') { // input
      if (c == '\\')
        switch (c = readChar()) { // input
        case 'a':
          c = 0x7;
          break;
        case 'b':
          c = '\b';
          break;
        case 'f':
          c = 0xC;
          break;
        case 'n':
          c = '\n';
          break;
        case 'r':
          c = '\r';
          break;
        case 't':
          c = '\t';
          break;
        case 'v':
          c = 0xB;
          break;
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
          c = c - '0';
          int c2 = readChar(); // input
          if ('0' <= c2 && c2 <= '7') {
            c = (c << 3) + (c2 - '0');
            c2 = readChar(); // input
            if ('0' <= c2 && c2 <= '7')
              c = (c << 3) + (c2 - '0');
            else
              peekc = c;
          } else
            peekc = c;
          break;
        }
      if (i >= buflen-1) {
        char *nb = new char[buflen * 2];
        memcpy(nb, buf, buflen*sizeof(char));
        delete[] buf;
        buf = nb;
        buflen *= 2;
      }
      buf[i++] = (char) c;
    }
    peekc = ' ';
    buf[i] = (char) 0;
    sval = buf; //String.copyValueOf(buf, 0, i);
    return ttype;
  }
  if (c == '/' && (slashSlashCommentsP || slashStarCommentsP)) {
    c = readChar(); // input
    if (c == '*' && slashStarCommentsP) {
      int prevc = 0;
      while ((c = readChar()) != '/' || prevc != '*') { // input
        if (c == '\n')
          LINENO++;
        if (c < 0)
          return ttype = TT_EOF;
        prevc = c;
      }
      peekc = ' ';
      return nextToken();
    } else if (c == '/' && slashSlashCommentsP) {
      while ((c = readChar()) != '\n' && c != '\r'
             && c >= 0); // input
      peekc = c;
      return nextToken();
    } else {
      peekc = c;
      return ttype = '/';
    }
  }
  peekc = ' ';
  return ttype = c;
}

string ParmParseTokenizer::toString() {
  ostringstream ret;
  switch (ttype) {
  case TT_EOF:
    ret << "EOF";
    break;
  case TT_EOL:
    ret << "EOL";
    break;
  case TT_WORD:
    ret << sval;
    break;
  case TT_NUMBER:
    ret << "n=" << nval;
    break;
  default:{
    char s[4];
    s[0] = s[2] = '\'';
    s[1] = (char) ttype;
    s[3] = (char) 0;
    ret << s;
    break;
  }
  }
  ostringstream result;
  result << "Token[" << ret.str() << "], line " << LINENO;
  return result.str();
}
