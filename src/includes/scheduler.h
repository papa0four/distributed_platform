#ifndef __SCHEDULER_H__
#define __SCHEDULER_H__

#include <stdio.h>
#include <ctype.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <signal.h>
#include <pthread.h>
#include <inttypes.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/un.h>
#include <arpa/inet.h>
#include <errno.h>
#include "./operations.h"
#include "./work_queue.h"

#define SIGINT          2
#define BACKLOG         3
#define TV_TIMEOUT      5
#define BASE_10         10
#define MAX_CLIENTS     100
#define MAX_BUFF        1024
#define BROADCAST_PORT  31337
#define MIN_PORT        31338
#define MAX_PORT        65535

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
volatile bool      g_running;
job_t           ** pp_jobs          = NULL;        
work_queue_t     * p_wqueue         = NULL;
volatile size_t    jobs_list_len    = 0;
pthread_mutex_t    jobs_list_mutex;
pthread_mutex_t    running_mutex;
pthread_mutex_t    wqueue_mutex;
pthread_cond_t     condition;

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
header_t * unpack_header (int client_conn);

/**
 * @brief - calls all receipt helper functions in order to appropriately retrieve
 *          all data sent from the submitter before calling the pack the submit
 *          job payload struct
 * @param client_conn - the file descriptor to the client's connection
 * @return - a pointer to the submit job payload struct containing all data
 *           received from the submitter or NULL on error
 */
subjob_payload_t * unpack_payload (int client_conn);

/**
 * @brief - calls unpack_header helper function to recieve the initial packet
 *          header. Validation on actual header data is done within the helper
 *          function, however, validation that the header was properly populated
 *          is done here before clearing header information
 * @param client_conn - the file descriptor of the client's connection
 * @return - N/A
 */
void handle_submitter_req (int client_conn);

/**
 * @brief - handle command line argument to receive a valid working
 *          port in which the scheduler will pass to the submitter
 *          to allow for communication and job completion
 * @param argc - number of command line arguments passed at runtime
 * @param argv - pointer to the array of character passed as command line
 *               arguments, specified and checked to ensure the positional
 *               argument is a number within the valid range
 * @return - the specified port number passed as a command line argument
 */
uint16_t get_port(int argc, char ** argv);

/**
 * @brief - handle to infinite broadcast send/receive between the
 *          submitter/worker and the scheduler. This will always listen
 *          for the incoming broadcast message based upon a global
 *          running variable that is only triggered by the shutdown flag
 *          sent by the submitter.
 * @param p_port - a pointer to the port parameter generated in order
 *                 to create the working socket
 * @return - N/A
 */
void * handle_broadcast (void * p_port);

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
 * @brief - receives a job and populates the global jobs list before taking
 *          each jobs work groups and populates the work queue and assigns the
 *          job id to keep track of all work associated with each job
 * @param p_job - a pointer to the job struct containing the instructions for
 *                the work to be performed
 * @return - returns the job id (index of the jobs list) upon successful addition
 *           to the jobs list/work queue or -1 on error
 */
ssize_t populate_jobs_and_queue (job_t * p_job);

/**
 * @brief - receive answer to each tasked work from the worker
 * @param worker_conn - the file descriptor of the worker's connection
 * @param answer - the answer calculated answer of the work passed
 * @return - N/A
 */
void recv_answer (int worker_conn, int32_t answer);

/**
 * @brief - checks to see if job within jobs list is completed. Upon successful
 *          completion check, will print the Jo
 */
bool jobs_done (job_t * p_job);

/**
 * find job
 */
job_t * find_job (uint32_t job_id);

/**
 * @brief iterates over the jobs list and appropriately free's the list of work
 *        and the associated job
 * @param - N/A
 * @return - N/A
 */
void destroy_job ();

/**
 * @brief - used as a helper function in order to free and NULL'ify memory
 *          allocated
 * @param memory_obj - a pointer to the allocated memory object in order to
 *                     appropriate clean memory
 * @return - N/A
 */
void clean_memory (void * memory_obj);

#endif
