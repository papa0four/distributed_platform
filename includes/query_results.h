#ifndef __QUERY_RESULTS_H__
#define __QUERY_RESULTS_H__

#define _GNU_SOURCE

#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>
#include "global_data.h"
#include "handle_jobs.h"
#include "query_status.h"

#define SUCCESS         0
#define NOJOB           1
#define NOTCOMPLETE     2
#define BUFF_SZ         1024

/**
 * 
 */
query_t * get_query_answers (int client_conn);

/**
 * 
 */
query_t * query_result_status (query_t * p_queried_work);

/**
 * 
 */
query_results_t * get_answers (query_t * p_queried_work);

/**
 * 
 */
int send_query_results_response(query_results_t * p_results, int client_conn);

#endif