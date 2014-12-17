/**
 * Multi-node 1-D shared arrays 
 *
 * See test_shared_array.cpp and gups.cpp for usage examples
 */

#pragma once

#include "global_ref.h"

// #define UPCXX_UPCXX_DEBUG

namespace upcxx
{
  /**
   * \ingroup gasgroup
   * \brief shared 1-D array with 1-D block-cyclic distribution
   *
   * In the current implementation, the application needs to call
   * the init() member function for each shared array object after
   * calling upcxxx::init().  For example, sa.init(total_sz, blk_sz);
   *
   * In UPC++, the block size (blk_sz) can be changed at runtime
   * by set_blk_sz().
   */
  template<typename T, size_t BLK_SZ = 1>
  struct shared_array
  {
    T *_data;
    T **_alldata;
    size_t _blk_sz; // blocking factor
    size_t _local_size;
    size_t _size;

    void global2local(const size_t global_index,
                      size_t &local_index,
                      rank_t &rank)
    {
      rank_t nplaces = ranks();
      size_t block_id = global_index / _blk_sz;
      size_t phase = global_index % _blk_sz;
      local_index = (block_id / nplaces) * _blk_sz + phase ;
      rank = block_id % nplaces;
    }

    shared_array(size_t size=0, size_t blk_sz=BLK_SZ)
    {
#ifdef UPCXX_DEBUG
      printf("In shared_array constructor, size %lu\n", size);
#endif
      _data = NULL;
      _alldata = NULL;
      _blk_sz = blk_sz;
      _local_size = size;
      _size = size;
    }

    /**
     * Return the total size of the shared array.
     * This value is not runtime changeable.
     */
    inline size_t size() { return _size; }

    /**
     * Get the current block size (a.k.a. blocking factor in UPC)
     */
    inline size_t get_blk_sz() { return _blk_sz; }

    /**
     * Set the current block size (a.k.a. blocking factor in UPC)
     */
    inline void set_blk_sz(size_t blk_sz) { _blk_sz = blk_sz; }

    /**
     * Initialize the shared array, which should be done after upcxx::init().
     * This is a collective function and all ranks should agree on the same
     * shared array size (sz) and the blocking factor (blk_sz) or the shared
     * array allocated would have undefined behavior (e.g., segmentation fault
     * due to insufficient memory allocated on some ranks).
     *
     * \param sz total size (# of elements) of the shared array
     * \param blk_sz the blocking factor (# of elements per block)
     */
    void init(size_t sz=0, size_t blk_sz=BLK_SZ)
    {
      if (_data != NULL) return;

      int nplaces = THREADS;
      if (sz != 0) _size = sz;
      if (blk_sz != 0) _blk_sz = blk_sz;
      _local_size = (_size + nplaces - 1) / nplaces;
      
      _data = allocate<T>(myrank(), _local_size).raw_ptr();
      
      assert(_data != NULL);
      _alldata = (T **)malloc(nplaces * sizeof(T*));
      assert(_alldata != NULL);

      // \Todo _data allocated in this way is not aligned!!
      gasnet_coll_handle_t h;
      h = gasnet_coll_gather_all_nb(GASNET_TEAM_ALL, _alldata, &_data, sizeof(T*),
                                    GASNET_COLL_IN_MYSYNC | GASNET_COLL_OUT_MYSYNC | GASNET_COLL_LOCAL);
      while(gasnet_coll_try_sync(h) != GASNET_OK) {
        advance(); // neeed to keep polling the task queue whlie waiting
      }
#ifdef UPCXX_DEBUG
      printf("my rank %d, _data %p\n", myrank(), _data);
      for (int i=0; i<nplaces; i++) {
        printf("_alldata[%d]=%p ", i, _alldata[i]);
      }
      printf("\n");
#endif
    }

    ~shared_array()
    {
      if (_alldata) free(_alldata);
      if (_data != NULL)
        deallocate(_data); // _data is from the global address space
    }

    /**
     * Collectively allocate a shared array of nblocks with blk_sz elements
     * per block.  This is similar to init() except that init takes the total
     * number of elements as argument whereas all_alloc takes the total number
     * of blocks as argument (similar to upc_all_alloc).
     *
     * \param nblocks total number of blocks
     * \param blk_sz the blocking factor (# of elements per block)
     */
    void all_alloc(size_t nblocks, size_t blk_sz=BLK_SZ)
    {
      if (_alldata != NULL || _data != NULL) {
        // fatal error!
      }

      this->init(nblocks*blk_sz, blk_sz);
    }

    global_ref<T> operator [] (size_t global_index)
    {
      // address translation
      size_t local_index;
      rank_t rank;
      global2local(global_index, local_index, rank);

      // assert(_data != NULL);
      // assert(_alldata != NULL);

#ifdef UPCXX_DEBUG
      printf("shared_array [], gi %lu, li %lu, nid %d\n",
             global_index, local_index, n.id());
#endif
      // only works with statically declared (and presumably aligned) data
      return global_ref<T>(rank, &_alldata[rank][local_index]);
    }
  }; // struct shared_array

  // init_shared_array should be called by all processes
  template<typename T>
  void init_ga(shared_array<T> *ga, T val)
  {
    // printf("myrank %d: ga %p, val %g\n", myrank(), ga, (double)val);
    ga->init(val);
  }

  // set_shared_array should be called by only one process
  template<typename T>
  void set(shared_array<T> *ga, T val)
  {
    int P = ranks();
    for (int i=P-1; i>=0; i--)
      async(i)(init_ga<T>, ga, val);
  }
} // namespace upcxx
