#include "../includes/global_data.h"
#include "../includes/scheduler.h"

work_queue_t * wqueue_init()
{
    work_queue_t * p_queue = calloc(1, sizeof(work_queue_t));
    if (NULL == p_queue)
    {
        errno = ENOMEM;
        perror("queue init - could not initialize queue container");
        return NULL;
    }

    p_queue->head      = 0;
    p_queue->tail      = 0;
    p_queue->size      = 0;
    p_queue->capacity  = QUEUE_CAPACITY;

    p_queue->p_work = calloc(QUEUE_CAPACITY, sizeof(work_t *));
    if (NULL == p_queue->p_work)
    {
        errno = ENOMEM;
        perror("queue init - could not allocate memory for work list");
        CLEAN(p_queue);
        return NULL;
    }

    return p_queue;
}

int enqueue_work (work_queue_t * p_queue, work_t * p_work)
{
    if ((NULL == p_queue) || (NULL == p_work))
    {
        errno = EINVAL;
        perror("one or more parameters passed are NULL");
        return false;
    }

    pthread_mutex_lock(&wqueue_mutex);
    if (1 == is_empty(p_queue))
    {
        p_queue->p_work[p_queue->head] = p_work;
        p_queue->tail      += 1;
        p_queue->size      += 1;

        pthread_mutex_unlock(&wqueue_mutex);
        pthread_cond_broadcast(&condition);
        return true;
    }

    if (1 == is_full(p_queue))
    {
        p_queue = resize_wqueue(p_queue);
    }    
    p_queue->p_work[p_queue->tail] = p_work;
    p_queue->tail      += 1;
    p_queue->size      += 1;

    pthread_mutex_unlock(&wqueue_mutex);
    pthread_cond_broadcast(&condition);

    return true;
}

work_t * dequeue_work (work_queue_t * p_queue)
{
    if (NULL == p_queue)
    {
        return NULL;
    }

    pthread_mutex_lock(&wqueue_mutex);
    while ((g_running) && (1 == is_empty(p_queue)))
    {
        pthread_cond_wait(&condition, &wqueue_mutex);
        if (false == g_running)
        {
            pthread_mutex_unlock(&wqueue_mutex);
            pthread_cond_broadcast(&condition);
            return NULL;
        }
    }

    work_t * p_work = p_queue->p_work[p_queue->head];
    p_queue->head      += 1;
    p_queue->size      -= 1;
    pthread_mutex_unlock(&wqueue_mutex);
    pthread_cond_broadcast(&condition);

    return p_work;
}

int is_full (work_queue_t * p_queue)
{
    if (NULL == p_queue)
    {
        errno = EINVAL;
        perror("queue pointer passed or queue head is NULL");
        return -1;
    }

    if (p_queue->size == (p_queue->capacity - 1))
    {
        return 1;
    }
    
    return 0;    
}

int is_empty (work_queue_t * p_queue)
{
    if (NULL == p_queue)
    {
        errno = EINVAL;
        perror("queue pointer passed is NULL");
        return -1;
    }

    if (0 == p_queue->size)
    {
        return 1;
    }
    
    return 0;
    
}

int wqueue_len (work_queue_t * p_queue)
{
    if (NULL == p_queue)
    {
        errno = EINVAL;
        perror("queue pointer is NULL");
        return -1;
    }

    return p_queue->size;
}

work_queue_t * resize_wqueue (work_queue_t * p_queue)
{
    if (NULL == p_queue)
    {
        errno = EINVAL;
        perror("resize queue - pointer to container is NULL");
        return NULL;
    }
    p_queue = realloc(p_queue, (p_queue->capacity * 2));
    if (NULL == p_queue)
    {
        errno = ENOMEM;
        perror("resize queue - could not reallocate container");
        CLEAN(p_queue);
        return NULL;
    }

    p_queue->capacity = (p_queue->capacity * 2);

    return p_queue;
}

int wqueue_destroy (work_queue_t * p_queue)
{
    if ((NULL == p_queue) || (NULL == p_queue->p_work))
    {
        return false;
    }

    if (1 == is_empty(p_queue))
    {
        if (p_queue)
        {
            if (p_queue->p_work)
            {
                CLEAN(p_queue->p_work);
            }
            CLEAN(p_queue);
        }
        return true;
    }
    CLEAN(p_queue->p_work);
    CLEAN(p_queue);
    return true;
}

/*** end work_queue.c ***/