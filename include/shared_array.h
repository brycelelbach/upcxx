/**
 * Multi-node 1-D shared arrays 
 *
 * See test_global_array.cpp and gups.cpp for usage examples
 */

#pragma once

#include "global_ref.h"

// #define UPCXX_UPCXX_DEBUG

namespace upcxx
{
  /**
   * \ingroup gasgroup
   * \brief shared 1-D array with 1-D block-cyclic distribution
   * static blocking factor BF, same restriction as UPC
   */
  template<typename T, size_t BF = 1>
  struct shared_array
  {
    T *_data;
    T **_alldata;
    size_t _bf; // blocking factor
    size_t _local_size;
    size_t _size;

    void global2local(const size_t global_index,
                      size_t &local_index,
                      node &node)
    {
      int nplaces = (int)gasnet_nodes(); // global_machine.node_count();
      size_t block_id = global_index / BF;
      size_t phase = global_index % BF;
      local_index = (block_id / nplaces) * BF + phase ;
      node = block_id % nplaces;
    }

    shared_array(size_t size=0)
    {
#ifdef UPCXX_DEBUG
      printf("In shared_array constructor, size %lu\n", size);
#endif
      _data = NULL;
      _bf = BF;
      _local_size = size;
      _size = size;
    }

    inline size_t size() { return _size; }

    void init(size_t sz=0)
    {
      if (_data != NULL) return;

      int nplaces = global_machine.node_count();
      if (sz != 0) _size = sz;
      _local_size = (_size + nplaces - 1) / nplaces;
      
#ifdef USE_GASNET_FAST_SEGMENT
      // YZ: may want to use the "placement new" functions
      // void * operator new[] (std::size_t, void *) throw();
      _data = (T *)gasnet_seg_alloc(sizeof(T) * _local_size);
#else
      _data = new T[_local_size];
#endif
      
      assert(_data != NULL);
      _alldata = (T **)malloc(nplaces * sizeof(T*));
      assert(_alldata != NULL);

      // for (size_t i=0; i<_local_size; i++) {
      //   _data[i] = val;
      // }

      // \Todo _data allocated in this way is not aligned!!
      gasnet_coll_gather_all(GASNET_TEAM_ALL, _alldata, &_data, sizeof(T*),
                             GASNET_COLL_IN_MYSYNC | GASNET_COLL_OUT_MYSYNC | GASNET_COLL_LOCAL);
#ifdef UPCXX_DEBUG
      printf("my node %d, _data %p\n", my_cpu_place.id(), _data);
      for (int i=0; i<nplaces; i++) {
        printf("_alldata[%d]=%p ", i, _alldata[i]);
      }
      printf("\n");
#endif
    }

    ~shared_array()
    {
      if (_data) {
#ifdef USE_GASNET_FAST_SEGMENT
        gasnet_seg_free(_data);
#else
        delete _data;
#endif
      }
      if (_alldata) free(_alldata);
    }

    // Collectively allocate a shared array of nblocks with count elements per block

    // void all_alloc(size_t nblocks, size_t count);

    global_ref<T> operator [] (size_t global_index)
    {
      // address translation
      size_t local_index;
      node n;
      global2local(global_index, local_index, n);

      // assert(_data != NULL);
      // assert(_alldata != NULL);

#ifdef UPCXX_DEBUG
      printf("shared_array [], gi %lu, li %lu, nid %d\n",
             global_index, local_index, n.id());
#endif
      // only works with statically declared (and presumably aligned) data
      return global_ref<T>(n, &_alldata[n.id()][local_index]);
    }
  }; // struct shared_array

  // init_shared_array should be called by all processes
  template<typename T>
  void init_ga(shared_array<T> *ga, T val)
  {
    // printf("node %d: ga %p, val %g\n", my_cpu_place.id(), ga, (double)val);
    ga->init(val);
  }

  // set_shared_array should be called by only one process
  template<typename T>
  void set(shared_array<T> *ga, T val)
  {
    int P = global_machine.node_count();
    for (int i=P-1; i>=0; i--)
      async(i)(init_ga<T>, ga, val);
  }
} // namespace upcxx
