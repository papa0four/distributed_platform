#define _GNU_SOURCE
#include <stdlib.h>
#include "../includes/global_data.h"

job_t           ** pp_jobs;
work_queue_t     * p_job_queue;
// work_queue_t     * p_progress_queue;
pthread_mutex_t    jobs_list_mutex;
pthread_mutex_t    running_mutex;
pthread_mutex_t    wqueue_mutex;
pthread_cond_t     condition;
volatile size_t    jobs_list_len;
size_t             max_job_list_sz;
int                num_clients;
pthread_t          worker_threads[MAX_CLIENTS];
volatile sig_atomic_t      g_running;

int initialize_global_data ()
{
    g_running = true;

    int func_ret = -1;

    func_ret = pthread_cond_init(&condition, NULL);
    if (0 != func_ret)
    {
        fprintf(stderr, "%s could not initialize pthread_cond\n", __func__);
        return -1;
    }

    func_ret = pthread_mutex_init(&wqueue_mutex, NULL);
    if (0 != func_ret)
    {
        fprintf(stderr, "%s could not initialize queue mutex\n", __func__);
        return -1;
    }

    func_ret = pthread_mutex_init(&running_mutex, NULL);
    if (0 != func_ret)
    {
        fprintf(stderr, "%s could not initialize running mutex\n", __func__);
        return -1;
    }

    func_ret = pthread_mutex_init(&jobs_list_mutex, NULL);
    if (0 != func_ret)
    {
        fprintf(stderr, "%s could not initialize jobs list mutex\n", __func__);
        return -1;
    }

    // initialize job queue
    p_job_queue = wqueue_init();
    if (NULL == p_job_queue)
    {
        fprintf(stderr, "%s could not intialize work queue\n", __func__);
        return -1;
    }

    // p_progress_queue = wqueue_init();
    // if (NULL == p_progress_queue)
    // {
    //     fprintf(stderr, "%s could not intialize progress queue\n", __func__);
    // }

    // set max job list size
    max_job_list_sz = MAX_JOBS;

    jobs_list_len = 0;


    //allocate jobs list
    pp_jobs = calloc(max_job_list_sz, sizeof(job_t *));
    if (NULL == pp_jobs)
    {
        errno = ENOMEM;
        perror("initialize_global_data could not allocate memory for jobs list");
        CLEAN(p_job_queue);
        return -1;
    }

    num_clients = 0;

    return 0;
}

static size_t get_jobid ()
{
    size_t min = 1;
    size_t max = MAX_ID;
    size_t result;

    srand(time(NULL));


    result = min + rand() % ((max + 1) - min);

    return result;
}

int populate_jobs_and_queue (job_t * p_job)
{
    if (NULL == p_job)
    {
        errno = EINVAL;
        perror("job pointer passed is NULL");
        return -1;
    }

    pthread_mutex_lock(&jobs_list_mutex);
    for (size_t idx = 0; idx < max_job_list_sz; idx++)
    {
        if (NULL == pp_jobs[idx])
        {
            p_job->job_id = get_jobid();
            usleep(SLEEPYTIME);
            for (size_t jdx = 0; jdx < p_job->num_items; jdx++)
            {
                p_job->p_work[jdx].job_id = p_job->job_id;
            }
            pp_jobs[idx] = p_job;
            jobs_list_len++;

            populate_queue(p_job_queue, pp_jobs[idx]);

            pthread_mutex_unlock(&jobs_list_mutex);
            pthread_cond_broadcast(&condition);
            return 0;
        }
    }
    pthread_mutex_unlock(&jobs_list_mutex);
    pthread_cond_broadcast(&condition);
    fprintf(stderr, "could not place job in jobs list\n");
    return -1;
}

job_t * find_job (uint32_t job_id)
{
    for (size_t idx = 0; idx < max_job_list_sz; idx++)
    {
        if ((NULL != pp_jobs[idx]) && (job_id == pp_jobs[idx]->job_id))
        {
            return pp_jobs[idx];
        }
    }
    return NULL;
}

void recv_answer (int worker_conn, int32_t answer)
{
    if (-1 == worker_conn)
    {
        errno = EBADF;
        perror("worker socket fd passed is invalid");
        return;
    }

    pthread_mutex_lock(&jobs_list_mutex);
    for (size_t idx = 0; idx < jobs_list_len; idx++)
    {
        for (size_t jdx = 0; jdx < pp_jobs[idx]->num_items; jdx++)
        {
            if ((worker_conn == pp_jobs[idx]->p_work[jdx].worker_sock) &&
                    (false == pp_jobs[idx]->p_work[jdx].b_work_done))
            {
                pp_jobs[idx]->p_work[jdx].answer        = answer;
                pp_jobs[idx]->p_work[jdx].b_work_done   = true;
                if (jobs_done(pp_jobs[idx]))
                {
                    pp_jobs[idx]->b_job_done = true;
                }
                pthread_mutex_unlock(&jobs_list_mutex);
                pthread_cond_broadcast(&condition);
                return;
            }
        }
    }

    pthread_mutex_unlock(&jobs_list_mutex);
    pthread_cond_broadcast(&condition);
    return;
}

int jobs_done (job_t * p_job)
{
    size_t      completed_cnt = 0;
    uint32_t    job_id        = 0;
    uint32_t    item          = 0;
    int32_t     answer        = 0;

    for (size_t idx = 0; idx < p_job->num_items; idx++)
    {
        if (true == p_job->p_work[idx].b_work_done)
        {
            completed_cnt += 1;
        }
    }

    if (completed_cnt == p_job->num_items)
    {
        job_id = p_job->job_id;
        printf("Job ID: %u\n", job_id);

        // retrieve and print the item and answer for the work
        for (size_t idx = 0; idx < p_job->num_items; idx++)
        {
            item = ntohl(p_job->p_work[idx].item);
            answer = p_job->p_work[idx].answer;
            printf("\tItem: %u --> Answer: %d\n", item, answer);
        }
    }
    return true;
}

void populate_queue (work_queue_t * p_queue, job_t * p_job)
{
    if ((NULL == p_queue) || (NULL == p_job))
    {
        errno = EINVAL;
        fprintf(stderr, "%s one or more parameters passed are NULL\n", __func__);
        return;
    }

    int b_enqueued = false;
    for (size_t idx = 0; idx < p_job->num_items; idx++)
    {
        b_enqueued = enqueue_work(p_queue, &p_job->p_work[idx]);
        if (false == b_enqueued)
        {
            fprintf(stderr, "could not add work to queue\n");
            break;
        }
    }
}

/*** end of initialize_globals.c ***/
