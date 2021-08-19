#include "../includes/scheduler.h"

job_t           ** pp_jobs;
work_queue_t     * p_wqueue;
pthread_mutex_t    jobs_list_mutex;
pthread_mutex_t    running_mutex;
pthread_mutex_t    wqueue_mutex;
pthread_cond_t     condition;
volatile size_t    jobs_list_len;
volatile bool      g_running;
int                num_clients;
pthread_t          worker_threads[MAX_CLIENTS];

int initialize_global_data ()
{
    g_running = true;

    int func_ret = -1;

    func_ret = pthread_cond_init(&condition, NULL);
    if (0 != func_ret)
    {
        fprintf(stderr, "could not initialize pthread_cond\n");
        return -1;
    }

    func_ret = pthread_mutex_init(&wqueue_mutex, NULL);
    if (0 != func_ret)
    {
        fprintf(stderr, "could not initialize queue mutex\n");
        return -1;
    }

    func_ret = pthread_mutex_init(&running_mutex, NULL);
    if (0 != func_ret)
    {
        fprintf(stderr, "could not initialize running mutex\n");
        return -1;
    }

    func_ret = pthread_mutex_init(&jobs_list_mutex, NULL);
    if (0 != func_ret)
    {
        fprintf(stderr, "could not initialize jobs list mutex\n");
        return -1;
    }

    // initialize work queue
    p_wqueue = wqueue_init();
    if (NULL == p_wqueue)
    {
        fprintf(stderr, "could not intialize work queue\n");
        return -1;
    }

    // set max job list size
    jobs_list_len = MAX_JOBS;

    //allocate jobs list
    pp_jobs = calloc(jobs_list_len, sizeof(job_t *));
    if (NULL == pp_jobs)
    {
        errno = ENOMEM;
        perror("could not allocate memory for jobs list");
        CLEAN(p_wqueue);
        return -1;
    }

    num_clients = 0;

    memset(worker_threads, 0, MAX_CLIENTS * sizeof(pthread_t));

    return 0;
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
    for (size_t idx = 0; idx < jobs_list_len; idx++)
    {
        if (NULL == pp_jobs[idx])
        {
            p_job->job_id = idx;
            for (size_t jdx = 0; jdx < p_job->num_items; jdx++)
            {
                p_job->p_work[jdx].job_id = p_job->job_id;
            }
            pp_jobs[idx] = p_job;
            populate_wqueue(pp_jobs[idx]);
            pthread_mutex_unlock(&jobs_list_mutex);
            return idx;
        }
    }
    pthread_mutex_unlock(&jobs_list_mutex);
    fprintf(stderr, "could not place job in jobs list\n");
    return -1;
}

job_t * find_job (uint32_t job_id)
{
    for (size_t idx = 0; idx < jobs_list_len; idx++)
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
                return;
            }
        }
    }
    pthread_mutex_unlock(&jobs_list_mutex);
}

bool jobs_done (job_t * p_job)
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
        return true;
    }
    return false;
}

void populate_wqueue (job_t * p_job)
{
    if (NULL == p_job)
    {
        errno = EINVAL;
        perror("job parameter passed is NULL");
        return;
    }
    bool b_enqueued = false;
    for (size_t idx = 0; idx < p_job->num_items; idx++)
    {
        b_enqueued = enqueue_work(&p_job->p_work[idx]);
        if (false == b_enqueued)
        {
            fprintf(stderr, "could not add work to queue\n");
            break;
        }
    }
}

/*** end of initialize_globals.c ***/
