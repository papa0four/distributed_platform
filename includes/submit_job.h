#define _GNU_SOURCE
#ifndef __SUBMIT_JOB_H__
#define __SUBMIT_JOB_H__

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
#include <errno.h>
#include "operations.h"

/**
 * @brief - reads the value of num_operations within the packet sent from
 *          submitter and stores it within the submit job payload struct
 * @param client_conn - the file descriptor to the client's connection
 * @return - an appropriately converted 32-bit unsigned int containing the
 *           length of the opchain, 0 on error or empty opchain
 */
uint32_t recv_num_operations (int client_conn);

/**
 * @brief - receives the chain of operations from the packet sent from the
 *          submitter and stores it within the submit job payload struct
 * @param client_conn - the file descriptor to the client's connection
 * @param num_ops - the total length of the opchain
 * @return - the list of operations where each index stores a pair of 
 *           operations to perform on each item or NULL on error
 */
opchain_t * recv_opchain (int client_conn, uint32_t num_ops);

/**
 * @brief - receives the number of work iterations to be performed on each item
 *          NOTE: this value may only be 1, anything else is invalid
 * @param client_conn - the file descriptor to the client's connection
 * @return - an appropriately converted 32-bit unsigned int containing the 
 *           number of iterations in which to perform the work on each item
 *           or returns 0 on error
 */
uint32_t recv_iterations (int client_conn);

/**
 * @brief - receives the total length of the items list from the packet
 *          submitted and appropriately stores the data within the 
 *          submit job payload struct
 * @param client_conn - the file descriptor to the client's connection
 * @return - an appropriately converted 32-bit unsigned int containing the
 *           total length of the items list or 0 on error and empty lists
 */
uint32_t recv_num_items (int client_conn);

/**
 * @brief - iterates over the list of items within the packet submitted and
 *          stores them within the items list in the submit job payload struct
 * @param client_conn - the file descriptor to the client's connection
 * @param num_items - the total length of the items list
 * @return - a pointer to the items list or NULL on error
 */
item_t * recv_items (int client_conn, uint32_t num_items);

/**
 * @brief - received all appropriate data in order to obtain all submit
 *          job payload data and copies each piece of data into it's appropriate
 *          location within the submit job payload struct
 * @param num_operations - the total length of the chain of operations
 * @param p_ops - a pointer to the chain of operations list
 * @param num_iters - the number of iterations to perform work on each item
 * @param num_items - the total length of the items list
 * @param p_items - the list of items to perform each operation on
 * @return - a pointer to the submit job payload struct containing all
 *           appropriate data received from the submitter or NULL on error
 */
subjob_payload_t * pack_payload_struct (uint32_t num_operations, opchain_t * p_ops,
                uint32_t num_iters, uint32_t num_items, item_t * p_items);

/**
 * @brief - calls all receipt helper functions in order to appropriately retrieve
 *          all data sent from the submitter before calling the pack the submit
 *          job payload struct
 * @param client_conn - the file descriptor to the client's connection
 * @return - a pointer to the submit job payload struct containing all data
 *           received from the submitter or NULL on error
 */
subjob_payload_t * recv_and_pack_payload (int client_conn);

#endif