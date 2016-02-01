/**
 * Shared variable implementation
 */

#pragma once

#include "gasnet_api.h"
#include "allocate.h"

// #define UPCXX_DEBUG

namespace upcxx
{
  // **************************************************************************
  // Implement global address space shared variable template
  // **************************************************************************

  extern std::vector<void *> *pending_shared_var_inits;

  /**
   * @ingroup gasgroup
   * @brief globally shared variable
   *
   * shared_var variables can only be defined in the file scope
   * (not inside a function).  Global data reside on CPU place 0
   * (node 0). It only supports contiguous data types that can
   * use direct data memcpy (shallow copy) for assignment operations
   * without a customized copy function (deep copy) because the
   * system doesn't have a general mechanism to perform deep copy
   * across nodes.
   *
   * Example usage:
   *   shared_var<int> x=123;
   *
   *   void foo() { // can be executed on any cpu place
   *     int y = x.get();
   *     x = 456;
   *   }
   *
   *   std::cout << x;
   *   printf("x=%d\n", (int)x); // explicit casting is needed for printf
   */
  template<typename T>
  struct shared_var
  {
  private:
    global_ptr<T> _ptr;
    size_t _type_size;
    T _val;

  public:
    shared_var() : _type_size(sizeof(T))
    {
      if (upcxx::is_init()) {
        init();
      } else {
        enqueue_init();
      }
    }

    inline shared_var(const T &val) : _type_size(sizeof(T)), _val(val)
    {
      if (upcxx::is_init()) {
        init();
      } else {
        enqueue_init();
      }
    }

    // copy constructor
    inline
    shared_var(const shared_var<T> &g) : _type_size(g._type_size), _val(g._val)
    {
      if (upcxx::is_init()) {
        init();
      } else {
        enqueue_init();
      }
    }

    // init is a collective operation
    inline void init(rank_t where=0)
    {
#ifdef UPCXX_DEBUG
      std::cerr << "shared_var.init() " << myrank() << ": where " << where
                << " _type_size " << _type_size << "\n";
#endif
      // allocate the data space in bytes
      if (myrank() == where) {
        _ptr = allocate(where, _type_size);
      }

      gasnet_coll_handle_t h;
      h = gasnet_coll_broadcast_nb(GASNET_TEAM_ALL, &_ptr, where, &_ptr, sizeof(_ptr),
                                   UPCXX_GASNET_COLL_FLAG);
      while(gasnet_coll_try_sync(h) != GASNET_OK) {
        advance(); // need to keep polling the task queue while waiting
      }
#ifdef UPCXX_DEBUG
      std::cerr << "shared_var.init() " << myrank() << ": _ptr " << _ptr << std::endl;
#endif

#ifdef UPCXX_HAVE_CXX11
      assert(_ptr != nullptr);
#else
      assert(_ptr.raw_ptr() != NULL);
#endif
    }

    inline void enqueue_init()
    {
      if (pending_shared_var_inits == NULL)
        pending_shared_var_inits = new std::vector<void*>;
      assert(pending_shared_var_inits!= NULL);
      pending_shared_var_inits->push_back((void *)this);
    }

    T& get()
    {
      // gasnet_get(&_val, 0, (char *)shared_var_addr+_my_offset, sizeof(T));
      copy<T>(_ptr, &_val, 1);
      return _val;
    }

    void put(const T &val)
    {
      _val = val;
      // gasnet_put(0, (char *)shared_var_addr+_my_offset, &_val, sizeof(T));
      copy<T>(&_val, _ptr, 1);
    }

    shared_var<T>& operator =(const T &val)
    {
      put(val);
      return *this;
    }

#define UPCXX_SHARED_VAR_ASSIGN_OP(OP) \
    shared_var<T>& operator OP(const T &a) \
    { \
      /* gasnet_get(&_val, 0, (char *)shared_var_addr+_my_offset, sizeof(T)); */ \
      copy<T>(_ptr, &_val, 1); \
      _val OP a; \
      put(_val); \
      return *this; \
    }

    UPCXX_SHARED_VAR_ASSIGN_OP(+=)

    UPCXX_SHARED_VAR_ASSIGN_OP(-=)

    UPCXX_SHARED_VAR_ASSIGN_OP(*=)

    UPCXX_SHARED_VAR_ASSIGN_OP(/=)

    UPCXX_SHARED_VAR_ASSIGN_OP(%=)

    UPCXX_SHARED_VAR_ASSIGN_OP(^=)

    UPCXX_SHARED_VAR_ASSIGN_OP(|=)

    UPCXX_SHARED_VAR_ASSIGN_OP(&=)

    UPCXX_SHARED_VAR_ASSIGN_OP(<<=)

    UPCXX_SHARED_VAR_ASSIGN_OP(>>=)

    //////

    // copy assignment
    shared_var<T>& operator =(shared_var<T> &g)
    {
      if (this != &g) {
        put(g.get());
      }
      return *this;
    }

    // type conversion operator
    operator T()
    {
      return get();
    }

    inline global_ptr<T> operator &()
    {
      return global_ptr<T>(&_val, 0);
    }

    inline size_t type_size() { return _type_size; }

  }; // struct shared_var

  static inline void run_pending_shared_var_inits()
  {
#ifdef UPCXX_DEBUG
    printf("Running shared_var::run_pending_inits(). pending_inits sz %lu\n",
           pending_shared_var_inits->size());
#endif

#ifdef UPCXX_HAVE_CXX11
    for (auto it = pending_shared_var_inits->begin();
        it != pending_shared_var_inits->end(); ++it) {
#else
    for (std::vector<void*>::iterator it = pending_shared_var_inits->begin();
        it != pending_shared_var_inits->end(); ++it) {
#endif
      shared_var<char> *current = (shared_var<char> *)*it;
#ifdef UPCXX_DEBUG
      printf("Rank %u: Init shared_var %p, size %lu\n",
             myrank(), current, current->type_size());
#endif
      current->init();
    }
  }
} // namespace upcxx
