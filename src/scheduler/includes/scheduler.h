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
#include "operations.h"
#include "work_queue.h"
#include "global_data.h"
#include "handle_jobs.h"

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
 * 
 */
void handle_worker_connections (int scheduler_fd, struct sockaddr_in scheduler,
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
 * retrieve work
 */
void send_task_to_worker (int worker_fd);

/**
 * 
 */
void recv_computation (int worker_conn);

/**
 * @brief - a helper function to print the opchain to the scheduler
 * @param p_chain - a pointer to the operations chain to print
 * @return - N/A
 */
void print_opchain (opchain_t * p_chain);

/**
 * @brief - used as a helper function in order to free and NULL'ify memory
 *          allocated
 * @param memory_obj - a pointer to the allocated memory object in order to
 *                     appropriate clean memory
 * @return - N/A
 */
void clean_memory (void * memory_obj);

/**
 * function passed to the worker thread
 */
void * worker_func (void * worker_conn);

#endif
