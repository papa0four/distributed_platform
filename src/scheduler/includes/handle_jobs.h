#ifndef __HANDLE_JOBS_H__
#define __HANDLE_JOBS_H__

#include "scheduler.h"

/**
 * @brief - stores all necessary data for the work to be passed to the worker
 * @member job_id - the associated job for specific work
 * @member iterations - number of times to perform work (always 1)
 * @member item - the operand of which to perform the work on
 * @member answer - the answer computed by the worker
 * @member worker_sock - the file descriptor of the worker connection
 * @member b_work_done - a true/false switch determining if work is finished
 * @member p_chain - the chain of operations to perform on the item
 */
typedef struct _work_t
{
    uint32_t    job_id;
    uint32_t    iterations;
    uint32_t    item;
    int32_t     answer;
    int         worker_sock;
    bool        b_work_done;
    opchain_t * p_chain;
} work_t;

/**
 * @brief - stores all necessary data for the work to be passed to the worker
 * @member job_id - the associated job for a specific submission
 * @member p_work - the specified item and operations to perform
 * @member b_job_done - a true/false switch determining if overarching job is done
 * @member answer - the answer computed by the worker
 * @member num_items - the number of operands in which the work with be operated on
 * @member num_operations - the number of operations to perform on a single item
 */
typedef struct _job_t
{
    uint32_t    job_id;
    work_t    * p_work;
    bool        b_job_done;
    uint32_t    num_items;
    uint32_t    num_operations;
} job_t;

/**
 * @brief - takes a pointer to the submit job payload struct in order to
 *          appropriately populate the job struct with the submitted data
 * @param p_work - a pointer to the submit job payload struct containing
 *                 all work data passed to the scheduler from the submitter
 * @return - a pointer to a job struct containing all work data appropriately
 *           packed or NULL on error
 */
job_t * create_job (subjob_payload_t * p_work);

/**
 * @brief iterates over the jobs list and appropriately free's the list of work
 *        and the associated job
 * @param - N/A
 * @return - N/A
 */
void destroy_jobs ();



#endif