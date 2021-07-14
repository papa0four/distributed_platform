#ifndef __WORK_QUEUE_H__
#define __WORK_QUEUE_H__

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include "operations.h"

#define QUEUE_CAPACITY 100

/**
 * @brief - declaration of global variables
 * @var p_wqueue - a queue container to store the work required for each job
 * @var job_list_len - the length of the jobs array
 * @var jobs_list_mutex - a mutex type for adding jobs to the job list
 * @var running_mutex - mutex to lock/unlock the status of g_running
 * @var wqueue_mutex - a mutex type for adding work to the work queue
 * @var condition - a signaling condition to alert working/waiting threads
 */
extern volatile bool      g_running;
extern volatile size_t    jobs_list_len;
extern pthread_mutex_t    jobs_list_mutex;
extern pthread_mutex_t    running_mutex;
extern pthread_mutex_t    wqueue_mutex;
extern pthread_cond_t     condition;

/**
 * @brief - a structure container for the queue to store work
 * @member p_nodes - an array of queue nodes containing work
 * @member head - a pointer to the head of the queue structure
 * @member q_size - a number to indicate the current queue size
 * @member capacity - the overall size of the queue
 *                  NOTE: can be adjusted to accept more work
 */
typedef struct _work_queue_t
{
    work_t       ** p_work;
    size_t          head;
    size_t          tail;
    size_t          q_size;
    size_t          capacity;
} work_queue_t;

/**
 * @brief - initial the overarching queue structure
 *          this structure can be resized to accomodate
 *          large amounts of work
 * @param - N/A
 * @return - a pointer to the newly created queue structure
 */
work_queue_t * wqueue_init ();

/**
 * @brief - used to add a new set of work instructions to the queue
 * @param p_tqueue - a pointer to the queue structure
 * @param p_work - a pointer to the work structure containing all instructions
 * @return - true upon successful addition of work, false on errors
 */
bool enqueue_work (work_queue_t * p_wqueue, work_t * p_work);

/**
 * @brief - pop's a work instruction off the top of the queue,
 *          free'ing the node structure leaving only the work struct
 * @param p_tqueue - a pointer to the overarching queue structure
 * @return - returns the work structure to be passed to the worker
 *           and free's the node structure, return's NULL on error
 */
work_t * dequeue_work (work_queue_t * p_wqueue);

/**
 * @brief - checks to see if the current queue is full
 * @param p_tqueue - a pointer to the overarching queue structure
 * @return - 1 if current queue->size == capacity, 0 if not 
 *           and -1 on error
 */
ssize_t is_full (work_queue_t * p_wqueue);

/**
 * @brief - checks to see if current queue is empty
 * @param p_tqueue - a pointer to the overarching queue structure
 * @return - 1 if current queue->size == 0, 0 if not
 *           and -1 on error
 */
ssize_t is_empty (work_queue_t * p_wqueue);

/**
 * @brief - checks the current length of the work queue
 * @param p_tqueue - a pointer to the overarching queue structure
 * @return - the value stored a p_tqueue->q_size, -1 on error
 */
ssize_t wqueue_len (work_queue_t * p_wqueue);

/**
 * @brief - helper function: used to dynamically resize the overall
 *          size of the queue if the current capacity is reached
 * @param p_tqueue - a pointer to the overarching queue structure
 * @return - a pointer to the newly resized queue container, NULL
 *           on error
 */
work_queue_t * resize_wqueue (work_queue_t * p_wqueue);

/**
 * @brief - eliminates current queue, free'ing all nodes and the
 *          current container regardless if empty or not
 * @param p_tqueue - a pointer to the overarching queue structure
 * @return - true upon proper tear down of the queue, false on error
 */
bool wqueue_destroy (work_queue_t * p_wqueue);

#endif
