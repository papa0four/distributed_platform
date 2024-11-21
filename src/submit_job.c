
#define _GNU_SOURCE
#include "../includes/submit_job.h"
#include "../includes/network_handler.h"

uint32_t recv_num_operations (int client_conn)
{
    if (-1 == client_conn)
    {
        errno = EBADF;
        perror("bad file descriptor passed");
        return 0;
    }

    uint32_t num_operations = 0;
    ssize_t  bytes_recv     = 0;


    bytes_recv = recv(client_conn, &num_operations, sizeof(uint32_t), 0);
    if (-1 == bytes_recv)
    {
        perror("num_operations recv");
        return 0;
    }
    num_operations = ntohl(num_operations);
    return num_operations;
}

opchain_t * recv_opchain (int client_conn, uint32_t num_ops)
{
    if (-1 == client_conn)
    {
        errno = EBADF;
        perror("bad file descriptor passed");
        return 0;
    }

    if (0 == num_ops)
    {
        errno = EINVAL;
        perror("num_ops is 0");
        return NULL;
    }

    ssize_t     bytes_recv  = 0;
    uint32_t    operation   = 0;
    uint32_t    operand     = 0;
    opchain_t * p_ops       = calloc(num_ops, sizeof(opchain_t));
    if (NULL == p_ops)
    {
        errno = ENOMEM;
        perror("could not allocate op chain");
        return NULL;
    }

    for (size_t idx = 0; idx < num_ops; idx++)
    {
        bytes_recv = recv(client_conn, &operation, sizeof(uint32_t), 0);
        if (0 >= bytes_recv)
        {
            perror("operation recv");
            CLEAN(p_ops);
            return NULL;
        }

        bytes_recv = recv(client_conn, &operand, sizeof(uint32_t), 0);
        if (0 >= bytes_recv)
        {
            perror("operand recv");
            CLEAN(p_ops);
            return NULL;
        }
        operation = ntohl(operation);
        operand   = ntohl(operand);
        memcpy(&p_ops[idx].operation, &operation, sizeof(uint32_t));
        memcpy(&p_ops[idx].operand, &operand, sizeof(uint32_t));
    }

    return p_ops;
}

uint32_t recv_iterations (int client_conn)
{
    if (-1 == client_conn)
    {
        errno = EBADF;
        perror("bad file descriptor passed");
        return 0;
    }

    ssize_t     bytes_recv  = 0;
    uint32_t    num_iters   = 0;

    bytes_recv = recv(client_conn, &num_iters, sizeof(uint32_t), 0);
    if (0 >= bytes_recv)
    {
        perror("iteration recv");
        return 0;
    }

    num_iters = ntohl(num_iters);
    if (num_iters < 1)
    {
        perror("default iterations is 1");
        return 1;
    }

    return num_iters;
}

uint32_t recv_num_items (int client_conn)
{
    if (-1 == client_conn)
    {
        errno = EBADF;
        perror("bad file descriptor passed");
        return 0;
    }

    ssize_t     bytes_recv  = 0;
    uint32_t    num_items   = 0;

    bytes_recv = recv(client_conn, &num_items, sizeof(uint32_t), 0);
    if (0 >= bytes_recv)
    {
        perror("num_items recv");
        return 0;
    }
    
    num_items = ntohl(num_items);
    return num_items;
}

item_t * recv_items (int client_conn, uint32_t num_items)
{
    if (-1 == client_conn)
    {
        errno = EBADF;
        perror("bad file descriptor passed");
        return 0;
    }

    if (0 == num_items)
    {
        errno = EINVAL;
        perror("num_items is 0");
        return NULL;
    }

    ssize_t     bytes_recv  = 0;
    uint32_t    item        = 0;
    item_t    * p_items     = calloc(num_items, sizeof(item_t));
    if (NULL == p_items)
    {
        errno = ENOMEM;
        perror("could not allocate memory for p_items");
        return NULL;
    }

    for (size_t idx = 0; idx < num_items; idx++)
    {
        bytes_recv = recv(client_conn, &item, sizeof(uint32_t), 0);
        if (0 >= bytes_recv)
        {
            perror("recv item");
            CLEAN(p_items);
            return NULL;
        }
        item = ntohl(item);
        memcpy(&p_items[idx].item, &item, sizeof(uint32_t));
   }

    return p_items;
}

subjob_payload_t * pack_payload_struct (uint32_t num_operations, opchain_t * p_ops,
                uint32_t num_iters, uint32_t num_items, item_t * p_items)
{
    if ((0 == num_operations) || (0 == num_iters) ||
        (0 == num_items))
    {
        errno = EINVAL;
        perror("one or more uint32_t parameters are 0");
        goto ERROR;
    }

    if ((NULL == p_ops) || (NULL == p_items))
    {
        errno = EINVAL;
        perror("one or more lists passed are NULL");
        goto ERROR;
    }

    subjob_payload_t * p_payload = calloc(1, sizeof(subjob_payload_t));
    if (NULL == p_payload)
    {
        errno = ENOMEM;
        perror("could not allocate memory for p_payload");
        goto CLEANUP;
    }

    memcpy(&p_payload->num_operations, &num_operations, sizeof(uint32_t));
    p_payload->op_groups = calloc(p_payload->num_operations, sizeof(opchain_t));
    if (NULL == p_payload->op_groups)
    {
        errno = ENOMEM;
        perror("could not allocate payload opchain");
        goto CLEANUP;
    }
    memcpy(p_payload->op_groups, p_ops, (num_operations * sizeof(opchain_t)));
    memcpy(&p_payload->num_iters, &num_iters, sizeof(uint32_t));
    memcpy(&p_payload->num_items, &num_items, sizeof(uint32_t));
    p_payload->items = calloc(num_items, sizeof(item_t));
    if (NULL == p_payload->items)
    {
        errno = ENOMEM;
        perror("could not allocate payload items");
        goto CLEANUP;
    }
    memcpy(p_payload->items, p_items, (num_items * sizeof(item_t)));
    CLEAN(p_items);
    CLEAN(p_ops);
    return p_payload;

CLEANUP:
    CLEAN(p_payload);
ERROR:
    CLEAN(p_ops);
    CLEAN(p_items);
    return NULL;
}

subjob_payload_t * recv_and_pack_payload (int client_conn)
{
    if (-1 == client_conn)
    {
        errno = EBADF;
        perror("unpack_payload - bad file descripto passed");
        return NULL;
    }

    uint32_t           num_operations      = 0;
    uint32_t           num_iters           = 0;
    uint32_t           num_items           = 0;
    opchain_t        * p_ops               = NULL;
    item_t           * p_items             = NULL;
    subjob_payload_t * p_packed          = NULL;

    num_operations = recv_num_operations(client_conn);
    if (0 == num_operations)
    {
        fprintf(stderr, "no operations recv\n");
        goto ERROR;
    }

    p_ops = recv_opchain(client_conn, num_operations);
    if (NULL == p_ops)
    {
        perror("could not recv ops chain");
        goto ERROR;
    }
    
    num_iters = recv_iterations(client_conn);
    if (0 == num_iters)
    {
        perror("could not recv number of iterations");
        CLEAN(p_ops);
        goto ERROR;
    }

    num_items = recv_num_items(client_conn);
    if (0 == num_items)
    {
        perror("could not recv number of items");
        CLEAN(p_ops);
        goto ERROR;
    }

    p_items = recv_items(client_conn, num_items);
    if (NULL == p_items)
    {
        perror("could not recv items");
        CLEAN(p_ops);
        goto ERROR;
    }

    p_packed = pack_payload_struct(num_operations, p_ops, num_iters,
                        num_items, p_items);
    if (NULL == p_packed)
    {
        perror("could not pack payload struct");
        p_packed = NULL;
    }

ERROR:
    return p_packed;    
}

/*** end of submit_job.c ***/
