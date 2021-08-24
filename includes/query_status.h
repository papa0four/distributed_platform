#ifndef __QUERY_STATUS_H__
#define __QUERY_STATUS_H__

#define _GNU_SOURCE

#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include "global_data.h"
#include "handle_jobs.h"

//predefined conversion functions
#define pack754_32(f) (pack754((f), 32, 8))
#define unpack754_32(i) (unpack((i), 32, 8))

/**
 * @brief - converts a long double floating point number into a hexidecimal value able
 *          to be transmitted over the wire, maintaining signed-ness and value
 *          NOTE: taken directly from BEEJ's Guide to Network Programming section 7.5
 * @param f - the long double number to convert to hexidecimal
 * @param bits - numbers of bits contained in the original value
 * @param expbits - the number of bits contained within the exponent value
 * @return - an unsigned 32 bit value that can be printed in hex and sent over the wire
 */
uint64_t pack754(long double f, unsigned bits, unsigned expbits);

/**
 * @brief - converts an unsigned 64 bit integer passed over the wire into the appropriate
 *          long double floating point number.
 *          NOTE: taken directly from BEEJ's Guide to Network Programming section 7.5
 * @param i - the number received to convert back into a long double
 * @param bit - the number of total bits contained in the original value
 * @param expbits - the number of bits contained within the exponent value
 * @return - a 32 bit long double contained the appropriate value passed over the wire
 */
long double unpack754(uint64_t i, unsigned bits, unsigned expbits);

/**
 * @brief - receives the job id from the submitter and prepares the query packet structure
 * @param client_conn - the file descriptor for the connection to the submitter
 * @return - a pointer to the query_t struct storing the requested job id
 *           NULL on errors either allocating memory or receiving data from the submitter
 */
query_t * get_requested_job(int client_conn);

/**
 * @brief - searches the global jobs array looking for the associated job requested
 *          by the submitter, before determining if there is any data ready to
 *          prepare and send back to the submitter
 * @param p_queried_work - the query_t structure containing the requested job id,
 *                         the remainder of the struct members will be packed within
 *                         this function pending no errors occur
 * @param job_id - the requested job id from the submitter, used to locate the associated job
 *                 and its data
 * @return - fully packed query_t struct containing all relevant data to submit to the submitter
 *           NULL on errors or if there is no work to send to the submitter
 */
query_t * query_task_status (query_t * p_queried_work, uint32_t job_id);

/**
 * 
 */
int send_query_results (query_t * p_queried_work, int client_conn);

#endif