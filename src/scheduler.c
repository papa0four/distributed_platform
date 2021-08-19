#define _GNU_SOURCE
#include "../includes/scheduler.h"

void sigint_handler (int signo)
{
    if (SIGINT == signo)
    {
        char msg[MAX_BUFF] = "\nSIGINT (ctrl+c) caught.\nShutting down...";
        fprintf(stderr, "%s\n", msg);
        g_running = false;
    }
}

char * get_port(int argc, char ** argv)
{
    if (1 == argc)
    {
        errno = EINVAL;
        perror("no command line arguments detected, exiting ...");
        return 0;
    }

    int     opt     = 0;
    char  * p_end   = NULL;
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

                port = strtol(argv[optind], &p_end, 10);
                if ((LONG_MIN == port) || (LONG_MAX == port))
                {
                    errno = EINVAL;
                    perror("get_port() - strtol() failed");
                    return NULL;
                }
                if ((MIN_PORT > port) || (MAX_PORT < port))
                {
                    errno = EINVAL;
                    perror("get_port() - invalid port entered");
                    fprintf(stderr, "\tvalid port range: 1024 - 65535\n");
                    return NULL;;
                }
                else if (BROADCAST == port)
                {
                    errno = EINVAL;
                    perror("get_port() - supplied port cannot be broadcast port");
                    return NULL;
                }
                else
                {
                    return argv[optind];
                }
                break;

            case '?':
                errno = EINVAL;
                perror("invalid command line argument passed");
                fprintf(stderr, "only valid cmdline argument:\n\t-p [specified port]\n");
                break;
        }
    }
    return NULL;
}

/**
 * @brief - appropriately sets up the scheduler's information in order to 
 *          initiate conversation with all workers and submitters
 * @param - N/A
 * @return - the packed struct containing the scheduler's information
 */
static struct addrinfo setup_scheduler (struct addrinfo * hints, int socket_type)
{
    hints->ai_family     = AF_UNSPEC;
    hints->ai_socktype   = socket_type;
    hints->ai_flags      = AI_PASSIVE;

    return *hints;
}

/**
 * @brief - handles the initial setup of the socket in which the scheduler will
 *          continuously listen on for the workers and submitters
 * @param scheduler - the sockaddr_in struct containing the scheduler's info
 * @return - the broadcast file descriptor listening for submitters and workers
 *           or -1 on error
 */
static int create_socket (struct addrinfo * hints, char * str_port, int sockopt_flag)
{
    int sock_fd     = 0;
    int reuse       = 1;
    int ret_val     = -1;

    struct addrinfo * serv_info;
    struct addrinfo * p_addr;

    if ((ret_val = getaddrinfo(NULL, str_port, hints, &serv_info)) != 0)
    {
        fprintf(stderr, "getaddrinfo() - %s\n", gai_strerror(ret_val));
        return -1;
    }

    for (p_addr = serv_info; p_addr != NULL; p_addr = p_addr->ai_next)
    {
        if ((sock_fd = socket(p_addr->ai_family, p_addr->ai_socktype, p_addr->ai_protocol)) == -1)
        {
            perror("server: socket");
            continue;
        }

        if (-1 == setsockopt(sock_fd, SOL_SOCKET, sockopt_flag, &reuse, sizeof(reuse)))
        {
            perror("setsockopt()");
            exit(1);
        }

        if (-1 == bind(sock_fd, p_addr->ai_addr, p_addr->ai_addrlen))
        {
            close(sock_fd);
            perror("server: bind");
            continue;
        }

        break;
    }

    freeaddrinfo(serv_info);

    return sock_fd;
}

void * handle_broadcast (void * p_port)
{
    struct sockaddr_storage cli_address     = { 0 };
    socklen_t       client_len              = sizeof(cli_address);
    
    struct addrinfo hints                   = { 0 };
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;
    hints.ai_family = AF_UNSPEC; // for ipv4

    char            broadcast_msg[MAX_BUFF] = { 0 };

    ssize_t     bytes_recv          = 0;
    ssize_t     bytes_sent          = 0;
    int         broadcast_socket    = create_socket(&hints, BROADCAST_PORT, SO_REUSEADDR | SO_BROADCAST);

    while (g_running)
    {
        bytes_recv  = recvfrom(broadcast_socket, broadcast_msg, MAX_BUFF, 0,
                        (struct sockaddr *)&cli_address, &client_len);
        if (-1 == bytes_recv)
        {
            perror("handle_broadcast - recvfrom");
            close(broadcast_socket);
            return NULL;
        }

        bytes_sent = sendto(broadcast_socket, (char *)p_port, strlen((char *)p_port), 0, 
                            (struct sockaddr *)&cli_address, client_len);
        if (-1 == bytes_sent)
        {
            perror("handle_broadcast - sendto");
            close(broadcast_socket);
            return NULL;
        }
    }

    close(broadcast_socket);
    return NULL;
}

uint32_t unpack_header (thread_info_t * p_info)
{
    if (-1 == p_info->sock)
    {
        errno = EBADF;
        perror("unpack_header - invalid file descriptor passed");
        return ERR;
    }

    ssize_t    bytes_recv  = 0;
    header_t   p_hdr;

    bytes_recv = recv(p_info->sock, &p_hdr, sizeof(header_t), 0);
    if (-1 == bytes_recv)
    {
        perror("version recv");
        return ERR;
    }

    p_hdr.version = ntohl(p_hdr.version);
    p_hdr.operation = ntohl(p_hdr.operation);
    p_info->operation = p_hdr.operation;

    return p_hdr.operation;
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
    if (-1 == bytes_recv)
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
    if (num_iters < 1)
    {
        perror("default iterations is 1");
        return 1;
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
            CLEAN(p_items);
            return NULL;
        }
        item = ntohl(item);
        memcpy(&p_items[idx].item, &item, sizeof(uint32_t));
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

    p_unpacked = pack_payload_struct(num_operations, p_ops, num_iters,
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
static int handle_working_socket (struct addrinfo * hints, char * p_port)
{
    int         scheduler_fd    = 0;
    int         listen_ret      = -1;

    hints->ai_socktype = SOCK_STREAM;
    hints->ai_flags = AI_PASSIVE;
    hints->ai_family = AF_UNSPEC;

    scheduler_fd = create_socket(hints, p_port, SO_REUSEADDR);

    listen_ret = listen(scheduler_fd, BACKLOG);
    if (-1 == listen_ret)
    {
        perror("handle_working_socket - listen");
        close(scheduler_fd);
        return -1;
    }

    return scheduler_fd;
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

void recv_computation (int worker_conn)
{
    if (-1 == worker_conn)
    {
        errno = EINVAL;
        perror("recv_computation - buffer is NULL and/or file descriptor is invalid");
        return;
    }

    ssize_t bytes_recv = -1;
    int answer = -1;

    bytes_recv = recv(worker_conn, &answer, sizeof(int), 0);
    if (-1 == bytes_recv)
    {
        return;
    }
    answer = ntohl(answer);
    recv_answer(worker_conn, answer);
}

static ssize_t determine_operation (thread_info_t * p_info)
{
    if (NULL == p_info)
    {
        return -1;
    }
    if (-1 == p_info->sock)
    {
        errno = EINVAL;
        perror("determine operation - file descriptor passed is invalid");
        return -1;
    }

    uint32_t operation = unpack_header(p_info);
    if (ERR == operation)
    {
        fprintf(stderr, "%s header is NULL\n", __func__);
        return -1;
    }

    subjob_payload_t * submitter_payload = NULL;
    job_t            * p_new_job         = NULL;
    ssize_t            populate_ret      = -1;
    ssize_t            bytes_sent        = -1;
    uint32_t           job_id;
    int                shutdown          = SHUTDOWN;

    switch (operation)
    {
        case SUBMIT_JOB:
            submitter_payload = unpack_payload(p_info->sock);
            if (NULL == submitter_payload)
            {
                fprintf(stderr, "could not recv submitter request\n");
                CLEAN(submitter_payload);
                return -1;
            }

            p_new_job = create_job(submitter_payload);
            if (NULL == p_new_job)
            {
                fprintf(stderr, "could not create a new job\n");
                CLEAN(submitter_payload);
                return -1;
            }

            populate_ret = populate_jobs_and_queue(p_new_job);
            if (-1 == populate_ret)
            {
                fprintf(stderr, "could not add job to list or populate queue\n");
                CLEAN(submitter_payload);
                return -1;
            }
            job_id = htonl(p_new_job->job_id);
            bytes_sent = send(p_info->sock, &job_id, sizeof(uint32_t), 0);
            if (-1 == bytes_sent)
            {
                fprintf(stderr, "could not send job_id\n");
            }
            CLEAN(submitter_payload->items);
            CLEAN(submitter_payload->op_groups);
            CLEAN(submitter_payload);
            break;

        case QUERY_WORK:
            send_task_to_worker(p_info->sock);
            break;

        case SUBMIT_WORK:
            recv_computation(p_info->sock);
            break;

        case SHUTDOWN:
            pthread_mutex_lock(&running_mutex);
            bytes_sent = send(p_info->sock, &shutdown, sizeof(int), 0);
            if (0 >= bytes_sent)
            {
                fprintf(stderr, "could not send shutdown command to worker\n");
            }
            g_running = false;
            pthread_mutex_unlock(&running_mutex);
            close(p_info->sock);
            break;

        default:
            fprintf(stderr, "invalid operation received\n");
            CLEAN(p_new_job);
            return -1;
    }

    return 0;
}

void * worker_func (void * p_info)
{
    if (NULL == p_info)
    {
        return NULL;
    }
    thread_info_t ** p_thread_info = (thread_info_t **) &p_info;
    if (NULL == p_thread_info)
    {
        CLEAN(p_info);
        return NULL;
    }
    if (-1 == (*p_thread_info)->sock)
    {
        errno = EBADF;
        perror("worker file descriptor passed is invalid");
        return NULL;
    }

    ssize_t det_op_ret = -1;

    while (g_running)
    {
        det_op_ret = determine_operation(*p_thread_info);
        if ((0 == (*p_thread_info)->operation) && (0 == det_op_ret))
        {
            pthread_cancel((*p_thread_info)->t_id);
        }

        if (-1 == det_op_ret)
        {
            break;
        }
    }

    CLEAN(p_info);
    return NULL;
}

void handle_worker_connections (int scheduler_fd, struct addrinfo * hints,
                                socklen_t scheduler_len)

{
    int idx             = 0;
    int pthread_ret     = -1;
    thread_info_t ** p_threads = calloc(MAX_CLIENTS, sizeof(thread_info_t *));

    while (g_running)
    {
        p_threads[idx] = calloc(1, sizeof(thread_info_t));
        if (NULL == p_threads[idx])
        {
            errno = ENOMEM;
            fprintf(stderr, "%s() - could not allocate memory for thread information struct\n", __func__);
            CLEAN(p_threads);
            return;
        }

        pthread_attr_init(&p_threads[idx]->attr);
        p_threads[idx]->sock = accept(scheduler_fd, (struct sockaddr *)&hints->ai_addr, &scheduler_len);
        if (-1 == p_threads[idx]->sock)
        {
            g_running = false;
            close(scheduler_fd);
            CLEAN(p_threads[idx]);
            break;
        }

        num_clients++;
        
        pthread_attr_setdetachstate(&p_threads[idx]->attr, PTHREAD_CREATE_DETACHED);
        pthread_ret = pthread_create(&p_threads[idx]->t_id, &p_threads[idx]->attr,
                    worker_func, (void *)p_threads[idx]);
        if (0 != pthread_ret)
        {
            fprintf(stderr, "could not create worker thread\n");
            close(p_threads[idx]->sock);
            CLEAN(p_threads[idx]);
            break;
        }

        worker_threads[idx] = p_threads[idx]->t_id;

        if (MAX_CLIENTS == (num_clients + 1))
        {
            printf("max clients connected ... ");
            break;
        }
        idx++;
    }

    for (size_t i = 0; i < MAX_CLIENTS; i++)
    {
        if (p_threads[i])
        {
            pthread_cancel(p_threads[i]->t_id);
            CLEAN(p_threads[i]);
            sleep(1);
        }
    }
    CLEAN(p_threads);
    return;
}

int main (int argc, char ** argv)
{
    struct sigaction handle_sigint = { 0 };
    handle_sigint.sa_handler       = sigint_handler;
    sigfillset(&handle_sigint.sa_mask);
    handle_sigint.sa_flags         = 0;
    sigaction(SIGINT, &handle_sigint, NULL);

    char      * op_port             = NULL;
    int         scheduler_fd        = 0;
    int         pthread_ret         = -1;
    ssize_t     init_globals_ret    = -1;

    pthread_t   broadcast_thread;
    
    op_port = get_port(argc, argv);
    if (NULL == op_port)
    {
        perror("get_port");
        return EXIT_FAILURE;
    }

    init_globals_ret = initialize_global_data();
    if (-1 == init_globals_ret)
    {
        close(scheduler_fd);
        return EXIT_FAILURE;
    }

    pthread_ret = pthread_create(&broadcast_thread, NULL, handle_broadcast, op_port);
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

    usleep(3000);

    struct addrinfo scheduler = { 0 };
    scheduler = setup_scheduler(&scheduler, SOCK_STREAM);

    scheduler_fd = handle_working_socket(&scheduler, op_port);
    if (-1 == scheduler_fd)
    {
        errno = EBADF;
        perror("could not establish working socket, exiting ...");
        return EXIT_FAILURE;
    }

    handle_worker_connections(scheduler_fd, &scheduler, scheduler.ai_addrlen);

    if (false == g_running)
    {
        pthread_cancel(broadcast_thread);
        // for (int i = 0; i < MAX_CLIENTS; i++)
        // {
        //     // if (worker_threads[i])
        //     // {
        //     //     pthread_cancel(worker_threads[i]);
        //     //     sleep(1);
        //     // }
        //     pthread_cancel(worker_threads[i]);
        //     usleep(500);
        // }
    }

    close(scheduler_fd);
    destroy_jobs();
    wqueue_destroy();
    CLEAN(pp_jobs);
    return EXIT_SUCCESS;    
}

/*** end of scheduler.c ***/
