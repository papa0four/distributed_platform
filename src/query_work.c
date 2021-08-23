#define _GNU_SOURCE
#include "../includes/query_work.h"

void send_task_to_worker (int worker_fd)
{
    work_t * p_work = dequeue_work();
    if (NULL == p_work)
    {
        if (0 == g_running)
        {
            return;
        }
        fprintf(stderr, "could not retrieve task\n");
        return;
    }
    p_work->worker_sock = worker_fd;

    const job_t * p_job         = find_job(p_work->job_id);
    uint32_t      num_ops       = p_job->num_operations;
    uint32_t      offset        = 0;
    ssize_t       bytes_sent    = -1;

    char work_buffer[MAX_BUFF] = { 0 };

    p_work->item = htonl(p_work->item);

    memcpy(work_buffer, &p_work->item, sizeof(uint32_t));
    offset += sizeof(uint32_t);

    num_ops = htonl(num_ops);
    memcpy((work_buffer + offset), &num_ops, sizeof(uint32_t));
    offset += sizeof(uint32_t);

    for (size_t idx = 0; idx < p_job->num_operations; idx++)
    {
        p_work->p_chain[idx].operation = htonl(p_work->p_chain[idx].operation);
        memcpy((work_buffer + offset), 
                &p_work->p_chain[idx].operation, sizeof(uint32_t));
        offset += sizeof(uint32_t);

        p_work->p_chain[idx].operand = htonl(p_work->p_chain[idx].operand);
        memcpy((work_buffer + offset), 
                &p_work->p_chain[idx].operand, sizeof(uint32_t));
        offset += sizeof(uint32_t);
    }

    p_work->iterations = htonl(p_work->iterations);
    memcpy((work_buffer + offset), &p_work->iterations, sizeof(uint32_t));
    offset += sizeof(uint32_t);

    bytes_sent = send(worker_fd, work_buffer, offset, 0);
    if (0 >= bytes_sent)
    {
        fprintf(stderr, "could not send work packet to worker\n");
        return;
    }
}

/*** end query_work.c ***/
