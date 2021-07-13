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

#define BROADCAST_PORT  31337
#define MIN_PORT        31338
#define MAX_PORT        65535
#define MAX_BUFF        1024
#define MAX_CLIENTS     100
#define BASE_10         10
#define TV_TIMEOUT      5
#define BACKLOG         3
#define SIGINT          2

/**
 * @brief - a SIGINT handler to catch ctrl+c keyboard interrupt.
 *          should trigger graceful safe teardown
 * @param signo - an in value equal to a SIGINT keyboard interrupt '2'
 * @return - N/A
 */
static void sigint_handler (int signo);

/**
 * function info here
 */
static header_t * unpack_header (int client_conn);

/**
 * recv num ops
 */
static uint32_t recv_num_operations (int client_conn);

/**
 * handle opchain
 */
static opchain_t * recv_opchain (int client_conn, uint32_t num_ops);

/**
 * recv iterations
 */
static uint32_t recv_iterations (int client_conn);

/**
 * recv number of items
 */
static uint32_t recv_num_items (int client_conn);

/**
 * handle items reciept
 */
static item_t * recv_items (int client_conn, uint32_t num_items);

/**
 * pack stuffs
 */
static subjob_payload_t * pack_payload_struct (uint32_t num_operations, opchain_t * p_ops,
                uint32_t num_iters, uint32_t num_items, item_t * p_items);

/**
 * unpack payload
 */
static subjob_payload_t * unpack_payload (int client_conn);

/**
 * handle conns
 */
static void handle_submitter_req (void * p_client_socket);

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
static uint16_t get_port(int argc, char ** argv);

/**
 * setup scheduler
 */
static struct sockaddr_in setup_scheduler ();

/**
 * setup socket stuffs
 */
static int create_broadcast_socket (struct sockaddr_in scheduler);

/**
 * @brief - handle to infinite broadcast send receive betweem the
 *          submitter/worker and the scheduler. This will always listen
 *          for the incoming broadcast message based upon a global
 *          running variable that is only triggered by the shutdown flag
 *          sent by the submitter.
 * @param p_port - a pointer to the port parameter generated in order
 *                 to create the working socket
 * @return - N/A
 */
static void * handle_broadcast (void * p_port);

/**
 * clean memory stuffs
 */
static void clean_memory (void * memory_obj);
#endif
