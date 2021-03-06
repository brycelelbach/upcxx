/**
 * UPC++ runtime
 */

#include <list> // for outstanding event list

#include <stdio.h>
#include <assert.h>

// #define UPCXX_DEBUG

#include "upcxx.h"
#include "upcxx/upcxx_internal.h"
#include "upcxx/array_bulk_internal.h"

#ifdef UPCXX_USE_DMAPP
#include "upcxx/dmapp_channel/dmapp_helper.h"
#endif

using namespace std;

/*
 * Here are the data structures for storing the offsets of data/text
 * segments of all other nodes from the address of my data/text
 * segment.  If the segments are aligned,i.e. having the same starting
 * address, then all offsets are 0 and there is no need to store them.
 * For now, let's be optimistic and assume the segments are aligned.
 */
//uint64_t *gasnet_dataseg_offsets; // data segment offsets for all gasnet nodes
//uint64_t *gasnet_textseg_offsets; // text segment offsets for all gasnet nodes

const char *upcxx_op_strs[] = {
  "UPCXX_MAX",
  "UPCXX_MIN",
  "UPCXX_SUM",
  "UPCXX_PROD",
  "UPCXX_LAND",
  "UPCXX_BAND",
  "UPCXX_LOR",
  "UPCXX_BOR",
  "UPCXX_LXOR",
  "UPCXX_BXOR"};

namespace upcxx
{
  static gasnet_handlerentry_t AMtable[] = {
    {ASYNC_AM,                (void (*)())async_am_handler},
    {ASYNC_DONE_AM,           (void (*)())async_done_am_handler},
    {ALLOC_CPU_AM,            (void (*)())alloc_cpu_am_handler},
    {ALLOC_REPLY,             (void (*)())alloc_reply_handler},
    {FREE_CPU_AM,             (void (*)())free_cpu_am_handler},
    {LOCK_AM,                 (void (*)())shared_lock::lock_am_handler},
    {LOCK_REPLY,              (void (*)())shared_lock::lock_reply_handler},
    {UNLOCK_AM,               (void (*)())shared_lock::unlock_am_handler},
    {INC_AM,                  (void (*)())inc_am_handler},
    {FETCH_ADD_U64_AM,        (void (*)())fetch_add_am_handler<uint64_t>},
    {FETCH_ADD_U64_REPLY,     (void (*)())fetch_add_reply_handler<uint64_t>},

    gasneti_handler_tableentry_with_bits(copy_and_signal_request),
    gasneti_handler_tableentry_with_bits(copy_and_signal_reply),

#ifdef UPCXX_HAVE_MD_ARRAY
    /* array_bulk.c */
    gasneti_handler_tableentry_with_bits(misc_delete_request),
    gasneti_handler_tableentry_with_bits(misc_alloc_request),
    gasneti_handler_tableentry_with_bits(misc_alloc_reply),
    gasneti_handler_tableentry_with_bits(array_alloc_request),
    gasneti_handler_tableentry_with_bits(strided_pack_request),
    gasneti_handler_tableentry_with_bits(strided_pack_reply),
    gasneti_handler_tableentry_with_bits(strided_unpackAll_request),
    gasneti_handler_tableentry_with_bits(strided_unpack_reply),
    gasneti_handler_tableentry_with_bits(strided_unpackData_request),
    gasneti_handler_tableentry_with_bits(strided_unpackOnly_request),
    gasneti_handler_tableentry_with_bits(sparse_simpleScatter_request),
    gasneti_handler_tableentry_with_bits(sparse_done_reply),
    gasneti_handler_tableentry_with_bits(sparse_generalScatter_request),
    gasneti_handler_tableentry_with_bits(sparse_simpleGather_request),
    gasneti_handler_tableentry_with_bits(sparse_simpleGather_reply),
    gasneti_handler_tableentry_with_bits(sparse_generalGather_request),
#endif
  };

  // static gasnet_seginfo_t* seginfo_table; // GASNet segments info, unused for now
  gasnet_seginfo_t *all_gasnet_seginfo;
  gasnet_seginfo_t *my_gasnet_seginfo;
  mspace _gasnet_mspace = 0;
  gasnet_nodeinfo_t *all_gasnet_nodeinfo;
  gasnet_nodeinfo_t *my_gasnet_nodeinfo;
  gasnet_node_t my_gasnet_supernode;

#if defined(UPCXX_THREAD_SAFE) || defined(GASNET_PAR)
  upcxx_mutex_t async_lock = UPCXX_MUTEX_INITIALIZER;
  upcxx_mutex_t in_task_queue_lock = UPCXX_MUTEX_INITIALIZER;
  upcxx_mutex_t out_task_queue_lock = UPCXX_MUTEX_INITIALIZER;
  upcxx_mutex_t all_events_lock = UPCXX_MUTEX_INITIALIZER;
  // protect gasnet calls if GASNET_PAR mode is not used
  upcxx_mutex_t gasnet_call_lock = UPCXX_MUTEX_INITIALIZER;
#endif


  queue_t *in_task_queue = NULL;
  queue_t *out_task_queue = NULL;
  event *system_event;
  bool init_flag = false;  //  equals 1 if the backend is initialized
  std::list<event*> *outstanding_events;
  std::vector<void *> *pending_shared_var_inits = NULL;

  rank_t _global_ranks; /**< total ranks of the parallel job */
  rank_t _global_myrank; /**< my rank in the global universe */

  int env_use_am_for_copy_and_set;
  int env_use_dmapp;

  std::vector<void*> *pending_array_inits = NULL;

  int init(int *pargc, char ***pargv)
  {
#ifdef UPCXX_DEBUG
    cerr << "upcxx::init()\n";
#endif
    static upcxx_mutex_t init_lock = UPCXX_MUTEX_INITIALIZER;
    upcxx_mutex_lock(&init_lock);

    if (init_flag) {
      return UPCXX_ERROR;
    }

#ifdef GASNETI_USE_HUGETLBFS
    // initialize HUGETLBFS before calling gasnet_init
    // can't call setup_libhugetlbfs() because it's static inside libhugetlbfs.
#endif

#ifdef UPCXX_DEBUG
    cerr << "gasnet_init()\n";
#endif

#if (GASNET_RELEASE_VERSION_MINOR > 24 || GASNET_RELEASE_VERSION_MAJOR > 1)
    gasnet_init(NULL, NULL); // init gasnet
#else
    if (pargc != NULL && pargv != NULL) {
      gasnet_init(pargc, pargv); // init gasnet
    } else {
      int dummy_argc = 1;
      char *dummy_argv = new char[6]; // "upcxx"
      char **p_dummy_argv = &dummy_argv;
      gasnet_init(&dummy_argc, &p_dummy_argv); // init gasnet
    }
#endif

    // allocate UPC++ internal global variable before anything else
    _team_stack = new std::vector<team *>;
    system_event = new event;
    outstanding_events = new std::list<event *>;
    events = new event_stack;

#ifdef UPCXX_DEBUG
    cerr << "gasnet_attach()\n";
#endif
    UPCXX_CALL_GASNET(
        GASNET_CHECK_RV(
            gasnet_attach(AMtable,
                          sizeof(AMtable)/sizeof(gasnet_handlerentry_t),
                          gasnet_getMaxLocalSegmentSize(),
                          0)));

    // The following collectives initialization only works with SEG build
    // \TODO: add support for the PAR-SYNC build
    // gasnet_coll_init(NULL, 0, NULL, 0, 0); // init gasnet collectives
#ifdef UPCXX_DEBUG
    cerr << "init_collectives()\n";
#endif
    init_collectives();

    _global_ranks = gasnet_nodes();
    _global_myrank = gasnet_mynode();

    // Get gasnet_nodeinfo for PSHM support
    all_gasnet_nodeinfo
      = (gasnet_nodeinfo_t *)malloc(sizeof(gasnet_nodeinfo_t) * _global_ranks);
    assert(all_gasnet_nodeinfo != NULL);
    if (gasnet_getNodeInfo(all_gasnet_nodeinfo, _global_ranks) != GASNET_OK) {
      cerr << "Unable to get GASNet nodeinfo: aborting\n";
      gasnet_exit(1);
    }
    my_gasnet_nodeinfo = &all_gasnet_nodeinfo[_global_myrank];
    my_gasnet_supernode = my_gasnet_nodeinfo->supernode;

#ifdef UPCXX_DEBUG
    fprintf(stderr, "rank %u, total_shared_var_sz %lu, shared_var_addr %p\n",
            gasnet_mynode(), total_shared_var_sz, shared_var_addr);
#endif

    // Initialize Team All
    team_all.init_global_team();

    // Initialize PSHM teams
    init_pshm_teams(all_gasnet_nodeinfo, _global_ranks);

    // Initialize the async task queues and the async locks
    in_task_queue = queue_new();
    out_task_queue = queue_new();
    assert(in_task_queue != NULL);
    assert(out_task_queue != NULL);

#ifdef UPCXX_USE_DMAPP
    init_dmapp();
    env_use_dmapp = gasnett_getenv_yesno_withdefault("UPCXX_USE_DMAPP", 1);
#else
    env_use_dmapp = 0;
#endif

#ifdef UPCXX_HAVE_MD_ARRAY
    // Initialize array bulk operations
    array_bulk_init();
#endif

    env_use_am_for_copy_and_set = gasnett_getenv_yesno_withdefault("UPCXX_USE_AM_FOR_COPY_AND_SET", 0);

    init_flag = true;

    // run the pending_shared_var_inits
    if (pending_shared_var_inits != NULL)
      run_pending_shared_var_inits();

    // run the pending initializations of shared arrays
    // run_pending_array_inits();

    barrier();

    upcxx_mutex_unlock(&init_lock);
    return UPCXX_SUCCESS;
  }

  static int _finalize()
  {
    if (!init_flag) return UPCXX_ERROR;

    async_wait();
    while (advance() > 0);
    barrier();
    // gasnet_exit(0);
    extern bool _threads_deprecated_warned;
    if (global_myrank() == 0 && _threads_deprecated_warned) {
      std::cerr << "WARNING: THREADS and MYTHREADS are deprecated;\n"
                << "         use upcxx::ranks() and upcxx::myranks() instead!"
                << "\n";
    }

#ifdef UPCXX_DEBUG
    printf("Rank %u: done finalize()\n", global_myrank());
#endif

    init_flag = false;
    return UPCXX_SUCCESS;
  }

  int finalize()
  {
    return _finalize();
  }

  bool is_init() { return init_flag; }

  uint32_t global_ranks()
  {
    assert(init_flag == true);
    return _global_ranks;
  }

  uint32_t global_myrank()
  {
    assert(init_flag == true);
    return _global_myrank;
  }

  // Active Message handlers
  void inc_am_handler(gasnet_token_t token, void *buf, size_t nbytes)
  {
    struct inc_am_t *am = (struct inc_am_t *)buf;
    // gasnett_strongatomic64_X
    // uint64_t gasnett_atomic64_add(gasnett_atomic64_t *p, uint64_t v, int flags);
    // Atomically add value v to *p, returning the new value.

    long *tmp = (long *)am->ptr;
    (*tmp)++;
  }

  void async_am_handler(gasnet_token_t token, void *buf, size_t nbytes)
  {
    async_task *task;

    task = (async_task *)malloc(nbytes);
    assert(task != NULL);
    memcpy(task, buf, nbytes);

    // assert(async_task_queue != NULL);
    assert(in_task_queue != NULL);

#ifdef UPCXX_DEBUG
    cerr << "Rank " << global_myrank() << " is about to enqueue an async task.\n";
    cerr << *task << endl;
#endif
    // enqueue the async task
    upcxx_mutex_lock(&in_task_queue_lock);
    queue_enqueue(in_task_queue, task);
    upcxx_mutex_unlock(&in_task_queue_lock);
  }

  void async_done_am_handler(gasnet_token_t token, void *buf, size_t nbytes)
  {
    async_done_am_t *am = (async_done_am_t *)buf;

    assert(nbytes == sizeof(async_done_am_t));

#ifdef UPCXX_DEBUG
    gasnet_node_t src;
    gasnet_AMGetMsgSource(token, &src);
    fprintf(stderr, "Rank %u receives async done from %u",
            global_myrank(), src);
#endif

    if (am->ack_event != NULL) {
      am->ack_event->decref();
      // am->future->_rv = am->_rv;
#ifdef UPCXX_DEBUG
      fprintf(stderr, "Rank %u receives async done from %u. event count %d\n",
              global_myrank(), src, am->ack_event->_count);
#endif
    }
  }

  int advance_in_task_queue(queue_t *inq, int max_dispatched)
  {
    async_task *task;
    int num_dispatched = 0;

    UPCXX_CALL_GASNET(gasnet_AMPoll()); // make progress in GASNet

    // Execute tasks in the async queue
    while (!queue_is_empty(inq)) {
      // dequeue an async task
      upcxx_mutex_lock(&in_task_queue_lock);
      task = (async_task *)queue_dequeue(inq);
      upcxx_mutex_unlock(&in_task_queue_lock);

      if (task == NULL) break;
      assert (task->_callee == global_myrank());

#ifdef UPCXX_DEBUG
      cerr << "Rank " << global_myrank() << " is about to execute async task.\n";
      cerr << *task << "\n";
#endif

        // execute the async task
        if (task->_fp) {
          (*task->_fp)(task->_args);
        }

        if (task->_ack != NULL) {
          if (task->_caller == global_myrank()) {
            // local event acknowledgment
            task->_ack->decref(); // need to enqueue callback tasks
#ifdef UPCXX_DEBUG
            fprintf(stderr, "Rank %u completes a local task. event count %d\n",
                    global_myrank(), task->_ack->_count);
#endif
          } else {
            // send an ack message back to the caller of the async task
            async_done_am_t am;
            am.ack_event = task->_ack;
            UPCXX_CALL_GASNET(
                GASNET_CHECK_RV(
                    gasnet_AMRequestMedium0(task->_caller,
                                            ASYNC_DONE_AM,
                                            &am,
                                            sizeof(am))));
          }
        }
        delete task;
        num_dispatched++;
        if (num_dispatched >= max_dispatched) break;
    }; // end of while (!queue_is_empty(inq))

    return num_dispatched;
  } // end of poll_in_task_queue;

  int advance_out_task_queue(queue_t *outq, int max_dispatched)
  {
    async_task *task;
    int num_dispatched = 0;

    // Execute tasks in the async queue
    while (!queue_is_empty(outq)) {
      // dequeue an async task
      upcxx_mutex_lock(&out_task_queue_lock);
      task = (async_task *)queue_dequeue(outq);
      upcxx_mutex_unlock(&out_task_queue_lock);
      if (task == NULL) break;
      assert (task->_callee != global_myrank());

#ifdef UPCXX_DEBUG
      cerr << "Rank " << global_myrank() << " is about to send outgoing async task.\n";
      cerr << *task << endl;
#endif

      // remote async task
      // Send AM "there" to request async task execution
      UPCXX_CALL_GASNET(
          GASNET_CHECK_RV(
              gasnet_AMRequestMedium0(task->_callee, ASYNC_AM,
                                      task, task->nbytes())));

      delete task;
      num_dispatched++;
      if (num_dispatched >= max_dispatched) break;
    } // end of while (!queue_is_empty(outq))

    return num_dispatched;
  } // end of poll_out_task_queue()

  int advance(int max_in, int max_out)
  {
    int num_in = 0;
    int num_out = 0;

    max_in = (max_in >= 0) ? max_in : MAX_DISPATCHED_IN;
    max_out = (max_out >= 0) ? max_out : MAX_DISPATCHED_OUT;

    if (max_in > 0) {
      num_in = advance_in_task_queue(in_task_queue, max_in);
      assert(num_in >= 0);
    }
    if (max_out > 0) {
      num_out = advance_out_task_queue(out_task_queue, max_out);
      assert(num_out >= 0);
    }

    // check outstanding events
    upcxx_mutex_lock(&all_events_lock);
    if (!outstanding_events->empty()) {
      for (std::list<event*>::iterator it = outstanding_events->begin();
           it != outstanding_events->end(); ++it) {
        event *e = (*it);
        assert(e != NULL);
#ifdef UPCXX_DEBUG
        fprintf(stderr, "P %u: Number of outstanding_events %u, Advance event: %p\n", 
                global_myrank(), outstanding_events->size(), e);
#endif
        if (e->_async_try()) break;
      }
    }
    upcxx_mutex_unlock(&all_events_lock);

    return num_out + num_in;
  } // advance()

  int peek()
  {
    UPCXX_CALL_GASNET(gasnet_AMPoll());
    return ! (queue_is_empty(in_task_queue) && queue_is_empty(out_task_queue));
  } // peek()

  volatile int exit_signal = 0;

  void signal_exit_am()
  {
    exit_signal = 1;
  }

  void signal_exit()
  {
    // Only the master process should wait for incoming tasks
    assert(global_myrank() == 0);

    for (rank_t i = 1; i < ranks(); i++) {
      async(i)(signal_exit_am);
    }
    async_wait();
  }

  void wait_for_incoming_tasks()
  {
    // Only the worker processes should wait for incoming tasks
    assert(global_myrank() != 0);

    // Wait until the master process sends out an exit signal
    while (!exit_signal) {
      advance();
    }
  }

  int remote_inc(global_ptr<long> ptr)
  {
    inc_am_t am;
    am.ptr = ptr.raw_ptr();
    UPCXX_CALL_GASNET(
        GASNET_CHECK_RV(
            gasnet_AMRequestMedium0(ptr.where(), INC_AM, &am, sizeof(am))));
    return UPCXX_SUCCESS;
  }

#if defined(UPCXX_THREAD_SAFE) || defined(GASNET_PAR)
  upcxx_mutex_t upcxxi_mutex_for_memory = UPCXX_MUTEX_INITIALIZER;
#endif

  // free memory
  void gasnet_seg_free(void *p)
  {
    if (_gasnet_mspace == 0) {
      fprintf(stderr, "Error: the gasnet memory space is not initialized.\n");
      fprintf(stderr, "It is likely due to the pointer (%p) was not from hp_malloc().\n",
              p);
      UPCXX_CALL_GASNET(gasnet_exit(1));
    }
    assert(p != 0);

    upcxx_mutex_lock(&upcxxi_mutex_for_memory);
    mspace_free(_gasnet_mspace, p);
    upcxx_mutex_unlock(&upcxxi_mutex_for_memory);
  }

  void *gasnet_seg_memalign(size_t nbytes, size_t alignment)
  {
    upcxx_mutex_lock(&upcxxi_mutex_for_memory);

    if (_gasnet_mspace== 0) {
      init_gasnet_seg_mspace();
    }
    void *m = mspace_memalign(_gasnet_mspace, alignment, nbytes);

    upcxx_mutex_unlock(&upcxxi_mutex_for_memory);

    return m;
  }

  void *gasnet_seg_alloc(size_t nbytes)
  {
    return gasnet_seg_memalign(nbytes, 64);
  }

  // Return true if they physical memory of rank r can be shared and
  // directly accessed by the calling process (usually via some kind
  // of process-shared memory mechanism)
  bool is_memory_shared_with(rank_t r)
  {
    assert(all_gasnet_nodeinfo != NULL);
    return all_gasnet_nodeinfo[r].supernode == my_gasnet_supernode;
  }

  // Return local version of remote in-supernode address if the data
  // pointed to is on the same supernode (shared-memory node)
  void *pshm_remote_addr2local(rank_t r, void *addr)
  {
#if GASNET_PSHM
    if (is_memory_shared_with(r) == false)
      return NULL;

    return (void *)((char *)addr + all_gasnet_nodeinfo[r].offset);
#else
    return NULL; // always return NULL if no PSHM support
#endif
  }

} // namespace upcxx
