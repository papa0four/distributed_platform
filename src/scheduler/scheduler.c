#include "../includes/scheduler.h"
#include "../includes/operations.h"

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
                    return 0;
                }

                port = strtol(argv[optind], &ptr, BASE_10);
                if ((MIN_PORT > port) || (MAX_PORT < port))
                {
                    errno = EINVAL;
                    perror("invalid port entered");
                    fprintf(stderr, "\tvalid port range: 31338 - 65535\n");
                    return 0;
                }
                port = (uint16_t)port;
                break;

            case '?':
                errno = EINVAL;
                perror("invalid command line argument passed");
                fprintf(stderr, "only valid cmdline argument:\n\t-p [specified port]\n");
                break;
        }
    }
    return port;
}

int main (int argc, char ** argv)
{
    struct sockaddr_in scheduler = { 0 };
    struct sockaddr_in submitter = { 0 };
    socklen_t          scheduler_len;
    socklen_t          submitter_len = sizeof(submitter);

    scheduler.sin_family        = AF_INET;
    scheduler.sin_addr.s_addr   = INADDR_ANY;
    scheduler.sin_port          = htons(BROADCAST_PORT);
    scheduler_len               = sizeof(scheduler);

    int         broadcast_socket    = 0;
    int         reuse               = 1;
    int         bind_ret            = -1;
    int         opt_ret             = -1;
    int         list_ret            = -1;
    uint16_t    op_port             = 1;
    ssize_t     bytes_recv          = 0;
    ssize_t     bytes_sent          = 0;

    broadcast_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (0 == broadcast_socket)
    {
        perror("socket");
        return EXIT_FAILURE;
    }

    opt_ret = setsockopt(broadcast_socket, SOL_SOCKET, SO_REUSEADDR | SO_BROADCAST, &reuse, sizeof(reuse));
    if (-1 == opt_ret)
    {
        perror("setsockopt");
        close(broadcast_socket);
        return EXIT_FAILURE;
    }

    char broadcast_msg[MAX_BUFF] = { 0 };

    bind_ret = bind(broadcast_socket, (const struct sockaddr *)&scheduler, scheduler_len);
    if (0 > bind_ret)
    {
        perror("bind");
        return EXIT_FAILURE;
    }

    bytes_recv = recvfrom(broadcast_socket, broadcast_msg, MAX_BUFF, 0,
                         (struct sockaddr *)&submitter, &submitter_len);
    if (0 > bytes_recv)
    {
        perror("recvfrom");
        close(broadcast_socket);
        return EXIT_FAILURE;
    }

    printf("broadcast_msg recv: %s\n", broadcast_msg);
    op_port = get_port(argc, argv);
    if (0 == op_port)
    {
        close(broadcast_socket);
        return EXIT_FAILURE;
    }
    printf("port generated: %hu\n", op_port);
    op_port = htons(op_port);

    bytes_sent = sendto(broadcast_socket, &op_port, sizeof(uint16_t), 0, 
                        (struct sockaddr *)&submitter, submitter_len);
    if (0 >= bytes_sent)
    {
        perror("sendto");
        close(broadcast_socket);
        return EXIT_FAILURE;
    }
    printf("scheduler sent %ld bytes\n", bytes_sent);
    close(broadcast_socket);

    return EXIT_SUCCESS;
}

