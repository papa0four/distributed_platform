
#define _GNU_SOURCE
#ifndef __SCHEDULER_H__
#define __SCHEDULER_H__

#include <stdio.h>
#include <ctype.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <limits.h>
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
#include "operations.h"
#include "work_queue.h"
#include "global_data.h"
#include "handle_jobs.h"

// predefined macros to avoid magic number usage
#define SIGINT          2
#define PORT_LEN        6
#define BACKLOG         10
#define MAX_BUFF        1024
#define BROADCAST       31337
#define BROADCAST_PORT  "31337"
#define DEFAULT_PORT    "4337"
#define MIN_PORT        1024
#define MAX_PORT        65535

// clean up macro
#define CLEAN(a) if(a)free(a);(a)=NULL

/**
 * @brief - a SIGINT handler to catch ctrl+c keyboard interrupt.
 *          should trigger graceful safe teardown
 * @param signo - an in value equal to a SIGINT keyboard interrupt '2'
 * @return - N/A
 */
void sigint_handler (int signo);

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
char * get_port(int argc, char ** argv);

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
 * @brief - a helper function to handle the workers connecting to the scheduler
 *          and create a thread for each
 * @param scheduler_fd - the socket file descriptor created for the scheduler
 * @param scheduler - the scheduler's address info
 * @param scheduler_len - the size of the scheduler struct for socket communications
 * @return - N/A
 */
void handle_worker_connections (int scheduler_fd, struct addrinfo * hints,
                                socklen_t scheduler_len);

/**
 * @brief - takes data sent from submitter and unpacks the header info,
 *          checks the version and operation values for validity and
 *          stores values into their appropriate places within the 
 *          header struct
 * @param client_conn - the file descriptor to the client's connection
 * @return - a pointer to the header struct containing valid info or NULL
 *           on error
 */
uint32_t unpack_header (int client_conn);

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
 * @brief - appropriately dequeue's work from the work queue, assigns the worker's
 *          socket file descriptor, packs the job packet, and sends the data to the
 *          worker
 * @param worker_fd - the socket file descriptor of the worker assigned to the task
 * @return - N/A
 */
void send_task_to_worker (int worker_fd);

/**
 * @brief - receives the SUBMIT_JOB packet from the worker once a task is complete
 *          and passes the answer to the recv_answer function
 * @param worker_fd - the socket file descriptor for the worker submitting the answer
 * @return - N/A
 */
void recv_computation (int worker_conn);

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
