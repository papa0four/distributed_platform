#include "../includes/scheduler.h"
#include "../includes/operations.h"

volatile bool g_running = true;

static uint16_t get_port(int argc, char ** argv)
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

static void * handle_broadcast (void * p_port)
{
    struct sockaddr_in scheduler    = { 0 };
    struct sockaddr_in client       = { 0 };

    memset((void *)&scheduler, 0, sizeof(scheduler));
    scheduler.sin_family            = AF_INET;
    scheduler.sin_addr.s_addr       = INADDR_ANY;
    scheduler.sin_port              = htons(BROADCAST_PORT);

    socklen_t scheduler_len   = sizeof(scheduler);
    socklen_t client_len      = sizeof(client);

    int         broadcast_socket    = 0;
    int         reuse               = 1;
    int         bind_ret            = -1;
    int         opt_ret             = -1;
    int         select_ret          = -1;
    ssize_t     bytes_recv          = 0;
    ssize_t     bytes_sent          = 0;
    fd_set      read_fd;

    broadcast_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (0 == broadcast_socket)
    {
        perror("socket");
        return NULL;
    }

    opt_ret = setsockopt(broadcast_socket, SOL_SOCKET, SO_REUSEADDR | SO_BROADCAST, &reuse, sizeof(reuse));
    if (-1 == opt_ret)
    {
        perror("handle_broadcast: setsockopt");
        close(broadcast_socket);
        return NULL;
    }

    char broadcast_msg[MAX_BUFF] = { 0 };

    bind_ret = bind(broadcast_socket, (const struct sockaddr *)&scheduler, scheduler_len);
    if (0 > bind_ret)
    {
        perror("bind");
        return NULL;
    }

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
                perror("recvfrom");
                close(broadcast_socket);
                return NULL;
            }

            printf("broadcast_msg recv'd from: %s:%hu\n", 
                    inet_ntoa(client.sin_addr), ntohs(client.sin_port));

            bytes_sent = sendto(broadcast_socket, op_port, sizeof(uint16_t), 0, 
                                (struct sockaddr *)&client, client_len);
            if (0 >= bytes_sent)
            {
                perror("sendto");
                close(broadcast_socket);
                return NULL;
            }
            printf("operational port sent: %hu\n", ntohs(*op_port));
            printf("scheduler sent %ld bytes\n", bytes_sent);
        }
        else
        {
            break;
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

int main (int argc, char ** argv)
{
    uint16_t    op_port         = 0;
    int         scheduler_fd    = 0;
    int         opt_ret         = 0;
    int         reuse           = 1;
    int         bind_ret        = -1;
    int         listen_ret      = -1;
    int         accept_ret      = -1;
    ssize_t     bytes_recv      = 0;
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

    scheduler_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (0 == scheduler_fd)
    {
        perror("driver: socket");
        return EXIT_FAILURE;
    }

    opt_ret = setsockopt(scheduler_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
    if (-1 == opt_ret)
    {
        perror("driver: setsockopt");
        close(scheduler_fd);
        return EXIT_FAILURE;
    }

    bind_ret = bind(scheduler_fd, (const struct sockaddr *)&scheduler, scheduler_len);
    if (0 > bind_ret)
    {
        perror("driver: bind");
        close(scheduler_fd);
        return EXIT_FAILURE;
    }

    listen_ret = listen(scheduler_fd, BACKLOG);
    if (-1 == listen_ret)
    {
        perror("driver: listen");
        close(scheduler_fd);
        return EXIT_FAILURE;
    }

    accept_ret = accept(scheduler_fd, (struct sockaddr *)&scheduler, &scheduler_len);
    if (-1 == accept_ret)
    {
        perror("driver: accept");
        close(scheduler_fd);
        return EXIT_FAILURE;
    }

    char worker_buff[MAX_BUFF] = { 0 };
    bytes_recv = recv(accept_ret, worker_buff, MAX_BUFF, 0);
    printf("worker_buff: %s\n", worker_buff);
    if (0 >= bytes_recv)
    {
        perror("driver: recv");
        close(scheduler_fd);
        return EXIT_FAILURE;
    }

    close(scheduler_fd);

    return EXIT_SUCCESS;    
}

/*** end of scheduler.c ***/
