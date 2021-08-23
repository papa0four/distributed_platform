#define _GNU_SOURCE
#ifndef __QUERY_WORK_H__
#define __QUERY_WORK_H__

#include <stdio.h>
#include <ctype.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <signal.h>
#include <pthread.h>
#include <inttypes.h>
#include <errno.h>
#include "global_data.h"

/**
 * @brief - appropriately dequeue's work from the work queue, assigns the worker's
 *          socket file descriptor, packs the job packet, and sends the data to the
 *          worker
 * @param worker_fd - the socket file descriptor of the worker assigned to the task
 * @return - N/A
 */
void send_task_to_worker (int worker_fd);

#endif