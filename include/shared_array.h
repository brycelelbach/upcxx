/**
 * Multi-node 1-D shared arrays
 *
 * See test_shared_array.cpp and gups.cpp for usage examples
 */

#pragma once

#include "global_ref.h"
#include "coll_flags.h"

// #define UPCXX_DEBUG

namespace upcxx
{
  extern std::vector<void*> *pending_array_inits;

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
    size_t _type_size;

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
      _local_size = 0;
      _size = size;
      _type_size = sizeof(T);

      if (upcxx::is_init()) {
        // init the array if upcxx is already initialized
        init(size, blk_sz);
      } else {
        // queue up the initialization if upcxx is not yet initialized
        assert(pending_array_inits != NULL);
        pending_array_inits->push_back(this);
#ifdef UPCXX_DEBUG
        printf("pending_array_inits size %lu\n", pending_array_inits->size());
#endif
      }
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
    void init(size_t sz, size_t blk_sz)
    {
      if (_data != NULL) return;

      rank_t nplaces = ranks();
      if (sz != 0) _size = sz;
      if (blk_sz != 0) _blk_sz = blk_sz;
      _local_size = (_size + nplaces - 1) / nplaces;

      // allocate the data space in bytes
      _data = (T*)allocate(myrank(), _local_size * _type_size).raw_ptr();

      assert(_data != NULL);
      _alldata = (T **)malloc(nplaces * sizeof(T*));
      assert(_alldata != NULL);

      // \Todo _data allocated in this way is not aligned!!
      gasnet_coll_handle_t h;
      h = gasnet_coll_gather_all_nb(GASNET_TEAM_ALL, _alldata, &_data, sizeof(T*),
                                    UPCXX_GASNET_COLL_FLAG);
      while(gasnet_coll_try_sync(h) != GASNET_OK) {
        advance(); // neeed to keep polling the task queue whlie waiting
      }
#ifdef UPCXX_DEBUG
      printf("my rank %d, size %lu, blk_sz %lu, local_sz %lu, type_sz %lu, _data %p\n",
             myrank(), size(), get_blk_sz(), _local_size, _type_size, _data);
      for (int i=0; i<nplaces; i++) {
        printf("_alldata[%d]=%p ", i, _alldata[i]);
      }
      printf("\n");
#endif
    }

    void init()
    {
      init(size(), get_blk_sz());
    }

    void init(size_t sz)
    {
      init(sz, get_blk_sz());
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
      printf("shared_array [], gi %lu, li %lu, rank %u\n",
             global_index, local_index, rank);
#endif
      // only works with statically declared (and presumably aligned) data
      return global_ref<T>(rank, &_alldata[rank][local_index]);
    }
  }; // struct shared_array

  // init should be called by all processes
  template<typename T>
  static void init_ga(shared_array<T> *sa, size_t sz, size_t blk_sz)
  {
    sa->init(sz, blk_sz);
  }

  struct __init_shared_array
  {
    __init_shared_array()
    {
#ifdef UPCXX_DEBUG
      printf("Constructing __dumb_obj_for_init_shared_array\n");
#endif
      if (pending_array_inits == NULL)
        pending_array_inits = new std::vector<void*>;
      assert(pending_array_inits != NULL);
    }

    ~__init_shared_array()
    {
#ifdef UPCXX_DEBUG
      printf("Destructing __dumb_obj_for_init_shared_array\n");
#endif
      assert(pending_array_inits != NULL);
      delete pending_array_inits;
    }
  };
  static __init_shared_array __dumb_obj_for_init_shared_array;

  static inline void run_pending_array_inits()
  {
    assert(pending_array_inits != NULL);

#ifdef UPCXX_DEBUG
    printf("Running run_pending_array_inits(). pending_array_inits sz %lu\n",
           pending_array_inits->size());
#endif

#if UPCXX_HAVE_CXX11
    for (auto it = pending_array_inits->begin();
         it != pending_array_inits->end(); ++it) {
#else
    for (std::vector<void*>::iterator it = pending_array_inits->begin();
         it != pending_array_inits->end(); ++it) {
#endif
      shared_array<void> *current = (shared_array<void> *)*it;
#ifdef UPCXX_DEBUG
      printf("Rank %u: Init shared_array %p, size %lu, blk_sz %lu\n",
             myrank(), current, current->size(), current->get_blk_sz());
#endif
      current->init();
    }
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
