///
/// Dense matrix and global matrix data structures
///

#pragma once 

#include <iostream>
#include <iomanip>
#include <cassert>

#include "upcxx.h"
#include "lapack_api.h"

using namespace upcxx;

#define MAX_BLOCKS 1024*1024

// #define DEBUG

namespace upcxx
{
  /**
   * \brief Dense Matrix (column-major storage)
   */
  template <typename T>
  class matrix
  {
  private:
    // The matrix is  m (rows) by n (columns).
    int _m;
    int _n;
    int _ld;
    int _free; /**< free the data on exit? */
    T  *_data;
     
  public:
    matrix() : _m(0), _n(0), _ld(0), _free(0), _data(0) {}

    matrix(int m, int n) : _m(m), _n(n), _ld(m)                                
    { 
      _data = new T[m*n];
      assert(_data != NULL);
      _free = 1;
    }

    // Don't free the external data
    matrix(int m, int n, T *a) 
      : _m(m), _n(n), _ld(m), _free(0), _data(a)
    {}

    matrix(int m, int n, T *a, int ld) 
      : _m(m), _n(n), _ld(ld) ,_free(0), _data(a)
    {}
  
    ~matrix()
    {
      if (_free) {
        assert(_data != NULL);
        delete _data;
      }
    }
  
    inline int m() const { return _m; };
    inline int n() const { return _n; };
    inline int ld () const { return _ld; }
    inline int needfree() const { return _free; }
    inline T* data() const { return _data; }
    
    // C = A * B + C
    inline static void 
    gemm(matrix<double> &A, matrix<double> &B, matrix<double> &C)
    {
      T alpha = 1;
      T beta = 1;
      char transpose = 'N';

      // cout << "call dgemm__\n";
      dgemm_(&transpose, &transpose, &A._m, &B._n, &A._n, &alpha, A.data(), &A._ld,
             B.data(), &B._ld, &beta, C.data(), &C._ld);
    }

    inline T& operator()(int i, int j) const
    { 
#ifdef BOUNDS_CHECK
      assert(i >= 0);
      assert(i < _m);
      assert(j >= 0);
      assert(j < _n);
#endif
      return _data[j*_ld + i];
    }
    
    inline bool IsSquare() { return (_m == _n); } 

    matrix &
    operator += (matrix &rhs)
    {
      assert(_m == rhs._m && _n == rhs._n);
      assert(_ld == _m && _ld == rhs._ld);
      // If the two matrices are conforming, then the addition is just
      // a stream operation
      if (_ld == _m) {
        for (int i = 0; i < _m * _n; i++) {
          _data[i] += rhs._data[i];
        }
      }
      return *this;
    }
  }; // close matrix  

  template<typename T>
  inline
  std::ostream& operator<<(std::ostream& out, matrix<T> &mat)
  {
    char strbuf[1024];
    sprintf(strbuf, "matrix: m %d, n %d, ld %d, free %d, data %p\n",
            mat.m(), mat.n(), mat.ld(), mat.needfree(), mat.data());
    out << strbuf;

#ifdef DEBUG
    out << "Matrix contents:\n";
    for (int i=0; i<mat.m(); i++) {
      for (int j=0; j<mat.n(); j++) {
        out << setprecision(16) << mat(i,j) << "\t";
      }
      out << "\n";
    }
#endif

    return out;
  }

  template<typename T>
  struct global_matrix
  {
  private:
    // The global matrix is m_blks (rows) by n_blks (columns).
    int _m_blks;
    int _n_blks;
    int _blk_sz; // use square blocks
    // The processor grid (pgrid) is tx (rows) by ty (columns).
    int _pgrid_nrow;
    int _pgrid_ncol;
    int _allocated; // flag for whether _blocks are allocated
    global_ptr<T> _blocks[MAX_BLOCKS]; // makes copy easier than dynamic allocation
    
  public:
    
    inline int m_blks() { return _m_blks; }
    inline int n_blks() { return _n_blks; }
    inline int blk_sz() { return _blk_sz; }
    inline int m()      { return _m_blks * _blk_sz; }
    inline int n()      { return _n_blks * _blk_sz; }
    inline int pgrid_nrow() { return _pgrid_nrow; }
    inline int pgrid_ncol() { return _pgrid_ncol; }
    inline int allocated() { return _allocated; }

    inline global_matrix(int m_blks = 0, int n_blks = 0, 
                           int block_sz = 0, int pgrid_nrow = 0,
                           int pgrid_ncol = 0)
    {
      assert(m_blks * n_blks <= MAX_BLOCKS);
      _m_blks = m_blks;
      _n_blks = n_blks;
      _blk_sz = block_sz;
      _pgrid_nrow = pgrid_nrow;
      _pgrid_ncol = pgrid_ncol;
      _allocated = 0;
    }

    // get (i,j)th block for the global matrix
    inline global_ptr<T> &
    operator() (int i, int j)
    {
#ifdef DEBUG
      fprintf(stderr, "global_matrix operator (%d, %d), return _block %d\n",
              i, j, i*_n_blks+j);
#endif
      
      assert(_m_blks != 0 && _n_blks != 0 && _blk_sz != 0);
      assert(i < _m_blks && j < _n_blks);
      // assert(_allocated);
      assert(i * _n_blks + j < _m_blks * _n_blks);

      return _blocks[i * _n_blks + j];
    }

    // get (i,j)th block for the global matrix
    inline matrix<T>
    get_block(int i, int j) const
    {
      assert(_m_blks != 0 && _n_blks != 0 && _blk_sz != 0);
      assert(i < _m_blks && j < _n_blks);
      assert(_allocated);
      assert(_blocks[i * _n_blks + j].islocal());
      return matrix<T>(_blk_sz, _blk_sz, _blocks[i * _n_blks + j].raw_ptr());
    }

    // convert a coordinate in the 2-D processor grid to a CPU place
    rank_t pgrid_to_node(int myrow, int mycol)
    {
      assert(myrow < _pgrid_nrow);
      assert(mycol < _pgrid_ncol);
      return myrow * _pgrid_ncol + mycol;
    }

    void node_to_pgrid(rank_t pla, int &myrow, int &mycol)
    {
      int id = pla;

      myrow = id / _pgrid_ncol;
      mycol = id % _pgrid_ncol;

      assert(myrow < _pgrid_nrow);
    }


    // allocate a global matrix with 2-D block-cyclic distribution on a
    // 2-D processor grid
    inline void
    alloc_blocks(int m_blks, int n_blks, int blk_sz,
                 int pgrid_nrow, int pgrid_ncol)
    {
      /* Error checking */
      assert(!_allocated); // need to deallocate first
      assert(m_blks > 0 && n_blks > 0 && blk_sz > 0);
      assert(pgrid_nrow > 0 && pgrid_ncol > 0
             && pgrid_nrow * pgrid_ncol == upcxx::ranks());

      _m_blks = m_blks;
      _n_blks = n_blks;
      _blk_sz = blk_sz;
      _pgrid_nrow = pgrid_nrow;
      _pgrid_ncol = pgrid_ncol;

#ifdef DEBUG
      fprintf(stderr, "global_matrix alloc_blocks(): m_blks %d, n_blks %d, blk_sz %d, pgrid_nrow %d, pgrid_ncol %d\n",
              m_blks, n_blks, blk_sz, pgrid_nrow, pgrid_nrow);
#endif

      for (int i = 0; i < m_blks; i += pgrid_nrow) {
        for (int j = 0; j< n_blks; j += pgrid_ncol) {
          for (int myrow = 0; myrow < pgrid_nrow; myrow++) {
            for (int mycol = 0; mycol <pgrid_ncol; mycol++) {
#ifdef DEBUG
              fprintf(stderr, "allocating blocks: i %d, j %d, myrow %d, mycol %d\n",
                      i, j, myrow, mycol);
#endif
              (*this)(i+myrow, j+mycol) = allocate<T>(pgrid_to_node(myrow, mycol), blk_sz*blk_sz);
            }
          }
        }
      }
      _allocated = 1;
    }

    // deallocate a global matrix with 2-D block-cyclic distribution on a
    // 2-D processor grid
    inline void free_blocks()
    {
      assert(_allocated);
      for (int i = 0; i < _m_blks; i += _pgrid_nrow) {
        for (int j = 0; j< _n_blks; j += _pgrid_ncol) {
          for (int myrow = 0; myrow < _pgrid_nrow; myrow++) {
            for (int mycol = 0; mycol < _pgrid_ncol; mycol++) {
              upcxx::deallocate<T>((*this)(i+myrow, j+mycol));
            }
          }
        }
      }
      _allocated = 0;
    }

    // gather a distributed global matrix into a local matrix
    // this is a slow operation and unoptimized and it's intended
    // for debugging and testing purposes
    inline matrix<T>* gather()
    {
      matrix<T> *local;
      matrix<T> tmp(_blk_sz, _blk_sz);
      global_ptr<T> pTmp(tmp.data());

      local = new matrix<T> (this->m(), this->n());
      assert(local != NULL);

      assert(_allocated);
      for (int i = 0; i < _m_blks; i += _pgrid_nrow) {
        for (int j = 0; j < _n_blks; j += _pgrid_ncol) {
          for (int myrow = 0; myrow < _pgrid_nrow; myrow++) {
            for (int mycol = 0; mycol < _pgrid_ncol; mycol++) {

#if 0
              // Only print when the global matrix is local
              if ((*this)(i+myrow, j+mycol).place() == my_cpu_place) {
                matrix<T> gtmp(_blk_sz, _blk_sz, (*this)(i+myrow, j+mycol).raw_ptr());
                cout << "matrix gtmp: " << gtmp;
              }
#endif
              upcxx::copy((*this)(i+myrow, j+mycol), pTmp, _blk_sz * _blk_sz);

#if 0
              printf("i %d, j %d, myrow %d, mycol %d\n", i, j, myrow, mycol);
              cout << "matrix tmp: " << tmp;
#endif

              // rearrange the data into the right location
              for (int k = 0; k < _blk_sz; k++) {
                for (int l = 0; l < _blk_sz; l++) {
                  (*local)((i+myrow)*_blk_sz+k, (j+mycol)*_blk_sz+l) = tmp(k, l);
                }
              }
            }
          }
        }
      }

      return local;
    }
  
  }; // close of gloal_matrix

  template<typename T>
  inline
  std::ostream& operator<<(std::ostream& out, global_matrix<T> &gm)
  {
    char strbuf[1024];
    sprintf(strbuf, "gloal_matrix: m_blks %d, n_blks %d, blk_sz %d, pgrid_nrow %d, pgrid_ncol %d, allocated %d\n",
            gm.m_blks(), gm.n_blks(), gm.blk_sz(), gm.pgrid_nrow(), gm.pgrid_ncol(), gm.allocated());
    out << strbuf;
#ifdef DEBUG
    out << "block pointers:\n";
    for (int i=0; i<gm.m_blks(); i++) {
      for (int j=0; j<gm.n_blks(); j++) {
        out << gm(i,j) << "\t";
      }
      out << "\n";
    }
#endif
    return out;
  }

} // close of upcxx
