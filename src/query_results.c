#define _GNU_SOURCE
#include "../includes/query_results.h"

query_t * get_query_answers (int client_conn)
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

    bytes_recv = recv(client_conn, &p_query->timeout, sizeof(uint32_t), 0);
    if (-1 == bytes_recv)
    {
        fprintf(stderr, "%s could not receive job id from submitter\n", __func__);
        CLEAN(p_query);
        return NULL;
    }

    p_query->job_id     = ntohl(p_query->job_id);
    p_query->timeout    = ntohl(p_query->timeout);

    return p_query;
}

query_t * query_result_status (query_t * p_queried_work)
{
    if (NULL == p_queried_work)
    {
        errno = EINVAL;
        fprintf(stderr, "%s query packet structure passed is NULL\n", __func__);
        return NULL;
    }

    signed int tasks_complete   = 0;
    signed int tasks_incomplete = 0;
    uint32_t   convert_to_milli = 1000;
    int        b_enqueued       = -1;
    job_t * p_found_job = find_job(p_queried_work->job_id);
    if (NULL == p_found_job)
    {
        fprintf(stderr, "requested job not found\n");
        CLEAN(p_queried_work);
        return NULL;
    }
    
    time_t start;
    start = time(NULL);

    for (uint32_t idx = 0; idx < p_found_job->num_items; idx++)
    {
        time_t current = time(NULL);
        if ((current == (start + (p_queried_work->timeout / convert_to_milli))) &&
            (false == p_found_job->p_work[idx].b_work_done))
        {
            printf("p_found_job->job_id: %u\n", p_found_job->job_id);
            printf("p_found_job->num_items: %u\n", p_found_job->num_items);
            b_enqueued = enqueue_work(p_job_queue, &p_found_job->p_work[idx]);
            if (1 != b_enqueued)
            {
                fprintf(stderr, "%s could not add failed job back into queue\n", __func__);
                break;
            }
            // printf("p_job_queue->head: %p\tp_job_queue->head->p_work: %p\n", (void *)p_job_queue->head, (void *)p_job_queue->head->p_work);
            break;
        }
        if (p_found_job->p_work[idx].b_work_done)
        {
            tasks_complete++;
        }
        else
        {
            tasks_incomplete++;
        }
    }

    if ((0 == tasks_complete) || ((uint32_t) tasks_complete != p_found_job->num_items))
    {
        p_queried_work->num_completed = tasks_complete;
        p_queried_work->num_total     = p_found_job->num_items;
        return p_queried_work;
    }

    p_queried_work->num_completed = tasks_complete;
    p_queried_work->num_total     = p_found_job->num_items;

    p_queried_work->p_answers     = calloc(tasks_complete, sizeof(int32_t));
    if (NULL == p_queried_work->p_answers)
    {
        errno = ENOMEM;
        fprintf(stderr, "%s could not allocate answer list for query results\n", __func__);
        CLEAN(p_queried_work);
        return NULL;

    }

    p_queried_work->p_items     = calloc(p_found_job->num_items, sizeof(uint32_t));
    if (NULL == p_queried_work->p_answers)
    {
        errno = ENOMEM;
        fprintf(stderr, "%s could not allocate answer list for query results\n", __func__);
        CLEAN(p_queried_work->p_answers);
        CLEAN(p_queried_work);
        return NULL;

    }

    for (uint32_t idx = 0; idx < p_queried_work->num_completed; idx++)
    {
        if (p_found_job->p_work[idx].b_work_done)
        {
            p_found_job->p_work[idx].item = htonl(p_found_job->p_work[idx].item);
            memcpy(&p_queried_work->p_items[idx],
                   &p_found_job->p_work[idx].item, sizeof(uint32_t));

            memcpy(&p_queried_work->p_answers[idx],
                   &p_found_job->p_work[idx].answer, sizeof(int32_t));
            p_queried_work->average += p_queried_work->p_answers[idx];
        }
    }

    p_queried_work->average /= p_queried_work->num_completed;
    p_queried_work->packed = pack754_64(p_queried_work->average);

    return p_queried_work;
}

query_results_t * get_answers(query_t * p_queried_work)
{
    query_results_t * p_results = calloc(1, sizeof(query_results_t));
    if (NULL == p_results)
    {
        errno = ENOMEM;
        fprintf(stderr, "%s could not allocate memory for query results packet\n", __func__);
        goto CLEANUP;
    }

    if (NULL == p_queried_work)
    {
        p_results->status       = NOJOB;
        p_results->num_results  = 0;
        p_results->p_answers    = NULL;
        return p_results;

    }

    if ((p_queried_work) && (p_queried_work->num_completed == p_queried_work->num_total))
    {
        p_results->status       = SUCCESS;
        p_results->num_results  = p_queried_work->num_total;
        p_results->timeout      = p_queried_work->timeout;
        p_results->p_answers = calloc(p_results->num_results, sizeof(int32_t));
        if (NULL == p_results->p_answers)
        {
            errno = ENOMEM;
            fprintf(stderr, "%s could not allocate answer list in results packet\n", __func__);
            CLEAN(p_results);
            goto CLEANUP;
        }

        p_results->p_items = calloc(p_results->num_results, sizeof(uint32_t));
        if (NULL == p_results->p_items)
        {
            errno = ENOMEM;
            fprintf(stderr, "%s could not allocate answer list in results packet\n", __func__);
            CLEAN(p_results);
            goto CLEANUP;
        }

        for (uint32_t idx = 0; idx < p_results->num_results; idx++)
        {
            memcpy(&p_results->p_items[idx], &p_queried_work->p_items[idx], sizeof(uint32_t));
            memcpy(&p_results->p_answers[idx], &p_queried_work->p_answers[idx], sizeof(int32_t));
        }
        
        goto IT_WORKED;
    }

    if ((p_queried_work) && (p_queried_work->num_completed != p_queried_work->num_total))
    {
        p_results->status       = NOTCOMPLETE;
        p_results->num_results  = 0;
        p_results->p_answers    = NULL;
        goto IT_WORKED;
    }

IT_WORKED:
    CLEAN(p_queried_work->p_items);
    CLEAN(p_queried_work->p_answers);
    CLEAN(p_queried_work);
    return p_results;

CLEANUP:
    CLEAN(p_queried_work->p_items);
    CLEAN(p_queried_work->p_answers);
    CLEAN(p_queried_work);
    return NULL;
}

int send_query_results_response(query_results_t * p_results, int client_conn)
{
    if (-1 == client_conn)
    {
        fprintf(stderr, "%s client socket descriptor passed is invalid\n", __func__);
        CLEAN(p_results->p_answers);
        CLEAN(p_results);
        return -1;
    }

    ssize_t bytes_sent      = -1;
    size_t  offset          = 0;
    uint32_t num_results    = 0;
    char    response[BUFF_SZ];

    switch (p_results->status)
    {
        case SUCCESS:
            memcpy((response + offset), &p_results->status, sizeof(uint32_t));
            offset += sizeof(uint32_t);

            num_results = p_results->num_results;
            p_results->num_results = htonl(p_results->num_results);
            memcpy((response + offset), &p_results->num_results, sizeof(uint32_t));
            offset += sizeof(uint32_t);

            for (uint32_t idx = 0; idx < num_results; idx++)
            {
                p_results->p_items[idx] = htonl(p_results->p_items[idx]);
                memcpy((response + offset), &p_results->p_items[idx], sizeof(uint32_t));
                offset += sizeof(uint32_t);

                p_results->p_answers[idx] = htonl(p_results->p_answers[idx]);
                memcpy((response + offset), &p_results->p_answers[idx], sizeof(int32_t));
                offset += sizeof(int32_t);
            }

            bytes_sent = send(client_conn, response, offset, 0);
            if (-1 == bytes_sent)
            {
                fprintf(stderr, "%s could not send response to submitter\n", __func__);
                CLEAN(p_results->p_answers);
                CLEAN(p_results);
                return -1;
            }
            CLEAN(p_results->p_items);
            CLEAN(p_results->p_answers);
            CLEAN(p_results);
            break;

        case NOJOB:
            p_results->status = htonl(p_results->status);
            memcpy((response + offset), &p_results->status, sizeof(uint32_t));
            offset += sizeof(uint32_t);

            bytes_sent = send(client_conn, response, offset, 0);
            if (-1 == bytes_sent)
            {
                fprintf(stderr, "%s could not send response to submitter\n", __func__);
                CLEAN(p_results->p_answers);
                CLEAN(p_results);
                return -1;
            }

            CLEAN(p_results);
            break;

        case NOTCOMPLETE:
            p_results->status = htonl(p_results->status);
            memcpy((response + offset), &p_results->status, sizeof(uint32_t));
            offset += sizeof(uint32_t);
            bytes_sent = send(client_conn, response, offset, 0);
            if (-1 == bytes_sent)
            {
                fprintf(stderr, "%s could not send response to submitter\n", __func__);
                CLEAN(p_results->p_answers);
                CLEAN(p_results);
                return -1;
            }
            CLEAN(p_results->p_answers);
            CLEAN(p_results);
            break;

        default:
            fprintf(stderr, "%s invalid packet status\n", __func__);
            return -1;
    }

    return 0;
}

/*** end query_results.c ***/
