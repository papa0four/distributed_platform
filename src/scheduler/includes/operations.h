#ifndef __PROTOCOL_H__
#define __PROTOCOL_H__

#include "scheduler.h"

// define pre-processor constants
#define VERSION         1
#define ITERATIONS      1
#define MAX_JOBS        50

// define operations
#define SUBMIT_JOB      0
#define QUERY_WORK      3
#define SUBMIT_WORK     4
#define SHUTDOWN        5

// mathematical macros
#define ADD     0
#define SUBR    1
#define SUBL    2
#define AND     3
#define OR      4
#define XOR     5
#define NOT     6
#define ROLR    7
#define ROLL    8

/**
 * @brief - contains the opchain group data
 * @member operation - an unsigned 32 bit integer determining the mathematical
 *                     operation to perform
 * @member operand - an unsigned 32 bit integer determining the operand to perform
 *                   the operation upon
 */
typedef struct _opchain_t
{
    uint32_t operation;
    uint32_t operand;
} opchain_t;

/**
 * @brief - an item structure containing the individual item to perform the op_chain
 *          on
 * @member item - the individual item stored as an unsigned 32 bit integer
 */
typedef struct _item_t
{
    uint32_t item;
} item_t;

/**
 * @brief - a structure containing the platform packet protocol header data
 * @member version - an unsigned 32 bit integer containing the packet protocol
 *                   version
 * @member operation - an unsigned 32 bit integer containing the operation of
 *                     the packet
 */
typedef struct _header_t
{
    uint32_t    version;
    uint32_t    operation;
} header_t;

/**
 * @brief - a structure containing all the appropriate data received from the
 *          submitter
 * @member num_operations - an unsigned 32 bit integer containing the size of
 *                          the op chain
 * @member op_groups - an array of operation/operand groups to perform against
 *                     the item
 * @member num_iters - an unsigned 32 bit integer containing the number of times
 *                     the op chain will be performed against the item (always 1)
 * @member num_items - an unsigned 32 bit integer containing the number of items
 *                     to perform the operations chain against
 * @member items - an array of items passed by the submitter
 */
typedef struct _submit_job_payload_t
{
    uint32_t    num_operations;
    opchain_t * op_groups;
    uint32_t    num_iters;  // always 1
    uint32_t    num_items;
    item_t    * items;
} subjob_payload_t;

#endif
