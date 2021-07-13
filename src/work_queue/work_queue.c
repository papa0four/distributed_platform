#include "../includes/work_queue.h"
#include "../includes/scheduler.h"

work_queue_t * tqueue_init()
{
    work_queue_t * p_new_queue = calloc(1, sizeof(work_queue_t));
    if (NULL == p_new_queue)
    {
        errno = ENOMEM;
        perror("could not allocate memory for new queue");
        return NULL;
    }

    p_new_queue->p_nodes = calloc(QUEUE_CAPACITY, sizeof(queue_node_t));
    if (NULL == p_new_queue->p_nodes)
    {
        errno = ENOMEM;
        perror("could not allocate array of work");
        clean_memory(p_new_queue);
        return NULL;
    }

    p_new_queue->q_size       = 0;
    p_new_queue->capacity   = QUEUE_CAPACITY;

    return p_new_queue;
}

bool enqueue_work (work_queue_t * p_wqueue, work_t * p_work)
{
    if ((NULL == p_wqueue) || (NULL == p_work))
    {
        errno = EINVAL;
        perror("could not enqueue - one or more parameters are NULL");
        return false;
    }

    bool b_enqueued = false;
    queue_node_t * p_new_node = calloc(1, sizeof(queue_node_t));
    if (NULL == p_new_node)
    {
        errno = ENOMEM;
        perror("could not allocate new node");
        return b_enqueued;
    }

    p_new_node->work = p_work;

    if (NULL == p_wqueue->head)
    {
        p_wqueue->head = p_new_node;
        p_wqueue->q_size += 1;
        b_enqueued = true;
    }
    else
    {
        if (1 == is_full(p_wqueue))
        {
            p_wqueue = resize_tqueue(p_wqueue);
        }
        queue_node_t * p_current = p_wqueue->head;
        while (p_current->next)
        {
            p_current = p_current->next;
        }
        p_current->next = p_new_node;
        p_wqueue->q_size += 1;
        b_enqueued = true;
    }
    return b_enqueued;    
}

work_t * dequeue_work (work_queue_t * p_wqueue)
{
    if ((NULL == p_wqueue) || (NULL == p_wqueue->head))
    {
        errno = EINVAL;
        perror("the queue or queue head are NULL");
        return NULL;
    }

    queue_node_t * p_work_ret   = p_wqueue->head;
    work_t       * p_work       = p_work_ret->work;
    
    if (NULL == p_wqueue->head->next)
    {
        p_wqueue->head = NULL;
    }
    else
    {
        p_wqueue->head = p_wqueue->head->next;
    }

    p_wqueue->q_size -= 1;
    clean_memory(p_work_ret);

    return p_work;
}

ssize_t is_full (work_queue_t * p_wqueue)
{
    if ((NULL == p_wqueue) || (NULL == p_wqueue))
    {
        errno = EINVAL;
        perror("queue pointer passed or queue head is NULL");
        return -1;
    }

    if (p_wqueue->q_size == p_wqueue->capacity)
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

ssize_t tqueue_len (work_queue_t * p_wqueue)
{
    if ((NULL == p_wqueue) || (NULL == p_wqueue->head))
    {
        errno = EINVAL;
        perror("queue pointer or queue head is NULL");
        return -1;
    }

    return p_wqueue->q_size;
}

work_queue_t * resize_tqueue (work_queue_t * p_wqueue)
{
    if (NULL == p_wqueue)
    {
        errno = EINVAL;
        perror("queue pointer passed is NULL");
        return NULL;
    }

    work_queue_t * p_bigger_queue = NULL;
    p_bigger_queue = realloc(p_wqueue, (QUEUE_CAPACITY * 2) * sizeof(work_queue_t));
    if (NULL == p_bigger_queue)
    {
        errno = ENOMEM;
        perror("could not reallocate queue");
        return NULL;
    }

    p_bigger_queue->capacity = (QUEUE_CAPACITY * 2);
    return p_bigger_queue;
}

bool tqueue_destroy (work_queue_t * p_wqueue)
{
    if (NULL == p_wqueue)
    {
        errno = EINVAL;
        perror("queue pointer is NULL, nothing to destroy");
        return false;
    }

    if (1 == is_empty(p_wqueue))
    {
        printf("destroying task queue container\n");
        clean_memory(p_wqueue);
        return true;
    }

    queue_node_t * p_current = p_wqueue->head;
    queue_node_t * p_temp    = NULL;

    while (p_current)
    {
        p_temp      = p_current;
        p_current   = p_current->next;

        clean_memory(p_temp);
        p_wqueue->q_size -= 1;
    }
    p_wqueue->q_size    = 0;
    p_wqueue->capacity  = 0;

    clean_memory(p_wqueue);
    return true;
}

/*** end work_queue.c ***/
