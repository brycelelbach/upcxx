#pragma once

#include <assert.h>
#include <string>

namespace upcxx
{
  /**
   * \brief data structure for describing a range of integers
   *
   * in the begin element is included in the range but the end element of the range
   * is excluded.  For example, range(0, 5, 1) = {0,1, 2, 3, 4};
   */
  struct range {
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
      return (_end - _begin) / _step;
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
