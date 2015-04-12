/**
 * interfaces.h - provides helpers type for implementing the UPC++
 * local array and globalization interfaces.
 *
 * A type implements the local array interface if
 * a) it defines a 'local_elem_type' typedef that is a type that
 *    itself provides a 'type' typedef, which should be the element
 *    type of the array;
 * b) it defines a storage_ptr() method that returns a local pointer
 *    to the underlying array memory; and
 * c) it defines a size() method that returns the number of elements
 *    it contains.
 * The underlying array memory is assumed to be contiguous. It is
 * legal for a type to generate a runtime error if when storage_ptr()
 * is called if this is not the case.
 *
 * A type is globalizable if
 * a) it defines a 'global_type' typedef that is a type that itself
 *    provides a 'type' typedef, which should be the desired global
 *    type;
 * b) a conversion operation (implicit or explicit) is defined from
 *    the local type to the global type.
 *
 * The type of 'local_elem_type' or 'global_type' may be any type that
 * provides a 'type' typedef; examples include std::enable_if<true, T>
 * or upcxx::enable_if<true, T>. This allows implementation of either
 * interface to be dependent on a template parameter, as in the
 * following:
 *
 * template<bool is_global> struct some_type {
 *   typedef enable_if< !is_global, some_type<true> > global_type;
 *   template<bool g2> explicit some_type(some_type<g2> other) {
 *     ...
 *   }
 * }
 *
 * Here, some_type<false> implements the globalization interface,
 * since some_type<false>::global_type::type is a valid type
 * (some_type<true>) and there is a covnersion operation from
 * some_type<false> to some_type<true>. On the other hand,
 * some_type<true> does not implement the interface, since
 * some_type<false>::global_type::type is undefined. Thus, template
 * metaprogramming can be used to define both a local type and its
 * corresponding global type with a single class template definition.
 *
 * \see reduce.h for code that makes use of the local array interface
 * \see broadcast.h for code that makes use of both interfaces
 */

#pragma once

namespace upcxx {
  template<bool enable, class T> struct enable_if {
    typedef T type;
  };

  template<class T> struct enable_if<false, T> {
  };

  /* Check if two types are the same. Can be used with enable_if to
   * implement an interface, as in the following:
   *   typedef enable_if<is_same<L, local>::value, T> local_elem_type;
   */
  template<class T, class U> struct is_same {
    enum { value = 0 };
  };

  template<class T> struct is_same<T, T> {
    enum { value = 1 };
  };

  /* Combination of enable_if and is_same */
#ifdef UPCXX_USE_CXX11
  template<class T, class U, class V>
  using enable_if_same = enable_if<is_same<T, U>::value, V>;
#else
  template<class T, class U, class V> struct enable_if_same {
  };

  template<class T, class V> struct enable_if_same<T, T, V> {
    typedef V type;
  };
#endif
}
