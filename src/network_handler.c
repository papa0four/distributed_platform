#include "../includes/network_handler.h"

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

struct addrinfo setup_scheduler (struct addrinfo * hints, int socket_type)
{
    hints->ai_family     = AF_UNSPEC;
    hints->ai_socktype   = socket_type;
    hints->ai_flags      = AI_PASSIVE;

    return *hints;
}

int create_socket (struct addrinfo * hints, char * str_port, int sockopt_flag, int type_flag)
{
    int sock_fd     = 0;
    int reuse       = 1;
    int ret_val     = -1;

    struct addrinfo * serv_info;
    struct addrinfo * p_addr;
    struct timeval timeout;
    timeout.tv_sec = 10;
    timeout.tv_usec = 0;

    if ((ret_val = getaddrinfo(NULL, str_port, hints, &serv_info)) != 0)
    {
        fprintf(stderr, "getaddrinfo() - %s\n", gai_strerror(ret_val));
        return -1;
    }

    for (p_addr = serv_info; p_addr != NULL; p_addr = p_addr->ai_next)
    {
        // if ((sock_fd = socket(p_addr->ai_family, p_addr->ai_socktype, p_addr->ai_protocol)) == -1)
        // {
        //     perror("server: socket");
        //     continue;
        // }

        if (UDP_CONN == type_flag)
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
        }

        if (TCP_CONN == type_flag)
        {
            if ((sock_fd = socket(p_addr->ai_family, p_addr->ai_socktype | SOCK_NONBLOCK, p_addr->ai_protocol)) == -1)
            {
                perror("server: socket");
                continue;
            }

            if (-1 == setsockopt(sock_fd, SOL_SOCKET, sockopt_flag, &timeout, sizeof(timeout)))
            {
                perror("setsockopt()");
                exit(1);
            }
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

    struct pollfd * pfds = calloc(MAX_CLIENTS, sizeof(*pfds));
    if (NULL == pfds)
    {
        errno = ENOMEM;
        fprintf(stderr, "%s could not allocate poll fd array\n", __func__);
        return NULL;
    }
    
    struct addrinfo hints                   = { 0 };
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;
    hints.ai_family = AF_UNSPEC;

    char            broadcast_msg[MAX_BUFF] = { 0 };

    ssize_t     bytes_recv          = 0;
    ssize_t     bytes_sent          = 0;
    int         broadcast_socket    = create_socket(&hints, BROADCAST_PORT, SO_REUSEADDR | SO_BROADCAST, UDP_CONN);
    int         timeout             = 1000;

    pfds[0].fd      = broadcast_socket;
    pfds[0].events  = POLLIN;

    int fd_count = 1;

    while (g_running)
    {
        int poll_count = poll(pfds, fd_count, timeout);
        if (-1 == poll_count)
        {
            CLEAN(pfds);
            close(broadcast_socket);
            return NULL;
        }

        for (int idx = 0; idx < fd_count; idx++)
        {
            if (pfds[idx].revents & POLLIN)
            {
                bytes_recv  = recvfrom(broadcast_socket, broadcast_msg, MAX_BUFF, 0,
                        (struct sockaddr *)&cli_address, &client_len);
                if (-1 == bytes_recv)
                {
                    perror("handle_broadcast - recvfrom");
                    CLEAN(pfds);
                    close(broadcast_socket);
                    return NULL;
                }

                bytes_sent = sendto(broadcast_socket, (char *)p_port, strlen((char *)p_port), 0, 
                                    (struct sockaddr *)&cli_address, client_len);
                if (-1 == bytes_sent)
                {
                    perror("handle_broadcast - sendto");
                    CLEAN(pfds);
                    close(broadcast_socket);
                    return NULL;
                }
            }
        }
    }
    CLEAN(pfds);
    shutdown(broadcast_socket, SHUT_RDWR);
    return NULL;
}

int handle_working_socket (struct addrinfo * hints, char * p_port)
{
    int         scheduler_fd    = 0;
    int         listen_ret      = -1;

    hints->ai_socktype  = SOCK_STREAM;
    hints->ai_flags     = AI_PASSIVE;
    hints->ai_family    = AF_UNSPEC;

    scheduler_fd = create_socket(hints, p_port, SO_REUSEADDR, TCP_CONN);

    listen_ret = listen(scheduler_fd, BACKLOG);
    if (-1 == listen_ret)
    {
        perror("handle_working_socket - listen");
        close(scheduler_fd);
        return -1;
    }

    return scheduler_fd;
}

void handle_worker_connections (int scheduler_fd, struct addrinfo * hints,
                                socklen_t scheduler_len)

{
    int idx             = 0;
    int pthread_ret     = -1;
    int timeout         = 10000;    // equates to 10 seconds
    int fd_count        = 1;
    int fd_size         = MAX_CLIENTS;

    thread_info_t ** p_threads = calloc(MAX_CLIENTS, sizeof(thread_info_t *));
    if (NULL == p_threads)
    {
        errno = ENOMEM;
        fprintf(stderr, "%s could not allocate memory for threads array\n", __func__);
        return;
    }

    struct pollfd * pfds = calloc(fd_size, sizeof(*pfds));
    if (NULL == pfds)
    {
        errno = ENOMEM;
        fprintf(stderr, "%s could not allocate poll fd array\n", __func__);
        CLEAN(p_threads);
        return;
    }

    pfds[0].fd      = scheduler_fd;
    pfds[0].events  = POLLIN;
    int i;

    while (g_running)
    {
        int poll_count = poll(pfds, fd_count, timeout);
        if (-1 == poll_count)
        {
            CLEAN(pfds);
            CLEAN(p_threads);
            shutdown(scheduler_fd, SHUT_RDWR);
            return;
        }

        for (i = 0; i < fd_count; i++)
        {
            if (pfds[i].revents & POLLIN)
            {
                if (pfds[i].fd == scheduler_fd)
                {
                    int new_conn = accept(scheduler_fd, (struct sockaddr *)&hints->ai_addr, &scheduler_len);
                    if (-1 == new_conn)
                    {
                        g_running = false;
                        shutdown(scheduler_fd, SHUT_RDWR);
                        goto CLEANUP;
                    }
                    else
                    {
                        p_threads[idx] = calloc(1, sizeof(thread_info_t));
                        if (NULL == p_threads[idx])
                        {
                            errno = ENOMEM;
                            fprintf(stderr, "%s() - could not allocate memory for thread information struct\n", __func__);
                            CLEAN(pfds);
                            CLEAN(p_threads);
                            close(scheduler_fd);
                            return;
                        }

                        p_threads[idx]->sock = new_conn;
                        num_clients++;

                        pthread_ret = pthread_create(&p_threads[idx]->t_id, NULL,
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
                }
            }
            else
            {
                printf("timeout occurred\n");
                g_running = false;
                goto CLEANUP;
            }
        }
    }

CLEANUP:
    printf("start cleanup\n");
    pthread_cond_broadcast(&condition);
    shutdown(scheduler_fd, SHUT_RDWR);
    for (size_t i = 0; i < MAX_CLIENTS; i++)
    {
        if (p_threads[i])
        {
            pthread_join(p_threads[i]->t_id, NULL);
            CLEAN(p_threads[i]);
            sleep(1);
        }
    }

    CLEAN(pfds);
    CLEAN(p_threads);
    return;
}

/*** end network_handler.c ***/