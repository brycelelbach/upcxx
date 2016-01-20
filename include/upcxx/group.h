/**
 * group.h - implements group objects
 *
 * limitations: function arguments are passed by values!
 */

#pragma once

/// \cond SHOW_INTERNAL

namespace upcxx
{
  struct group {
    int _size;
    int _index;

    group(int sz, int i) : _size(sz), _index(i)
    { }

    inline int index() const { return _index; }
    inline int size() const { return _size; }
  }; // class group

  inline std::ostream& operator<<(std::ostream& out, const group& g)
  {
    return out << "group: size " << g.size()
               << ", my index " << g.index() << "\n";
  }

  template<typename group_t>
  inline int get_global_index(const group &g)
  {
    return g.index();
  }
} // upcxx

/// @endcond
