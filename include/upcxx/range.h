#pragma once

#include <assert.h>
#include <string>

namespace upcxx
{
  /**
   * @brief data structure for describing a range of integers
   *
   * UPC++ range is left-closed and right-open: the first (begin)
   * element is inclusive while the last (end) element is exclusive.
   * For example, range(0, 5, 1) = {0, 1, 2, 3, 4};
   */
  struct range {

    typedef       int iterator;
    typedef const int const_iterator;

    int _begin;
    int _end;
    int _step;
    
  public:
    inline range() :
    _begin(0), _end(1), _step(1)
    { }
    
    inline range(int end) :
    _begin(0), _end(end), _step(1)
    {
      assert (end > 0);
    }
    
    inline range(int begin, int end, int step = 1) :
    _begin(begin), _end(end), _step(step)
    {
      if (end > begin) {
        assert(step > 0);
      } else {
        if (begin > end) {
          assert(step < 0);
        }
      }
    }
    
    inline int count() const
    {
      return (_end - _begin + (_step -1)) / _step;
    }
    
    inline int size() const
    {
      return count();
    }

    inline int begin() const
    {
      return _begin;
    }
    
    inline int end() const
    {
      return _end;
    }
    
    inline int step() const
    {
      return _step;
    }
    
    int operator [] (int i) const
    {
      assert(i < this->count());
      return (_begin + i * _step);
    }
    
    range operator + (int i)
    {
      // assert(i <= count());
      return range(_begin, _end + i * _step, _step);
    }
    
    range operator - (int i)
    {
      assert(count() > i);
      return range(_begin, _end - i * _step, _step);
    }
    
    range operator / (int i)
    {
      return range(_begin, _begin + count() / i * _step, _step);
    }
    
    bool contains(int i)
    {
      if ((i >= _begin) && (i < _end) &&
          ((i-_begin)%_step == 0))
        return true;
    
      return false;
    }
  }; // struct range
  
  inline std::ostream& operator<<(std::ostream& out, const upcxx::range& r)
  {
    const int buf_sz = 1024;
    char buf[buf_sz];
    snprintf(buf, buf_sz, "range: (begin %u, end %u, step %u, count %u)",
            r.begin(), r.end(), r.step(), r.count());
    return out << std::string(buf);
  }
  
} // end of namespace 
