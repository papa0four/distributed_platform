#ifndef __GLOBAL_DATA_H__
#define __GLOBAL_DATA_H__

#include <stdbool.h>
#include <pthread.h>
#include <arpa/inet.h>
#include "work_queue.h"

#define MAX_CLIENTS 100

/**
 * @brief - declaration of global variables
 * @var g_running - global, exportable bool to trigger state of driver
 * @var pp_jobs - an array of pointers to job structures
 * @var p_wqueue - a queue container to store the work required for each job
 * @var job_list_len - the length of the jobs array
 * @var jobs_list_mutex - a mutex type for adding jobs to the job list
 * @var running_mutex - mutex to lock/unlock the status of g_running
 * @var wqueue_mutex - a mutex type for adding work to the work queue
 * @var condition - a signaling condition to alert working/waiting threads
 */
extern job_t ** pp_jobs;

extern volatile size_t jobs_list_len;

extern work_queue_t * p_wqueue;

extern volatile bool g_running;

extern pthread_mutex_t running_mutex;

extern pthread_mutex_t wqueue_mutex;

extern pthread_mutex_t jobs_list_mutex;

extern pthread_cond_t condition;

extern int num_clients;

extern pthread_t worker_threads[MAX_CLIENTS];

/**
 * @brief - initializes all global variables to include:
 *              global running flag --> bool type
 *              initializes pthread_cond
 *              initializes pthread_mutex on queue mutex
 *              initializes pthread_mutex on running mutex
 *              initializes pthread_mutex on jobs list mutex
 *              creates work queue
 *              allocates the jobs list
 * @param - N/A
 * @return - 0 on successful initialization and -1 on error
 */
int initialize_global_data ();

/**
 * @brief - receives a job and populates the global jobs list before taking
 *          each jobs work groups and populates the work queue and assigns the
 *          job id to keep track of all work associated with each job
 * @param p_job - a pointer to the job struct containing the instructions for
 *                the work to be performed
 * @return - returns the job id (index of the jobs list) upon successful addition
 *           to the jobs list/work queue or -1 on error
 */
int populate_jobs_and_queue (job_t * p_job);

/**
 * @brief - iterates over the global list of jobs to find specific job
 *          based upon the job id (index of the jobs list)
 * @param job_id - the id of the specific job to find
 * @return - returns a pointer to the specific job found within the jobs list
 *           or returns NULL on error or if not found
 */
job_t * find_job (uint32_t job_id);

/**
 * @brief - packs job structure with the answer and sets the done flag
 * @param worker_conn - the file descriptor of the worker's connection
 * @param answer - the answer calculated answer of the work passed
 * @return - N/A
 */
void recv_answer (int worker_conn, int32_t answer);

/**
 * @brief - checks to see if job within jobs list is completed. Upon successful
 *          completion check, will print the Job and its answer(s) to the terminal
 * @param p_job - a pointer to the specific job
 * @return - true if successfil, false if not found or on error
 */
bool jobs_done (job_t * p_job);

/**
 * @brief - responsible for placing a job's work within the queue to hand off
 *          to each worker
 * @param p_job - a pointer to the specific job received from the submitter to
 *                enqueue
 * @return - N/A
 */
void populate_wqueue (job_t * p_job);

#endif
