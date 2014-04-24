#pragma once

template<class T> class list {
public:
  T car;
  list *cdr;

  // Main constructor.
  list(T x, list *rest) : car(x), cdr(rest) {}

  list(T x) : car(x), cdr(NULL) {}

  list *cons(T x) {
    return new list(x, this);
  }        

  /*inline*/ T first() {
    return car;
  }
  /*inline*/ list *rest() {
    return cdr;
  }

  // Destructive append.
  list *concat(list *l) {
    if (l != NULL)
      for (list *q = this; ; q = q->cdr)
        if (q->cdr == NULL) {
          q->cdr = l;
          break;
        }
    return this;
  }

  int length() {
    int i = 0;
    list *r = this;
    while (r != NULL) {
      i++;
      r = r->rest();
    }
    return i;
  }
};
