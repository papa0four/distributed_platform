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
 * opchain struct
 */
typedef struct _opchain_t
{
    uint32_t operation;
    uint32_t operand;
    uint32_t opchain_sz;
} opchain_t, * p_opchain;

/**
 * operand types
 */
typedef struct _operand_t
{
    uint32_t operand_num;
    uint32_t operand_list_sz;
} operand_t, * p_operand;

/**
 * struct info:
 */
typedef struct _computation_packet_t
{
    uint32_t    version;
    uint32_t    operation;
    opchain_t * op_chain;
    operand_t * operands;
} packet_t, * p_packet;

/**
 * @brief - a SIGINT handler to catch ctrl+c keyboard interrupt.
 *          should trigger graceful safe teardown
 * @param signo - an in value equal to a SIGINT keyboard interrupt '2'
 * @return - N/A
 */
static void sigint_handler (int signo);

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

#endif
