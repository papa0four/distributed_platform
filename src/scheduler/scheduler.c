#define _GNU_SOURCE

#include "includes/scheduler.h"

void sigint_handler (int signo)
{
    if (SIGINT == signo)
    {
        char msg[MAX_BUFF] = "\nSIGINT (ctrl+c) caught.\nShutting down...";
        fprintf(stderr, "%s\n", msg);
        g_running = false;
        destroy_jobs();
        wqueue_destroy();
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

/**
 * @brief - appropriately sets up the scheduler's information in order to 
 *          initiate conversation with all workers and submitters
 * @param - N/A
 * @return - the packed struct containing the scheduler's information
 */
static struct sockaddr_in setup_scheduler ()
{
    struct sockaddr_in scheduler = { 0 };
    memset((void *)&scheduler, 0, sizeof(scheduler));

    scheduler.sin_family            = AF_INET;
    scheduler.sin_addr.s_addr       = INADDR_ANY;
    scheduler.sin_port              = htons(BROADCAST_PORT);

    return scheduler;
}

/**
 * @brief - handles the initial setup of the socket in which the scheduler will
 *          continuously listen on for the workers and submitters
 * @param scheduler - the sockaddr_in struct containing the scheduler's info
 * @return - the broadcast file descriptor listening for submitters and workers
 *           or -1 on error
 */
static int create_broadcast_socket (struct sockaddr_in scheduler)
{
    int         broadcast_socket    = 0;
    int         reuse               = 1;
    int         bind_ret            = -1;
    int         opt_ret             = -1;
    socklen_t   scheduler_len       = sizeof(scheduler);

    broadcast_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (0 == broadcast_socket)
    {
        perror("create_broadcast_socket - socket");
        return -1;
    }

    opt_ret = setsockopt(broadcast_socket, SOL_SOCKET, SO_REUSEADDR | SO_BROADCAST, &reuse, sizeof(reuse));
    if (-1 == opt_ret)
    {
        perror("create_broadcast_socket - setsockopt");
        close(broadcast_socket);
        return -1;
    }

    bind_ret = bind(broadcast_socket, (const struct sockaddr *)&scheduler, scheduler_len);
    if (0 > bind_ret)
    {
        perror("create_broadcast_socket - bind");
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

            // printf("broadcast_msg recv'd from: %s:%hu\n", 
            //         inet_ntoa(client.sin_addr), ntohs(client.sin_port));

            bytes_sent = sendto(broadcast_socket, op_port, sizeof(uint16_t), 0, 
                                (struct sockaddr *)&client, client_len);
            if (0 >= bytes_sent)
            {
                perror("handle_broadcast - sendto");
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
    // printf("version recv'd: %u\n", ntohl(version));
    if (0 > bytes_recv)
    {
        perror("version recv");
        return NULL;
    }
    if (VERSION != ntohl(version))
    {
        return NULL;
    }

    bytes_recv = recv(client_conn, &operation, sizeof(uint32_t), 0);
    if (0 > bytes_recv)
    {
        perror("operation recv");
        return NULL;
    }

    if ((SUBMIT_JOB != ntohl(operation)) &&
        (QUERY_WORK != ntohl(operation)) &&
        (SUBMIT_WORK != ntohl(operation)) &&
        (SHUTDOWN != ntohl(operation)))
    {
        perror("packet operation protocol invalid");
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

/**
 * @brief - reads the value of num_operations within the packet sent from
 *          submitter and stores it within the submit job payload struct
 * @param client_conn - the file descriptor to the client's connection
 * @return - an appropriately converted 32-bit unsigned int containing the
 *           length of the opchain, 0 on error or empty opchain
 */
static uint32_t recv_num_operations (int client_conn)
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
    if (0 > bytes_recv)
    {
        perror("num_operations recv");
        return 0;
    }
    num_operations = ntohl(num_operations);
    return num_operations;
}

/**
 * @brief - receives the chain of operations from the packet sent from the
 *          submitter and stores it within the submit job payload struct
 * @param client_conn - the file descriptor to the client's connection
 * @param num_ops - the total length of the opchain
 * @return - the list of operations where each index stores a pair of 
 *           operations to perform on each item or NULL on error
 */
static opchain_t * recv_opchain (int client_conn, uint32_t num_ops)
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
            clean_memory(p_ops);
            return NULL;
        }

        bytes_recv = recv(client_conn, &operand, sizeof(uint32_t), 0);
        if (0 >= bytes_recv)
        {
            perror("operand recv");
            clean_memory(p_ops);
            return NULL;
        }
        operation = ntohl(operation);
        operand   = ntohl(operand);
        memcpy(&p_ops[idx].operation, &operation, sizeof(uint32_t));
        memcpy(&p_ops[idx].operand, &operand, sizeof(uint32_t));

        // printf("operation: %u\toperand: %u\n", p_ops[idx].operation, p_ops[idx].operand);
    }

    return p_ops;
}

/**
 * @brief - receives the number of work iterations to be performed on each item
 *          NOTE: this value may only be 1, anything else is invalid
 * @param client_conn - the file descriptor to the client's connection
 * @return - an appropriately converted 32-bit unsigned int containing the 
 *           number of iterations in which to perform the work on each item
 *           or returns 0 on error
 */
static uint32_t recv_iterations (int client_conn)
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
        perror("iterations to perform may only be 1");
        return 0;
    }

    return num_iters;
}

/**
 * @brief - receives the total length of the items list from the packet
 *          submitted and appropriately stores the data within the 
 *          submit job payload struct
 * @param client_conn - the file descriptor to the client's connection
 * @return - an appropriately converted 32-bit unsigned int containing the
 *           total length of the items list or 0 on error and empty lists
 */
static uint32_t recv_num_items (int client_conn)
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

/**
 * @brief - iterates over the list of items within the packet submitted and
 *          stores them within the items list in the submit job payload struct
 * @param client_conn - the file descriptor to the client's connection
 * @param num_items - the total length of the items list
 * @return - a pointer to the items list or NULL on error
 */
static item_t * recv_items (int client_conn, uint32_t num_items)
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
        // printf("item: %u\n", p_items[idx].item);
    }

    return p_items;
}

/**
 * @brief - received all appropriate data in order to obtain all submit
 *          job payload data and copies each piece of data into it's appropriate
 *          location within the submit job payload struct
 * @param num_operations - the total length of the chain of operations
 * @param p_ops - a pointer to the chain of operations list
 * @param num_iters - the number of iterations to perform work on each item
 *                    NOTE: this will always be 1
 * @param num_items - the total length of the items list
 * @param p_items - the list of items to perform each operation on
 * @return - a pointer to the submit job payload struct containing all
 *           appropriate data received from the submitter or NULL on error
 */
static subjob_payload_t * pack_payload_struct (uint32_t num_operations, opchain_t * p_ops,
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
    clean_memory(p_items);
    clean_memory(p_ops);
    return p_payload;

CLEANUP:
    clean_memory(p_payload);
ERROR:
    clean_memory(p_ops);
    clean_memory(p_items);
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
    subjob_payload_t * p_unpacked          = NULL;

    num_operations = recv_num_operations(client_conn);
    if (0 == num_operations)
    {
        fprintf(stderr, "no operations recv\n");
        goto ERROR;
    }
    // printf("number of operations: %u\n", num_operations);

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
        clean_memory(p_ops);
        goto ERROR;
    }

    num_items = recv_num_items(client_conn);
    if (0 == num_items)
    {
        perror("could not recv number of items");
        clean_memory(p_ops);
        goto ERROR;
    }
    // printf("number of items: %u\n", num_items);

    p_items = recv_items(client_conn, num_items);
    if (NULL == p_items)
    {
        perror("could not recv items");
        clean_memory(p_ops);
        goto ERROR;
    }

    p_unpacked = pack_payload_struct (num_operations, p_ops, num_iters,
                        num_items, p_items);
    if (NULL == p_unpacked)
    {
        perror("could not pack payload struct");
        p_unpacked = NULL;
    }

ERROR:
    return p_unpacked;    
}

/**
 * @brief - handles the initial setup of the socket in which the scheduler will
 *          continuously listen on for both workers and submitters
 * @param scheduler - the schedulers address information for socket creation
 * @return - the socket's file descriptor or -1 on error
 */
static int handle_working_socket (struct sockaddr_in scheduler)
{
    int         scheduler_fd    = 0;
    int         opt_ret         = 0;
    int         reuse           = 1;
    int         bind_ret        = -1;
    int         listen_ret      = -1;

    scheduler_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (0 == scheduler_fd)
    {
        perror("handle_working_socket - socket");
        return -1;
    }

    opt_ret = setsockopt(scheduler_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
    if (-1 == opt_ret)
    {
        perror("handle_working_socket - setsockopt");
        close(scheduler_fd);
        return -1;
    }

    bind_ret = bind(scheduler_fd, (const struct sockaddr *)&scheduler, sizeof(scheduler));
    if (0 > bind_ret)
    {
        perror("handle_working_socket - bind");
        close(scheduler_fd);
        return -1;
    }

    listen_ret = listen(scheduler_fd, BACKLOG);
    if (-1 == listen_ret)
    {
        perror("handle_working_socket - listen");
        close(scheduler_fd);
        return -1;
    }

    return scheduler_fd;
}

void print_opchain (opchain_t * p_chain)
{
    if (NULL == p_chain)
    {
        errno = EINVAL;
        perror("operation chain passed is NULL");
        return;
    }
    switch(p_chain->operation)
    {
        case ADD:
            printf("+%u ", p_chain->operand);
            break;

        case SUBR:
            printf("-%u ", p_chain->operand);
            break;

        case SUBL:
            printf("%u- ", p_chain->operand);
            break;

        case AND:
            printf("&%u ", p_chain->operand);
            break;

        case OR:
            printf("|%u ", p_chain->operand);
            break;

        case XOR:
            printf("^%u ", p_chain->operand);
            break;

        case NOT:
            printf("~ ");
            break;

        case ROLR:
            printf("=>>%u ", p_chain->operand);
            break;

        case ROLL:
            printf("=<<%u ", p_chain->operand);
            break;

        default:
            fprintf(stderr, "invalid operation/operand combination found\n");
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

void send_task_to_worker (int worker_fd)
{
    work_t * p_work = dequeue_work();
    if (NULL == p_work)
    {
        fprintf(stderr, "could not retrieve task\n");
        return;
    }
    p_work->worker_sock = worker_fd;

    const job_t * p_job         = find_job(p_work->job_id);
    uint32_t      num_ops       = p_job->num_operations;
    uint32_t      offset        = (sizeof(uint32_t) * 2);
    ssize_t       bytes_sent    = -1;

    char work_buffer[MAX_BUFF] = { 0 };
    memcpy(work_buffer, &p_work->item, sizeof(uint32_t));
    memcpy((work_buffer + sizeof(uint32_t)),
            &num_ops, sizeof(uint32_t));
    for (size_t idx = 0; idx < p_job->num_operations; idx++)
    {
        memcpy((work_buffer + offset), 
                &p_work->p_chain[idx].operation, sizeof(uint32_t));
        offset += sizeof(uint32_t);

        memcpy((work_buffer + offset), 
                &p_work->p_chain[idx].operand, sizeof(uint32_t));
        offset += sizeof(uint32_t);
    }
    // offset += sizeof(uint32_t);
    memcpy((work_buffer + offset), &p_work->iterations, sizeof(uint32_t));
    uint32_t pack_sz = ((sizeof(uint32_t)) + (sizeof(uint32_t)) +
                        ((num_ops * sizeof(uint32_t)) * num_ops) + 
                        sizeof(uint32_t));
    bytes_sent = send(worker_fd, work_buffer, pack_sz, 0);
    if (0 >= bytes_sent)
    {
        fprintf(stderr, "could not send work packet to worker\n");
        return;
    }

}

void recv_computation (int worker_conn)
{
    if (-1 == worker_conn)
    {
        errno = EINVAL;
        perror("recv_computation - buffer is NULL and/or file descriptor is invalid");
        return;
    }

    ssize_t bytes_recv = -1;
    int32_t answer = -1;

    bytes_recv = recv(worker_conn, &answer, sizeof(int32_t), 0);
    if (0 >= bytes_recv)
    {
        return;
    }
    recv_answer(worker_conn, answer);

}

static ssize_t determine_operation (int client_sock)
{
    if (-1 == client_sock)
    {
        errno = EINVAL;
        perror("determine operation - file descriptor passed is invalid");
        return -1;
    }

    header_t * p_hdr = unpack_header(client_sock);
    if (NULL == p_hdr)
    {
        return -1;
    }

    if (VERSION != p_hdr->version)
    {
        return -1;
    }

    subjob_payload_t * submitter_payload = NULL;
    job_t            * p_new_job         = NULL;
    ssize_t            populate_ret      = -1;
    ssize_t            bytes_sent        = -1;
    uint32_t           job_id;
    int                shutdown          = SHUTDOWN;

    switch (p_hdr->operation)
    {
        case SUBMIT_JOB:
            submitter_payload = unpack_payload(client_sock);
            if (NULL == submitter_payload)
            {
                fprintf(stderr, "could not recv submitter request\n");
                clean_memory(submitter_payload);
                return -1;
            }

            p_new_job = create_job(submitter_payload);
            if (NULL == p_new_job)
            {
                fprintf(stderr, "could not create a new job\n");
                clean_memory(submitter_payload);
                return -1;
            }

            populate_ret = populate_jobs_and_queue(p_new_job);
            if (-1 == populate_ret)
            {
                fprintf(stderr, "could not add job to list or populate queue\n");
            }
            job_id = ntohl(p_new_job->job_id);
            bytes_sent = send(client_sock, &job_id, sizeof(uint32_t), 0);
            if (0 >= bytes_sent)
            {
                fprintf(stderr, "could not send job_id\n");
            }
            clean_memory(submitter_payload->items);
            clean_memory(submitter_payload->op_groups);
            clean_memory(submitter_payload);
            clean_memory(p_hdr);
            break;

        case QUERY_WORK:
            clean_memory(p_hdr);
            send_task_to_worker(client_sock);
            break;

        case SUBMIT_WORK:
            clean_memory(p_hdr);
            recv_computation(client_sock);
            break;

        case SHUTDOWN:
            pthread_mutex_lock(&running_mutex);
            bytes_sent = send(client_sock, &shutdown, sizeof(int), 0);
            if (0 >= bytes_sent)
            {
                fprintf(stderr, "could not send shutdown command to worker\n");
            }
            g_running = false;
            pthread_mutex_unlock(&running_mutex);
            destroy_jobs();
            wqueue_destroy();
            close(client_sock);
            clean_memory(p_hdr);
            break;

        default:
            fprintf(stderr, "invalid operation received\n");
            clean_memory(p_new_job);
            clean_memory(p_hdr);
            return -1;
    }
    return 0;
}

void * worker_func (void * worker_conn)
{
    if (-1 == *(int *)worker_conn)
    {
        errno = EBADF;
        perror("worker file descriptor passed is invalid");
        return NULL;
    }
    int worker_sock = *(int *) worker_conn;
    ssize_t det_op_ret = -1;

    while (g_running)
    {
        det_op_ret = determine_operation(worker_sock);
        if (-1 == det_op_ret)
        {
            break;
        }
    }
    
    close(worker_sock);
    return NULL;
}

void handle_worker_connections (int scheduler_fd, struct sockaddr_in scheduler,
                                socklen_t scheduler_len)

{

    int         new_connection              = 0;
    int         num_clients                 = -1;
    pthread_t   worker_threads[MAX_CLIENTS] = { 0 };
    int         idx                         = 0;
    int         pthread_ret                 = -1;
    int         select_ret                  = -1;
    fd_set      read_fd;
    struct      timeval tv;
    
    while (g_running)
    {
        FD_ZERO(&read_fd);
        FD_SET(scheduler_fd, &read_fd);
        tv.tv_sec   = TV_TIMEOUT;
        tv.tv_usec  = 0;

        select_ret = select(scheduler_fd + 1, &read_fd, NULL, NULL, &tv);
        if (-1 == select_ret)
        {
            fprintf(stderr, "error on select call\n");
        }
        else if ((0 < select_ret) && (FD_ISSET(scheduler_fd, &read_fd)))
        {
            new_connection = accept(scheduler_fd, (struct sockaddr *)&scheduler, &scheduler_len);
            if (-1 == new_connection)
            {
                perror("driver: connection refused");
            }

            num_clients++;
            
            if (g_running)
            {
                pthread_ret = pthread_create(&worker_threads[idx], NULL,
                            worker_func, &new_connection);
                if (0 != pthread_ret)
                {
                    fprintf(stderr, "could not create worker thread\n");
                    close(new_connection);
                }

                pthread_ret = pthread_detach(worker_threads[idx]);
                if (0 != pthread_ret)
                {
                    fprintf(stderr, "could not detach worker thread\n");
                    close(new_connection);
                }
            }
            
            if (MAX_CLIENTS == (num_clients + 1))
            {
                printf("max clients connected ... ");
                break;
            }
        }
        else
        {
            if (false == g_running)
            {
                close(new_connection);
                break;
            }
        }
    }
}

int main (int argc, char ** argv)
{
    struct sigaction handle_sigint = { 0 };
    handle_sigint.sa_handler       = sigint_handler;
    sigfillset(&handle_sigint.sa_mask);
    handle_sigint.sa_flags         = 0;
    sigaction(SIGINT, &handle_sigint, NULL);

    uint16_t    op_port             = 0;
    int         scheduler_fd        = 0;
    int         pthread_ret         = -1;
    ssize_t     init_globals_ret    = -1;

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

    pthread_ret = pthread_create(&broadcast_thread, NULL, handle_broadcast, &op_port);
    if (0 != pthread_ret)
    {
        fprintf(stderr, "could not setup broadcast thread\n");
        return EXIT_FAILURE;
    }
    
    pthread_ret = pthread_detach(broadcast_thread);
    if (0 != pthread_ret)
    {
        fprintf(stderr, "could not detach broadcast thread\n");
        return EXIT_FAILURE;
    }

    scheduler_fd = handle_working_socket(scheduler);
    if (-1 == scheduler_fd)
    {
        errno = EBADF;
        perror("could not establish working socket, exiting ...");
        return EXIT_FAILURE;
    }

    init_globals_ret = initialize_global_data();
    if (-1 == init_globals_ret)
    {
        close(scheduler_fd);
        return EXIT_FAILURE;
    }
    
    handle_worker_connections(scheduler_fd, scheduler, scheduler_len);
    
    close(scheduler_fd);

    return EXIT_SUCCESS;    
}

/*** end of scheduler.c ***/
