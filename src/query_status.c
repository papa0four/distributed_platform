#include "../includes/query_status.h"

uint64_t pack754 (long double f, unsigned bits, unsigned expbits)
{
    double fnorm;
    int         shift;
    long   sign;
    long   exp;
    long   significand;

    unsigned significandbits = bits - expbits - 1; // -1 for sign bit

    // handle special case
    if (0.0 == f)
    {
        return 0;
    }

    // check sign and begin normalization
    if (f < 0)
    {
        sign    = 1;
        fnorm   = -f;
    }
    else
    {
        sign    = 0;
        fnorm   = f;
    }

    // get normalized form of f and track the exponent
    shift = 0;
    while (fnorm >= 2.0)
    {
        fnorm /= 2.0;
        shift++;
    }

    while (fnorm < 1.0)
    {
        fnorm *= 2.0;
        shift--;
    }

    fnorm -= 1.0;

    // calculate the binary form (non-float) of the significand data
    significand = fnorm * ((1LL<<significandbits) + 0.5f);

    // get the biased exponent
    exp = shift + ((1<<(expbits - 1)) - 1); // shift + bias

    //return the final answer
    return (sign << (bits - 1)) | (exp << (bits - expbits - 1)) | significand;    
}

long double unpack754 (uint64_t i, unsigned bits, unsigned expbits)
{
    long double result;
    long long   shift;
    unsigned    bias;

    unsigned significandbits = bits - expbits - 1; // -1 for sign bit

    // handle special case
    if (0 == i)
    {
        return 0.0;
    }

    // pull the significand
    result = (i & ((1LL << significandbits) - 1)); // mask
    result /= (1LL << significandbits); // convert back to float
    result += 1.0f; // add the one back on

    // deal with the exponent
    bias = (1 << (expbits - 1)) - 1;
    shift = (( i >> significandbits) & ((1LL << expbits) -1)) - bias;
    while (shift > 0)
    {
        result *= 2.0;
        shift--;
    }

    while (shift < 0)
    {
        result /= 2.0;
        shift++;
    }

    // sign it
    result *= (i >> (bits - 1)) & 1 ? -1.0 : 1.0;

    return result;
}

query_t * get_requested_job (int client_conn)
{
    if (-1 == client_conn)
    {
        errno = EBADF;
        fprintf(stderr, "%s client file descriptor passed is invalid\n", __func__);
        return NULL;
    }

    ssize_t     bytes_recv = -1;
    query_t   * p_query = calloc(1, sizeof(query_t));
    if (NULL == p_query)
    {
        errno = ENOMEM;
        fprintf(stderr, "%s could not allocate memory for query packet data\n", __func__);
        return NULL;
    }

    bytes_recv = recv(client_conn, &p_query->job_id, sizeof(uint32_t), 0);
    if (-1 == bytes_recv)
    {
        fprintf(stderr, "%s could not receive job id from submitter\n", __func__);
        CLEAN(p_query);
        return NULL;
    }

    p_query->job_id = ntohl(p_query->job_id);

    return p_query;
}

query_t * query_task_status (query_t * p_queried_work, uint32_t job_id)
{
    if (NULL == p_queried_work)
    {
        errno = EINVAL;
        fprintf(stderr, "%s query packet structure passed is NULL\n", __func__);
        return NULL;
    }

    signed int tasks_complete   = 0;
    signed int tasks_incomplete = 0;
    job_t * p_found_job = find_job(job_id);
    if (NULL == p_found_job)
    {
        fprintf(stderr, "requested job not found\n");
        CLEAN(p_queried_work);
        return NULL;
    }
    
    for (uint32_t idx = 0; idx < p_found_job->num_items; idx++)
    {
        if (p_found_job->p_work[idx].b_work_done)
        {
            tasks_complete++;
        }
        else
        {
            tasks_incomplete++;
        }
    }

    if (0 == tasks_complete)
    {
        // prepare error message for submitted
        CLEAN(p_queried_work);
        return NULL;
    }

    p_queried_work->num_completed = tasks_complete;
    p_queried_work->num_total     = tasks_complete + tasks_incomplete;
    p_queried_work->p_answers = calloc(tasks_complete, sizeof(int32_t));
    if (NULL == p_queried_work->p_answers)
    {
        errno = ENOMEM;
        fprintf(stderr, "%s could not allocate answer list for query results\n", __func__);
        CLEAN(p_queried_work);
        return NULL;

    }

    for (uint32_t idx = 0; idx < p_queried_work->num_completed; idx++)
    {
        if (p_found_job->p_work[idx].b_work_done)
        {
            memcpy(&p_queried_work->p_answers[idx],
                   &p_found_job->p_work[idx].answer, sizeof(int32_t));
            p_queried_work->average += p_queried_work->p_answers[idx];
        }
    }

    p_queried_work->average /= p_queried_work->num_completed;
    p_queried_work->packed = pack754_64(p_queried_work->average);

    return p_queried_work;
}

int send_query_results (query_t * p_queried_work, int client_conn)
{
    if ((NULL == p_queried_work) || (-1 == client_conn))
    {
        errno = EINVAL;
        fprintf(stderr, "%s one or more parameters passed are invalid\n", __func__);
        return -1;
    }

    ssize_t bytes_sent = -1;

    query_response_t q_response = { 0 };
    memcpy(&q_response.computed, &p_queried_work->num_completed, sizeof(uint32_t));
    q_response.computed = htonl(q_response.computed);

    memcpy(&q_response.total, &p_queried_work->num_total, sizeof(uint32_t));
    q_response.total = htonl(q_response.total);

    memcpy(&q_response.packed, &p_queried_work->packed, sizeof(uint64_t));
    q_response.packed = htobe64(q_response.packed);

    bytes_sent = send(client_conn, &q_response, sizeof(query_response_t), 0);
    if (-1 == bytes_sent)
    {
        errno = EBADF;
        fprintf(stderr, "%s could not send query status response to submitter\n", __func__);
        CLEAN(p_queried_work->p_answers);
        CLEAN(p_queried_work);
       
        return -1;
    }

    CLEAN(p_queried_work->p_answers);
    CLEAN(p_queried_work);
    return 0;
}

/*** end of query_status.c ***/