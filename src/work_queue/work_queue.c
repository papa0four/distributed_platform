#include "../includes/work_queue.h"
#include "../includes/scheduler.h"

work_queue_t * wqueue_init()
{
    work_queue_t * p_wqueue = calloc(1, sizeof(work_queue_t));
    if (NULL == p_wqueue)
    {
        errno = ENOMEM;
        perror("queue init - could not initialize queue container");
        return NULL;
    }

    p_wqueue->head      = 0;
    p_wqueue->tail      = 0;
    p_wqueue->q_size    = 0;
    p_wqueue->capacity  = QUEUE_CAPACITY;

    p_wqueue->p_work = calloc(QUEUE_CAPACITY, sizeof(work_t));
    if (NULL == p_wqueue->p_work)
    {
        errno = ENOMEM;
        perror("queue init - could not allocate memory for work list");
        clean_memory(p_wqueue);
        return NULL;
    }

    return p_wqueue;
}

bool enqueue_work (work_queue_t * p_wqueue, work_t * p_work)
{
    if ((NULL == p_wqueue) || (NULL == p_work))
    {
        errno = EINVAL;
        perror("one or more parameters passed are NULL");
        return false;
    }

    pthread_mutex_lock(&wqueue_mutex);
    if (1 == is_empty(p_wqueue))
    {
        p_wqueue->p_work[p_wqueue->head] = p_work;
        p_wqueue->tail      += 1;
        p_wqueue->q_size    += 1;

        pthread_mutex_unlock(&wqueue_mutex);
        pthread_cond_broadcast(&condition);
        return true;
    }

    if (1 == is_full(p_wqueue))
    {
        p_wqueue = resize_wqueue(p_wqueue);
    }    
    p_wqueue->p_work[p_wqueue->tail] = p_work;
    p_wqueue->tail      += 1;
    p_wqueue->q_size    += 1;

    pthread_mutex_unlock(&wqueue_mutex);
    pthread_cond_broadcast(&condition);

    return true;
}

work_t * dequeue_work (work_queue_t * p_wqueue)
{
    if (NULL == p_wqueue)
    {
        errno = EINVAL;
        perror("the queue is NULL");
        return NULL;
    }

    pthread_mutex_lock(&wqueue_mutex);
    while ((g_running) && is_empty(p_wqueue))
    {
        pthread_cond_wait(&condition, &wqueue_mutex);
        if (false == g_running)
        {
            pthread_mutex_unlock(&wqueue_mutex);
            clean_memory(p_wqueue);
            return NULL;
        }
    }

    work_t * p_work = p_wqueue->p_work[p_wqueue->head];
    p_wqueue->head      += 1;
    p_wqueue->q_size    -= 1;
    pthread_mutex_unlock(&wqueue_mutex);
    pthread_cond_broadcast(&condition);

    return p_work;
}

ssize_t is_full (work_queue_t * p_wqueue)
{
    if (NULL == p_wqueue)
    {
        errno = EINVAL;
        perror("queue pointer passed or queue head is NULL");
        return -1;
    }

    if (p_wqueue->q_size == (p_wqueue->capacity - 1))
    {
        return 1;
    }
    
    return 0;    
}

ssize_t is_empty (work_queue_t * p_wqueue)
{
    if (NULL == p_wqueue)
    {
        errno = EINVAL;
        perror("queue pointer passed is NULL");
        return -1;
    }

    if (0 == p_wqueue->q_size)
    {
        return 1;
    }
    
    return 0;
    
}

ssize_t wqueue_len (work_queue_t * p_wqueue)
{
    if (NULL == p_wqueue)
    {
        errno = EINVAL;
        perror("queue pointer is NULL");
        return -1;
    }

    return p_wqueue->q_size;
}

work_queue_t * resize_wqueue (work_queue_t * p_wqueue)
{
    if (NULL == p_wqueue)
    {
        errno = EINVAL;
        perror("resize queue - pointer to container is NULL");
        return NULL;
    }
    p_wqueue = realloc(p_wqueue, (p_wqueue->capacity * 2));
    if (NULL == p_wqueue)
    {
        errno = ENOMEM;
        perror("resize queue - could not reallocate container");
        clean_memory(p_wqueue);
        return NULL;
    }

    p_wqueue->capacity = (p_wqueue->capacity * 2);

    return p_wqueue;
}

bool wqueue_destroy (work_queue_t * p_wqueue)
{
    if (NULL == p_wqueue)
    {
        errno = EINVAL;
        perror("pointer to container passed is empty");
        return false;
    }

    if (1 == is_empty(p_wqueue))
    {
        if (p_wqueue)
        {
            if (p_wqueue->p_work)
            {
                clean_memory(p_wqueue->p_work);
            }
            clean_memory(p_wqueue);
        }
        return true;
    }

    for (size_t idx = 0; idx < p_wqueue->q_size; idx++)
    {
        if (p_wqueue->p_work[idx])
        {
            clean_memory(p_wqueue->p_work[idx]->p_chain);
            clean_memory(p_wqueue->p_work[idx]);
        }
    }
    clean_memory(p_wqueue);
    return true;
}

/*** end work_queue.c ***/
