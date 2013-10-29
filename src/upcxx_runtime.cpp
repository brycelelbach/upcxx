/**
 * UPCXX Lib
 */

#include <stdio.h>
#include <assert.h>

#include "upcxx.h"
#include "upcxx_internal.h"

// #define DEBUG

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

// static gasnet_seginfo_t* seginfo_table; // GASNet segments info, unused for now
gasnet_seginfo_t *all_gasnet_seginfo;
gasnet_seginfo_t *my_gasnet_seginfo;
mspace _gasnet_mspace = 0;

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
    {INC_AM,                  (void (*)())inc_am_handler}
  };

  int *gpu_id_map;
  gasnet_hsl_t async_lock;
  queue_t *async_task_queue = NULL;
  event default_event;
  int init_flag = 0;  //  equals 1 if the backend is initialized

  // maybe non-zero: = address of global_var_offset on node 0 - address of global_var_offset
  void *shared_var_addr = NULL;
  size_t total_shared_var_sz = 0;

  int init(int *pargc, char ***pargv)
  {
#ifdef DEBUG    
    cerr << "upcxx::init()" << endl;
#endif

    if (init_flag) {
      return UPCXX_ERROR;
    }

    gasnet_init(pargc, pargv); // init gasnet
    GASNET_SAFE(gasnet_attach(AMtable,
                              sizeof(AMtable)/sizeof(gasnet_handlerentry_t),
                              gasnet_getMaxLocalSegmentSize(),
                              0));
    // The following collectives initialization only works with SEG build
    // \TODO: add support for the PAR-SYNC build
    // gasnet_coll_init(NULL, 0, NULL, 0, 0); // init gasnet collectives
    init_collectives();
      
    int node_count = gasnet_nodes();
    int my_node_id = gasnet_mynode();
    int my_cpu_count = 1; // may read from env

    my_node = node(my_node_id, my_cpu_count);

    // Gather all nodes info.
    node *all_nodes = new node[node_count];

    gasnet_coll_gather_all(GASNET_TEAM_ALL, all_nodes, &my_node, sizeof(node),
                           UPCXX_GASNET_COLL_FLAG);

    global_machine.init(node_count, all_nodes, my_node_id);
    my_processor = processor(my_node_id, 0); // we have one cpu per place at the moment

    // Allocate gasnet segment space for shared_var
    if (total_shared_var_sz != 0) {
#ifdef USE_GASNET_FAST_SEGMENT
      shared_var_addr = gasnet_seg_alloc(total_shared_var_sz);
#else
      shared_var_addr = malloc(total_shared_var_sz);
#endif
      assert(shared_var_addr != NULL);
      void *tmp_addr = shared_var_addr;
      gasnet_coll_broadcast(GASNET_TEAM_ALL, &shared_var_addr, 0, &tmp_addr,
                            sizeof(void *), UPCXX_GASNET_COLL_FLAG);
    }

    // Initialize Team All
    range r_all(0, gasnet_nodes());
    team_all.init(0, gasnet_nodes(), gasnet_mynode(), r_all, GASNET_TEAM_ALL);

    // Because we assume the data and text segments of processes are
    // aligned (offset is always 0).  We are not using the offsets arrays for
    // now.
    /*
      gasnet_dataseg_offsets = (uint64_t *)malloc(sizeof(uint64_t) * gasnet_nodes());
      gasnet_textseg_offsets = (uint64_t *)malloc(sizeof(uint64_t) * gasnet_nodes());
      assert(gasnet_dataseg_offsets != NULL);
      assert(gasnet_textseg_offsets != NULL);
    */

    // Initialize the async task queue and the async lock
    gasnet_hsl_init(&async_lock);
    async_task_queue = queue_new();
    assert(async_task_queue != NULL);;

    barrier();

    init_flag = 1;
    return UPCXX_SUCCESS;
  }
    
  int finalize()
  {
    //barrier();
    gasnet_exit(0);
    return UPCXX_SUCCESS;
  }

  // Active Message handlers
  void inc_am_handler(gasnet_token_t token, void *buf, size_t nbytes)
  {
    struct inc_am_t *am = (struct inc_am_t *)buf;
    long *tmp = (long *)am->ptr;
    (*tmp)++;
  }

  void async_am_handler(gasnet_token_t token, void *buf, size_t nbytes)
  {
    async_task *task;

    task = (async_task *)malloc(nbytes);
    assert(task != NULL);
    memcpy(task, buf, nbytes);
      
    assert(async_task_queue != NULL);
#ifdef DEBUG
    cerr << my_node << " is about to enqueue an async task.\n";
    cerr << *task << endl;
#endif
    // enqueue the async task
    gasnet_hsl_lock(&async_lock);
    queue_enqueue(async_task_queue, task);
    gasnet_hsl_unlock(&async_lock);
  }

  void alloc_cpu_am_handler(gasnet_token_t token, void *buf, size_t nbytes)
  {
    assert(buf != NULL);
    assert(nbytes == sizeof(alloc_am_t));
    alloc_am_t *am = (alloc_am_t *)buf;

#ifdef DEBUG      
    cerr << my_node << " is inside alloc_cpu_am_handler.\n";
#endif

    alloc_reply_t reply;
    reply.ptr_addr = am->ptr_addr; // pass back the ptr_addr from the scr node
#ifdef USE_GASNET_FAST_SEGMENT
    reply.ptr = gasnet_seg_alloc(am->nbytes);
#else
    reply.ptr = malloc(am->nbytes);
#endif

#ifdef DEBUG      
    cerr << my_node << " allocated " << am->nbytes << " memory at " << reply.ptr << "\n";
#endif

    assert(reply.ptr != NULL);
    reply.cb_event = am->cb_event;
    GASNET_SAFE(gasnet_AMReplyMedium0(token, ALLOC_REPLY, &reply, sizeof(reply)));
  }


  void alloc_reply_handler(gasnet_token_t token, void *buf, size_t nbytes)
  {
    assert(buf != NULL);
    assert(nbytes == sizeof(alloc_reply_t));
    alloc_reply_t *reply = (alloc_reply_t *)buf;

#ifdef DEBUG
    cerr << my_node << " is in alloc_reply_handler. reply->ptr " 
         << reply->ptr << "\n";
#endif

    *(reply->ptr_addr) = reply->ptr;
    reply->cb_event->decref();
  }      

  void free_cpu_am_handler(gasnet_token_t token, void *buf, size_t nbytes)
  {
    assert(buf != NULL);
    assert(nbytes == sizeof(free_am_t));
    free_am_t *am = (free_am_t *)buf;
    assert(am->ptr != NULL);
#ifdef USE_GASNET_FAST_SEGMENT
    gasnet_seg_free(am->ptr);
#else
    free(am->ptr);
#endif
  }

  void async_done_am_handler(gasnet_token_t token, void *buf, size_t nbytes)
  {
    async_done_am_t *am = (async_done_am_t *)buf;

    assert(nbytes == sizeof(async_done_am_t));

    if (am->ack_event != NULL) {
      am->ack_event->decref();
      // am->future->_rv = am->_rv;
    }
  }
    
  int progress()
  {
    async_task *task;

    gasnet_AMPoll(); // make progress in GASNet

    // Execute tasks in the async queue
    if (!queue_is_empty(async_task_queue)) {
      // dequeue an async task
      gasnet_hsl_lock(&async_lock);
      task = (async_task *)queue_dequeue(async_task_queue);
      gasnet_hsl_unlock(&async_lock);
      if (task != NULL) {
#ifdef DEBUG
        cerr << my_node << " is about to execute async task.\n";
        cerr << *task << endl;
#endif
        if (task->_callee != my_node.id()) {
          // remote async task
          // Send AM "there" to request async task execution 
          GASNET_SAFE(gasnet_AMRequestMedium0(task->_callee, ASYNC_AM,
                                              task, task->nbytes()));
        } else {
          // execute the async task
          if (task->_fp)
            (*task->_fp)(task->_args);

          if (task->_ack != NULL) {
            if (task->_caller == my_node.id()) {
              // local event acknowledgment
              task->_ack->decref(); // need to enqueue callback tasks
            } else {
              // send an ack message back to the caller of the async task
              async_done_am_t am;
              am.ack_event = task->_ack;
              GASNET_SAFE(gasnet_AMRequestMedium0(task->_caller,
                                                  ASYNC_DONE_AM,
                                                  &am,
                                                  sizeof(am)));
            }
          }
        }
        delete task;
      }
    } // if (!queue_is_empty(async_task_queue)) {

    return UPCXX_SUCCESS;
  } // progress()

  volatile int exit_signal = 0;
    
  void signal_exit_am()
  {
    exit_signal = 1;
  }
    
  void signal_exit()
  {
    // Only the master process should wait for incoming tasks
    assert(my_node.id() == 0);

    for (int i = 1; i < global_machine.node_count(); i++) {
      async(i)(signal_exit_am);
    }

    progress();

    wait(); // wait for the default event;
  }

  void wait_for_incoming_tasks()
  {
    // Only the worker processes should wait for incoming tasks
    assert(my_node.id() != 0);
      
    // Wait until the master process sends out an exit signal 
    while (!exit_signal) {
      progress();
    }
  }
   
  void print_async_queue(FILE *fp)
  {
    fprintf(fp, "Printing the async task queue:\n");
    if (async_task_queue == NULL) {
      fprintf(fp, "async_task_queue is empty.\n");
      return;
    }
    // async_task *task = get_next_task(async_task_queue);
    // print_task(task);
  }
} // namespace upcxx
