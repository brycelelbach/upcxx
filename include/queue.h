/* queue.h -- implementation of a general doubly-linked queue (list)
 *
 * Note: this queue implementation is not thread-safe so queue
 * operations need to be protected by locks (mutexes) in a
 * multi-threaded environment.
 *
 */

#ifndef QUEUE_H_
#define QUEUE_H_

/// \cond SHOW_INTERNAL

extern "C"
{
#include <stdlib.h>
#include <assert.h>

  typedef struct qnode {
    void *data;
    struct qnode *next;
    struct qnode *prev;
  } qnode_t;

  typedef struct {
    qnode_t *head;
    qnode_t *tail;
  } queue_t;

  static inline qnode_t * qnode_new()
  {
    qnode_t *qnode;
    qnode = (qnode_t *)malloc(sizeof(qnode_t));
    assert(qnode != NULL);
    return qnode;
  }

  static inline void qnode_free(qnode_t *q)
  {
    free(q);
  }

  static inline queue_t * queue_new()
  {
    queue_t *q;
    q =  (queue_t *)malloc(sizeof(queue_t));
    assert(q != NULL);
    q->head = NULL;
    q->tail = NULL;
    return q;
  }

  static inline void queue_free(queue_t *q)
  {
    free(q);
  }

  static inline void qnode_remove(qnode_t *qnode)
  {
    qnode_t *mynext, *myprev;

    if (qnode == NULL)
      return;

    mynext = qnode->next;
    myprev = qnode->prev;

    if (mynext != NULL)
      mynext->prev = myprev;

    if (myprev != NULL)
      myprev->next = mynext;
  }

  static inline int qnode_insert_to_next(qnode_t *qnode, qnode_t *newnode)
  {
    qnode_t *mynext;

    if (qnode == NULL || newnode == NULL)
      return 1; /* fail */

    mynext = qnode->next;
    if (mynext != NULL)
      mynext->prev = newnode;
    qnode->next = newnode;
    newnode->next = mynext;
    newnode->prev = qnode;

    return 0; /* success */
  }

  static inline int qnode_insert_to_prev(qnode_t *qnode, qnode_t *newnode)
  {
    qnode_t *myprev;

    if (qnode == NULL || newnode == NULL)
      return 1; /* fail */

    myprev = qnode->prev;
    if (myprev != NULL)
      myprev->next = newnode;
    qnode->prev = newnode;
    newnode->next = qnode;
    newnode->prev = myprev;

    return 0; /* success */
  }

  static inline void queue_insert_head(queue_t *q, void *data)
  {
    qnode_t *newnode;

    newnode = qnode_new();
    newnode->data = data;

    if (q->head == NULL) {
      /* first queue node */
      q->head = newnode;
      q->tail = newnode;
      newnode->next = NULL;
      newnode->prev = NULL;
    } else {
      qnode_insert_to_prev(q->head, newnode);
      q->head = newnode;
    }
  }

  static inline void queue_insert_tail(queue_t *q, void *data)
  {
    qnode_t *newnode;

    newnode = qnode_new();
    newnode->data = data;

    if (q->tail == NULL) {
      /* first queue node */
      q->head = newnode;
      q->tail = newnode;
      newnode->next = NULL;
      newnode->prev = NULL;
    } else {
      qnode_insert_to_next(q->tail, newnode);
      q->tail = newnode;
    }
  }

  static inline void *queue_remove_head(queue_t *q)
  {
    qnode_t *head = q->head;
    void *data;

    if (head == NULL)
      return NULL;

    if (head == q->tail) {
      /* last queue node */
      q->head = NULL;
      q->tail = NULL;
    } else {
      q->head = head->next;
    }

    data = head->data;
    qnode_remove(head); /* remove the head node from the queue */
    qnode_free(head); /* free the storage of the head node */

    return data;
  }

  static inline void *queue_remove_tail(queue_t *q)
  {
    qnode_t *tail = q->tail;
    void *data;

    if (tail == NULL)
      return NULL;

    if (tail == q->head) {
      /* last queue node */
      q->tail = NULL;
      q->head = NULL;
    } else {
      q->tail = tail->prev;
    }
  
    data = tail->data;
    qnode_remove(tail);
    qnode_free(tail);

    return data;
  }

  static inline int queue_is_empty(queue_t *q)
  {
    assert(q != NULL);
    return (q->head == NULL);
  }

#define queue_enqueue queue_insert_head
#define queue_dequeue queue_remove_tail
#define queue_steal queue_remove_head

} // end of extern "C"


/// \endcond

#endif /* QUEUE_H_ */
