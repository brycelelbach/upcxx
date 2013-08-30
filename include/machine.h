/**
 * Multi-node machine abstraction
 *
 * Two level logical machine hierarchy:
 * A machine is composed of one or more nodes.
 * A node is composed of one or more processors.
 * Each node is mapped to an OS process in the current implementation.
 *
 */

#pragma once

#include <iostream>
#include <assert.h>

#include "gasnet_api.h"
#include "active_coll.h"
#include "range.h"

namespace upcxx
{
  // forward type declarations;
  struct machine;
  struct node;
  struct processor;

  struct node
  {
    node(int id, int processor_count = 1);
    node() : _id(gasnet_mynode()), _processor_count(1) { }

    // node(const node &node);
    // ~node();

    inline int id() const { return _id; }

    inline int node_id() const { return _id; }

    inline uint32_t count() { return 1; }

    inline int processor_count() const { return _processor_count; }

    processor get_processor(int i) const;

    bool islocal();

    // Data
    int _id;
    int _processor_count;
    // processor *_processors; // store the information of processors
    // machine *_machine; // the machine that this node belongs to
  }; // struct node

  struct machine
  {
    machine()
    {
      _node_count = 0;
      _nodes = NULL;
      _processor_count = 0;
      _my_node_id = 0;
      _cpu_count_prefix_sums = NULL;
      // need to be initialized with the init() member function
    }

    ~machine();

    void init(int node_count, node *nodes, int my_node_id);

    inline int
    node_count() const { return _node_count; }

    inline node &
    get_node(int i) const
    {
      assert(i >= 0 && i < _node_count);
      assert(_nodes != NULL);
      return _nodes[i];
    }

    inline upcxx::range
    get_nodes(int sz) const
    {
      int start = _node_count - sz;
      int end = _node_count;
      if (start < 0) start = 0;
      return upcxx::range(start, end);
    }

    inline uint32_t count() { return 1; }

    inline int
    processor_count() const { return _processor_count; }

    inline int
    my_node_id() const { return _my_node_id; }

    inline node &
    my_node() const { return _nodes[_my_node_id]; }

    /* return the unique global id of the cpu */
    inline int
    cpu_id_local2global(int node_id, int local_id)
    {
      assert(_cpu_count_prefix_sums != NULL);
      assert(node_id >=0 && node_id < _node_count);

      return _cpu_count_prefix_sums[node_id] + local_id;
    }

    int cpu_id_local2global(const node &node, int local_id);

    /**
     * \param[in] global_id the global cpu id
     * \param[out] node_id the node on which the cpu is located
     * \param[out] local_id the local device id of the cpu
     */
    inline void
    cpu_id_global2local(int global_id, int &node_id, int &local_id)
    {
      int i;

      assert(_cpu_count_prefix_sums != NULL);

      // compute _node_id and _local_id from global_cpu_id;
      for (i = 0; i < _node_count - 1; i++) {
        if (local_id < _cpu_count_prefix_sums[i + 1]) {
          node_id = i;
          local_id = i - _cpu_count_prefix_sums[i];
          // cerr << "cpu(" << _node_id << "," << _local_id << ")\n";
          return;
        }
      }
      node_id = i;
      local_id = i - _cpu_count_prefix_sums[i];
      // cerr << "cpu(" << _node_id << "," << _local_id << ")\n";
    }

    // Data
    int _node_count; // the total number of nodes (processes) in the GASNet domain
    node *_nodes;
    int _my_node_id;
    int _processor_count; // the total number of processors in the machine

    /*
     * cumulative sum of cpu places for each node, which is the
     * prefix sum of gpu_places_counts
     */
    int *_cpu_count_prefix_sums;
  }; // close of machine

  /**
   * \ingroup initgroup
   *
   * \brief the aggregate of all nodes in the current job
   */
  extern machine global_machine;
  
  /**
   * \ingroup initgroup
   *
   * \brief the virtual node (process) information of the execution unit
   */
  extern node my_node;

  /**
   * Data structure that is common to all processor types (e.g., CPU and GPU)
   */
  struct processor
  {
    processor() : _id(my_node.id()), _node_id(my_node.id()), _local_id(0)
    { }

    processor(const processor &p)
    {
      _id = p._id;
      _node_id = p._node_id;
      _local_id = p._local_id;
    }


    processor(int node_id, int local_id)
    {
      _node_id = node_id;
      _local_id = local_id;
      _id = global_machine.cpu_id_local2global(node_id, local_id);
    }

    processor(int global_id)
    {
      _id = global_id;
      global_machine.cpu_id_global2local(global_id, _node_id, _local_id);
    }

    inline int id() const { return _id; }

    inline int node_id() const { return _node_id; }

    inline int local_id() const { return _local_id; }

    inline bool operator==(const processor &p) const
    {
      return (_id == p._id && _node_id == p._node_id
              && _local_id == p._local_id);
    }

    inline bool operator!=(const processor &p) const
    {
      return !(*this == p);
    }

    inline bool islocal()
    {
      return (_node_id == global_machine.my_node_id());
    }

    inline uint32_t count() { return 1; }

    // Data
    int _id; // unique global id
    int _node_id; // node id on which the processor is located
    int _local_id; // local id within the node

  }; // struct processor

  extern processor my_processor;

  // Output stream support functions for machine types
  inline std::ostream& operator<<(std::ostream& out, const machine& p)
  {
    out << "(machine: " << p.node_count() << " nodes)";
    return out;
  }

  inline std::ostream& operator<<(std::ostream& out, const node& p)
  {
    return out << "(node: " << p.id() << ")";
  }

  inline std::ostream& operator<<(std::ostream& out, const processor& p)
  {
    return out << "(processor: " << p.id() << ")";
  }
} // namespace upcxx
