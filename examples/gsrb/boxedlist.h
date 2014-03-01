#pragma once

// #include "list.h"

template<class T> class foo_list {
public:
  T car;
  foo_list *cdr;

  // Main constructor.
  foo_list(T x, foo_list *rest) : car(x), cdr(rest) {}

  foo_list(T x) : car(x), cdr(NULL) {}

  foo_list *cons(T x) {
    return new foo_list(x, this);
  }        

  /*inline*/ T first() {
    return car;
  }
  /*inline*/ foo_list *rest() {
    return cdr;
  }

  // Destructive append.
  foo_list *concat(foo_list *l) {
    if (l != NULL)
      for (foo_list *q = this; ; q = q->cdr)
        if (q->cdr == NULL) {
          q->cdr = l;
          break;
        }
    return this;
  }

  int length() {
    int i = 0;
    foo_list *r = this;
    while (r != NULL) {
      i++;
      r = r->rest();
    }
    return i;
  }
};

template<class T> class boxedlist {
private:
  foo_list<T> *l;

public:
  boxedlist() : l(NULL) {}

  boxedlist(T x) : l(NULL) {
    push(x);
  }

  void push(T item) {
    l = new foo_list<T>(item, l);
  }

  T pop() {
    T t = l->first();
    l = l->rest();
    return t;
  }

  T first() {
    return l->first();
  }

  foo_list<T> *tolist() {
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
    for (foo_list<T> *r = l; r != NULL; r = r->rest()) {
      a[i] = r->first();
      i++;
    }
    return a;
  }

  ndarray<T, 1> toReversedArray() {
    int i = length();
    ndarray<T, 1> a(RECTDOMAIN((0), (i)));
    for (foo_list<T> *r = l; r != NULL; r = r->rest()) {
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
