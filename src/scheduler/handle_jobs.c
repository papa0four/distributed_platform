#include "includes/scheduler.h"

job_t * create_job (subjob_payload_t * p_job_payload)
{
    if (NULL == p_job_payload)
    {
        errno = EINVAL;
        perror("job parameter passed is NULL");
        return NULL;
    }
    uint32_t num_items = p_job_payload->num_items;
    job_t * p_job      = calloc(1, sizeof(job_t));
    if (NULL == p_job)
    {
        errno = ENOMEM;
        perror("could not allocate job");
        return NULL;
    }

    p_job->job_id           = -1;
    p_job->num_items        = num_items;
    p_job->num_operations   = p_job_payload->num_operations;
    p_job->p_work           = calloc(num_items, sizeof(work_t));
    if (NULL == p_job->p_work)
    {
        errno = ENOMEM;
        perror("could not allocate work array");
        clean_memory(p_job);
        return NULL;
    }

    for (size_t idx = 0; idx < num_items; ++idx)
    {
        p_job->p_work[idx].item    = p_job_payload->items[idx].item;
        p_job->p_work[idx].p_chain = calloc(p_job->num_operations, sizeof(opchain_t));
        if (NULL == p_job->p_work[idx].p_chain)
        {
            errno = ENOMEM;
            perror("could not allocate work's opchain");
            clean_memory(p_job->p_work);
            clean_memory(p_job);
            return NULL;
        }

        for (size_t jdx = 0; jdx < p_job->num_operations; ++jdx)
        {
            p_job->p_work[idx].p_chain[jdx].operation = 
            p_job_payload->op_groups[jdx].operation;

            p_job->p_work[idx].p_chain[jdx].operand =
            p_job_payload->op_groups[jdx].operand;
        }

        p_job->p_work[idx].iterations = p_job_payload->num_iters;
        p_job->p_work[idx].b_work_done  = false;
        p_job->p_work[idx].answer       = -1;
        p_job->p_work[idx].worker_sock  = -1;
        p_job->p_work[idx].job_id       = -1;

        if (NULL == &p_job->p_work[idx])
        {
            clean_memory(&p_job->p_work[idx]);
            fprintf(stderr, "error building work for job");
            return NULL;
        }
    }
    return p_job;
}

void destroy_jobs ()
{
    if (NULL == pp_jobs)
    {
        return;
    }

    for (size_t idx = 0; idx < jobs_list_len; ++idx)
    {
        if (NULL != pp_jobs[idx])
        {
            for(size_t jdx = 0; jdx < pp_jobs[idx]->num_items; jdx++)
            {
                if (NULL != &pp_jobs[idx]->p_work[jdx])
                {
                    free(pp_jobs[idx]->p_work[jdx].p_chain);
                }
                else
                {
                    break;
                }
                
            }
            clean_memory(pp_jobs[idx]->p_work);
            clean_memory(pp_jobs[idx]);
        }
    }
}

/*** end of handle_jobs.c ***/
