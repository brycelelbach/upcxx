#pragma once

#include "globals.h"
// #include "list.h"

template<class T> class blist {
public:
  T car;
  blist *cdr;

  // Main constructor.
  blist(T x, blist *rest) : car(x), cdr(rest) {}

  blist(T x) : car(x), cdr(NULL) {}

  ~blist() {
    delete cdr;
  }

  blist *cons(T x) {
    return new blist(x, this);
  }        

  /*inline*/ T first() {
    return car;
  }
  /*inline*/ blist *rest() {
    return cdr;
  }

  // Destructive append.
  blist *concat(blist *l) {
    if (l != NULL)
      for (blist *q = this; ; q = q->cdr)
        if (q->cdr == NULL) {
          q->cdr = l;
          break;
        }
    return this;
  }

  int length() {
    int i = 0;
    blist *r = this;
    while (r != NULL) {
      i++;
      r = r->rest();
    }
    return i;
  }
};

template<class T> class boxedlist {
private:
  blist<T> *l;

public:
  boxedlist() : l(NULL) {}

  boxedlist(T x) : l(NULL) {
    push(x);
  }

  ~boxedlist() {
    delete l;
  }

  void push(T item) {
    l = new blist<T>(item, l);
  }

  T pop() {
    T t = l->first();
    l = l->rest();
    return t;
  }

  T first() {
    return l->first();
  }

  blist<T> *tolist() {
    return l;
  }

  // Destructive append.
  boxedlist *concat(boxedlist *x) {
    if (l == NULL)
      l = x->l;
    else if (x != NULL)
      l->concat(x->l);
    return this;
  }

  ndarray<T, 1> toArray() {
    ndarray<T, 1> a(RECTDOMAIN((0), (length())));
    int i = 0;
    for (blist<T> *r = l; r != NULL; r = r->rest()) {
      a[i] = r->first();
      i++;
    }
    return a;
  }

  ndarray<T, 1> toReversedArray() {
    int i = length();
    ndarray<T, 1> a(RECTDOMAIN((0), (i)));
    for (blist<T> *r = l; r != NULL; r = r->rest()) {
      a[--i] = r->first();
    }
    return a;
  }

  bool isEmpty() {
    return l == NULL;
  }

  int length() {
    return l == NULL ? 0 : l->length();
  }
};
