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

/**
 * opchain struct
 */
typedef struct _opchain_t
{
    uint32_t operation;
    uint32_t operand;
} opchain_t;

/**
 * operand types
 */
typedef struct _item_t
{
    uint32_t item;
} item_t;

/**
 * header stuffs
 */
typedef struct _header_t
{
    uint32_t    version;
    uint32_t    operation;
} header_t;

/**
 * struct info:
 */
typedef struct _submit_job_payload_t
{
    uint32_t    num_operations;
    opchain_t * op_groups;
    uint32_t    num_iters;  // always 1
    uint32_t    num_items;
    item_t    * items;
} subjob_payload_t;

/**
 * work
 */
typedef struct _work_t
{
    uint32_t    job_id;
    uint32_t    iterations;
    uint32_t    item;
    int32_t     answer;
    int         worker_sock;
    bool        b_work_done;
    opchain_t * p_chain;
} work_t;

/**
 * job
 */
typedef struct _job_t
{
    uint32_t    job_id;
    work_t    * p_work;
    bool        b_job_done;
    uint32_t    num_items;
    uint32_t    num_operations;
} job_t;

#endif
