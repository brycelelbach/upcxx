/*
 * machine_gasnet.cpp
 *
 */

#include <cassert>

#include "machine.h"

// #define DEBUG

namespace upcxx
{
    /* Global abstraction machine for job execution */
    machine global_machine;

    /* The node (process) on which the current thread in running */
    node my_node;

    processor my_processor;

    ///
    /// node member functions
    ///
    node::node(int id, int processor_count)
    {
      _id = id;
      _processor_count = processor_count;
    }

    processor node::get_processor(int i) const
    {
      assert(i >= 0 && i < i < _processor_count);
      return processor(this->id(), i);
    }

    bool node::islocal()
    {
      return (_id == my_node.id());
    }

    ///
    /// machine member functions
    ///
    void machine::init(int node_count, node *nodes, int my_node_id)
    {
      assert(node_count > 0);
      assert(nodes != NULL);
      assert(my_node_id >= 0 && my_node_id < node_count);

#ifdef DEBUG
      fprintf(stderr, "machine::init node_count %d, my_node_id %d\n",
              node_count, my_node_id);
#endif

      _node_count = node_count;
      _nodes = new node [node_count];
      assert(_nodes != NULL);
      memcpy(_nodes, nodes, sizeof(node) * node_count);

      _cpu_count_prefix_sums = (int *)malloc(_node_count * sizeof(int));
      assert(_cpu_count_prefix_sums != NULL);


      // exclusive prefix sum
      _cpu_count_prefix_sums[0] = 0;
      for (int i = 1; i < _node_count; i++) {
        _cpu_count_prefix_sums[i] = _cpu_count_prefix_sums[i-1] + _nodes[i-1].processor_count();
      }
      _processor_count = _cpu_count_prefix_sums[_node_count - 1]
        + _nodes[_node_count - 1].processor_count();
    }

    machine::~machine()
    {
      if (_nodes != NULL) {
        assert(_node_count > 0);
        delete [] _nodes;
      }

      if (_cpu_count_prefix_sums != NULL) {
        free(_cpu_count_prefix_sums);
      }
    }

    int machine::cpu_id_local2global(const node &n, int local_id)
    {
      return n.get_processor(local_id).id();
    }
} // namespace upcxx
