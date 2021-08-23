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
 * @param - N/A
 * @return - the packed struct containing the scheduler's information
 */
struct addrinfo setup_scheduler (struct addrinfo * hints, int socket_type);

/**
 * @brief - handles the initial setup of the socket in which the scheduler will
 *          continuously listen on for the workers and submitters
 * @param scheduler - the sockaddr_in struct containing the scheduler's info
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
 * @return - N/A
 */
void * handle_broadcast (void * p_port);

/**
 * @brief - handles the initial setup of the socket in which the scheduler will
 *          continuously listen on for both workers and submitters
 * @param scheduler - the schedulers address information for socket creation
 * @return - the socket's file descriptor or -1 on error
 */
int handle_working_socket (struct addrinfo * hints, char * p_port);

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

#endif