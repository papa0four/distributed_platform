#ifndef __SCHEDULER_H__
#define __SCHEDULER_H__

#define _GNU_SOURCE

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <limits.h>
#include <signal.h>
#include <pthread.h>
#include <inttypes.h>
#include <errno.h>

#include "operations.h"
#include "work_queue.h"
#include "global_data.h"
#include "handle_jobs.h"
#include "network_handler.h"
#include "submit_job.h"
#include "query_work.h"

/**
 * @brief - a SIGINT handler to catch ctrl+c keyboard interrupt.
 *          should trigger graceful safe teardown
 * @param signo - an in value equal to a SIGINT keyboard interrupt '2'
 * @return - N/A
 */
void sigint_handler (int signo);

/**
 * @brief - takes data sent from submitter and unpacks the header info,
 *          checks the version and operation values for validity and
 *          stores values into their appropriate places within the 
 *          header struct
 * @param client_conn - the file descriptor to the client's connection
 * @return - a pointer to the header struct containing valid info or NULL
 *           on error
 */
uint32_t unpack_header (thread_info_t * p_info);

/**
 * @brief - receives the SUBMIT_JOB packet from the worker once a task is complete
 *          and passes the answer to the recv_answer function
 * @param worker_fd - the socket file descriptor for the worker submitting the answer
 * @return - N/A
 */
void recv_computation (int worker_conn);

/**
 * 
 */
ssize_t determine_operation (thread_info_t * p_info);

/**
 * @brief - the main worker thread function that is passed upon worker/submitter
 *          thread creation. worker_func then calls the determine operation function
 *          that handles each packet received by the scheduler appropriately
 * @param worker_conn - the socket file descriptor for the connection to the scheduler
 * @return - returns NULL on success and error, used for pthread_create, but errors 
 *           are handled within appropriately
 */
void * worker_func (void * worker_conn);

#endif
