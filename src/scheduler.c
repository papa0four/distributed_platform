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

ssize_t determine_operation (thread_info_t * p_info)
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
            submitter_payload = recv_and_pack_payload(p_info->sock);
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

    return NULL;
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
        pthread_cond_broadcast(&condition);
        pthread_join(broadcast_thread, NULL);
    }

    close(scheduler_fd);
    destroy_jobs();
    wqueue_destroy();
    CLEAN(pp_jobs);
    return EXIT_SUCCESS;    
}

/*** end of scheduler.c ***/
