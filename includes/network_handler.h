#define _GNU_SOURCE
#ifndef __NETWORK_HANDLER_H__
#define __NETWORK_HANDLER_H__

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/poll.h>
#include <sys/time.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/un.h>
#include <arpa/inet.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>

#include "operations.h"
#include "global_data.h"

// predefined macros to avoid magic number usage
#define UDP_CONN        0
#define TCP_CONN        1
#define SIGINT          2
#define PORT_LEN        6
#define BACKLOG         100
#define MAX_BUFF        1024
#define BROADCAST       31337
#define BROADCAST_PORT  "31337"
#define DEFAULT_PORT    "4337"
#define MIN_PORT        1024
#define MAX_PORT        65535

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
 * @brief - appropriately sets up the scheduler's information in order to 
 *          initiate conversation with all workers and submitters
 * @param hints - the addrinfo struct to setup the scheduler's network information
 * @param socket_type - the flag to determine SOCK_DGRAM or SOCK_STREAM
 * @return - the packed struct containing the scheduler's information
 */
struct addrinfo setup_scheduler (struct addrinfo * hints, int socket_type);

/**
 * @brief - handles the initial setup of the socket in which the scheduler will
 *          continuously listen on for the workers and submitters
 * @param hints - the addrinfo struct containing the scheduler's socket information
 * @param str_port - a string form of the TCP port for the working transactions
 * @param sockopt_flag - a variable containing the flags to pass to setsockopt (example SO_REUSEADDR, SO_BROADCAST)
 * @param type_flag - meant to be used to determine if socket is meant for UDP or TCP connections
 * @return - the broadcast file descriptor listening for submitters and workers
 *           or -1 on error
 */
int create_socket (struct addrinfo * hints, char * str_port, int sockopt_flag, int type_flag);

/**
 * @brief - handle to infinite broadcast send/receive between the
 *          submitter/worker and the scheduler. This will always listen
 *          for the incoming broadcast message based upon a global
 *          running variable that is only triggered by the shutdown flag
 *          sent by the submitter.
 * @param p_port - a pointer to the port parameter generated in order
 *                 to create the working socket
 * @return - NULL on success or failure (meant to be the broadcast thread function)
 */
void * handle_broadcast (void * p_port);

/**
 * @brief - handles the initial setup of the socket in which the scheduler will
 *          continuously listen on for both workers and submitters
 * @param hints - the schedulers address information for socket creation
 * @param p_port - a string version of the port meant for the TCP communications stream
 * @return - the socket's file descriptor or -1 on error
 */
int handle_working_socket (struct addrinfo * hints, char * p_port);

/**
 * 
 */
void add_to_threadlist (thread_info_t ** p_threads, int idx, int new_connection);

/**
 * @brief - a helper function to handle the workers connecting to the scheduler
 *          and create a thread for each
 * @param scheduler_fd - the socket file descriptor created for the scheduler
 * @param hints - the addrinfo struct containing the scheduler's address info
 * @param scheduler_len - the size of the scheduler struct for socket communications
 * @return - an array of thread_info_t pointers containing thread specific data
 */
thread_info_t ** handle_worker_connections (int scheduler_fd, struct addrinfo * hints,
                                socklen_t scheduler_len);

#endif