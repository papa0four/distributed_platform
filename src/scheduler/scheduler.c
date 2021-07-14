#define _GNU_SOURCE

#include "../includes/scheduler.h"
#include "../includes/operations.h"
#include "../includes/work_queue.h"

volatile bool      g_running        = true;
job_t           ** pp_jobs          = NULL;        
work_queue_t     * p_wqueue         = NULL;
volatile size_t    jobs_list_len    = 0;

void sigint_handler (int signo)
{
    if (SIGINT == signo)
    {
        char msg[MAX_BUFF] = "\nSIGINT (ctrl+c) caught.\nShutting down...";
        fprintf(stderr, "%s\n", msg);
        g_running = false;
    }
}

uint16_t get_port(int argc, char ** argv)
{
    if (1 == argc)
    {
        errno = EINVAL;
        perror("no command line arguments detected, exiting ...");
        return 0;
    }

    int     opt     = 0;
    char  * ptr     = NULL;
    long    port    = 0;

    while (-1 != (opt = getopt(argc, argv, ":p")))
    {
        switch (opt)
        {
            case 'p':
                if (NULL == argv[optind])
                {
                    errno = EINVAL;
                    perror("command line arguments array is NULL, exiting ...");
                    port = 0;
                }

                port = strtol(argv[optind], &ptr, BASE_10);
                if ((MIN_PORT > port) || (MAX_PORT < port))
                {
                    errno = EINVAL;
                    perror("invalid port entered");
                    fprintf(stderr, "\tvalid port range: 31338 - 65535\n");
                    port = 0;
                }
                port = (uint16_t)port;
                break;

            case '?':
                errno = EINVAL;
                perror("invalid command line argument passed");
                fprintf(stderr, "only valid cmdline argument:\n\t-p [specified port]\n");
                port = 0;
                break;
        }
    }
    return port;
}

struct sockaddr_in setup_scheduler ()
{
    struct sockaddr_in scheduler = { 0 };
    memset((void *)&scheduler, 0, sizeof(scheduler));

    scheduler.sin_family            = AF_INET;
    scheduler.sin_addr.s_addr       = INADDR_ANY;
    scheduler.sin_port              = htons(BROADCAST_PORT);

    return scheduler;
}

int create_broadcast_socket (struct sockaddr_in scheduler)
{
    int         broadcast_socket    = 0;
    int         reuse               = 1;
    int         bind_ret            = -1;
    int         opt_ret             = -1;
    socklen_t   scheduler_len       = sizeof(scheduler);

    broadcast_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (0 == broadcast_socket)
    {
        perror("handle_broadcast - socket");
        return -1;
    }

    opt_ret = setsockopt(broadcast_socket, SOL_SOCKET, SO_REUSEADDR | SO_BROADCAST, &reuse, sizeof(reuse));
    if (-1 == opt_ret)
    {
        perror("handle_broadcast - setsockopt");
        close(broadcast_socket);
        return -1;
    }

    bind_ret = bind(broadcast_socket, (const struct sockaddr *)&scheduler, scheduler_len);
    if (0 > bind_ret)
    {
        perror("handle_broadcast - bind");
        return -1;
    }

    return broadcast_socket;
}

void * handle_broadcast (void * p_port)
{
    struct sockaddr_in scheduler    = setup_scheduler();
    struct sockaddr_in client       = { 0 };
    char broadcast_msg[MAX_BUFF]    = { 0 };

    socklen_t client_len      = sizeof(client);

    int         select_ret          = -1;
    ssize_t     bytes_recv          = 0;
    ssize_t     bytes_sent          = 0;
    fd_set      read_fd;
    int         broadcast_socket    = create_broadcast_socket(scheduler);

    uint16_t * op_port = p_port;
    *op_port = htons(*op_port);

    struct timeval tv;

    while (g_running)
    {
        tv.tv_sec   = TV_TIMEOUT;
        tv.tv_usec  = 0;
        FD_ZERO(&read_fd);
        FD_SET(broadcast_socket, &read_fd);

        select_ret = select(broadcast_socket + 1, &read_fd, NULL, NULL, &tv);
        if ((0 < select_ret) && (FD_ISSET(broadcast_socket, &read_fd)) && (true == g_running))
        {
            bytes_recv  = recvfrom(broadcast_socket, broadcast_msg, MAX_BUFF, 0,
                         (struct sockaddr *)&client, &client_len);
            if (0 > bytes_recv)
            {
                perror("handle_broadcast - recvfrom");
                close(broadcast_socket);
                return NULL;
            }

            printf("broadcast_msg recv'd from: %s:%hu\n", 
                    inet_ntoa(client.sin_addr), ntohs(client.sin_port));

            bytes_sent = sendto(broadcast_socket, op_port, sizeof(uint16_t), 0, 
                                (struct sockaddr *)&client, client_len);
            if (0 >= bytes_sent)
            {
                perror("broadcast_handler - sendto");
                close(broadcast_socket);
                return NULL;
            }
        }

        if (false == g_running)
        {
            break;
        }
    }

    close(broadcast_socket);
    pthread_exit(NULL);
    return NULL;
}

header_t * unpack_header (int client_conn)
{
    if (-1 == client_conn)
    {
        errno = EBADF;
        perror("unpack_header - invalid file descriptor passed");
        return NULL;
    }

    ssize_t     bytes_recv  = 0;
    uint32_t    version     = 0;
    uint32_t    operation   = 0;

    bytes_recv = recv(client_conn, &version, sizeof(uint32_t), 0);
    if (0 >= bytes_recv)
    {
        perror("version recv");
        return NULL;
    }

    if (VERSION != ntohl(version))
    {
        perror("protocol version invalid ...");
        return NULL;
    }

    bytes_recv = recv(client_conn, &operation, sizeof(uint32_t), 0);
    if (0 >= bytes_recv)
    {
        perror("operation recv");
        return NULL;
    }

    header_t * p_packet_hdr = calloc(1, sizeof(header_t));
    if (NULL == p_packet_hdr)
    {
        errno = ENOMEM;
        perror("could not allocate packet header struct");
        return NULL;
    }
    p_packet_hdr->operation = ntohl(operation);
    p_packet_hdr->version   = ntohl(version);

    return p_packet_hdr;
}

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
    if (0 >= bytes_recv)
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
            return NULL;
        }

        bytes_recv = recv(client_conn, &operand, sizeof(uint32_t), 0);
        if (0 >= bytes_recv)
        {
            perror("operand recv");
            return NULL;
        }
        operation = ntohl(operation);
        operand   = ntohl(operand);
        memcpy(&p_ops[idx].operation, &operation, sizeof(uint32_t));
        memcpy(&p_ops[idx].operand, &operand, sizeof(uint32_t));

        printf("operation: %u\toperand: %u\n", p_ops[idx].operation, p_ops[idx].operand);
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
    if (ITERATIONS != num_iters)
    {
        perror("only one iteration allowed");
        return 0;
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
            clean_memory(p_items);
            return NULL;
        }
        item = ntohl(item);
        memcpy(&p_items[idx].item, &item, sizeof(uint32_t));
        printf("item: %u\n", p_items[idx].item);
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
    return p_payload;

CLEANUP:
    clean_memory(p_payload);
ERROR:
    return NULL;
}

subjob_payload_t * unpack_payload (int client_conn)
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
    subjob_payload_t * p_packed            = NULL;

    num_operations = recv_num_operations(client_conn);
    if (0 == num_operations)
    {
        perror("no operations recv");
        goto ERROR;
    }
    printf("number of operations: %u\n", num_operations);

    p_ops = recv_opchain(client_conn, num_operations);
    if (NULL == p_ops)
    {
        perror("could not recv ops chain");
        goto END;
    }
    
    num_iters = recv_iterations(client_conn);
    if (0 == num_iters)
    {
        perror("could not recv number of iterations");
        goto END;
    }

    num_items = recv_num_items(client_conn);
    if (0 == num_items)
    {
        perror("could not recv number of items");
        goto END;
    }
    printf("number of items: %u\n", num_items);

    p_items = recv_items(client_conn, num_items);
    if (NULL == p_items)
    {
        perror("could not recv items");
        goto END;
    }

    p_packed = pack_payload_struct (num_operations,p_ops, num_iters,
                        num_items, p_items);
    if (NULL == p_packed)
    {
        perror("could not pack payload struct");
        p_packed = NULL;
    }

END:
    clean_memory(p_ops);
    clean_memory(p_items);
ERROR:
    return p_packed;    
}

void handle_submitter_req (void * p_client_socket)
{
    int         * p_client_conn = (int *) p_client_socket;

    header_t    * p_hdr         = unpack_header(*p_client_conn);
    if (NULL == p_hdr)
    {
        perror("could not pack header");
        return;
    }
    
    clean_memory(p_hdr);
    return;
}

int handle_working_socket (struct sockaddr_in scheduler)
{
    int         scheduler_fd    = 0;
    int         opt_ret         = 0;
    int         reuse           = 1;
    int         bind_ret        = -1;
    int         listen_ret      = -1;

    scheduler_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (0 == scheduler_fd)
    {
        perror("driver - socket");
        return -1;
    }

    opt_ret = setsockopt(scheduler_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
    if (-1 == opt_ret)
    {
        perror("driver - setsockopt");
        close(scheduler_fd);
        return -1;
    }

    bind_ret = bind(scheduler_fd, (const struct sockaddr *)&scheduler, sizeof(scheduler));
    if (0 > bind_ret)
    {
        perror("driver - bind");
        close(scheduler_fd);
        return -1;
    }

    listen_ret = listen(scheduler_fd, BACKLOG);
    if (-1 == listen_ret)
    {
        perror("driver - listen");
        close(scheduler_fd);
        return -1;
    }

    return scheduler_fd;
}

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

    for (size_t idx = 0; idx < num_items; idx++)
    {
        p_job->p_work[idx].item = p_job_payload->items[idx].item;
        p_job->p_work[idx].p_chain = calloc(p_job->num_operations, sizeof(opchain_t));
        if (NULL == p_job->p_work[idx].p_chain)
        {
            errno = ENOMEM;
            perror("could not allocate work's opchain");
            clean_memory(p_job->p_work);
            clean_memory(p_job);
            return NULL;
        }

        for (size_t jdx = 0; jdx < p_job->num_operations; jdx++)
        {
            p_job->p_work[idx].p_chain[jdx].operation =
            p_job_payload->op_groups[jdx].operation;

            p_job->p_work[idx].p_chain[jdx].operand =
            p_job_payload->op_groups[jdx].operand;
        }

        p_job->p_work[idx].iterations   = p_job_payload->num_iters;
        p_job->p_work[idx].b_task_done  = false;
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
    clean_memory(p_job_payload);
    return p_job;
}

void destroy_job ()
{
    for (size_t idx = 0; idx < jobs_list_len; idx++)
    {
        if (NULL != pp_jobs[idx])
        {
            for(size_t jdx = 0; jdx <= pp_jobs[idx]->num_operations; jdx++)
            {
                clean_memory(pp_jobs[idx]->p_work[jdx].p_chain);
            }
            clean_memory(pp_jobs[idx]->p_work);
            clean_memory(pp_jobs[idx]);
        }
    }
}

void clean_memory (void * memory_obj)
{
    if (NULL == memory_obj)
    {
        return;
    }
    free(memory_obj);
    memory_obj = NULL;
}

int main (int argc, char ** argv)
{
    struct sigaction handle_sigint = { 0 };
    handle_sigint.sa_handler       = sigint_handler;
    sigfillset(&handle_sigint.sa_mask);
    handle_sigint.sa_flags         = 0;
    sigaction(SIGINT, &handle_sigint, NULL);

    uint16_t    op_port         = 0;
    int         scheduler_fd    = 0;
    pthread_t   broadcast_thread;
    
    op_port = get_port(argc, argv);
    if (0 == op_port)
    {
        perror("get_port");
        return EXIT_FAILURE;
    }

    struct sockaddr_in  scheduler       = { 0 };
    socklen_t           scheduler_len   = sizeof(scheduler);

    scheduler.sin_family        = AF_INET;
    scheduler.sin_addr.s_addr   = INADDR_ANY;
    scheduler.sin_port          = htons(op_port);

    pthread_create(&broadcast_thread, NULL, handle_broadcast, &op_port);
    pthread_detach(broadcast_thread);

    scheduler_fd = handle_working_socket(scheduler);
    if (-1 == scheduler_fd)
    {
        errno = EBADF;
        perror("could not establish working socket, exiting ...");
        return EXIT_FAILURE;
    }

    int     client_array[MAX_CLIENTS]   = { 0 };
    int     idx                         = 0;

    while (g_running)
    {
        client_array[idx] = accept(scheduler_fd, (struct sockaddr *)&scheduler, &scheduler_len);
        if (-1 == client_array[idx])
        {
            perror("driver: connection refused");
        }

        handle_submitter_req((void *) &client_array[idx]);
        
        if (MAX_CLIENTS == (idx + 1))
        {
            printf("max clients connected ... ");
            break;
        }

        idx++;
    }
    
    if (false == g_running)
    {
        for (int iter = 0; iter < MAX_CLIENTS; iter++)
        {
            close(client_array[iter]);
        }
    }
    
    close(scheduler_fd);

    return EXIT_SUCCESS;    
}

/*** end of scheduler.c ***/
