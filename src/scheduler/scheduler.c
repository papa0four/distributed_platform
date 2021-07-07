#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/un.h>
#include <arpa/inet.h>

#define BROADCAST_PORT 31337
#define WORK_PORT
#define MAX_BUFF       1024
#define HOSTNAME_LEN   253

int get_port()
{
    int min     = 31338;
    int max     = 65535;

    srand(time(NULL));
    return ((rand() % (max - min + 1)) + min);
}

int main ()
{
    struct sockaddr_in scheduler = { 0 };
    struct sockaddr_in submitter = { 0 };
    socklen_t          scheduler_len;
    socklen_t          submitter_len = sizeof(submitter);

    scheduler.sin_family        = AF_INET;
    scheduler.sin_addr.s_addr   = INADDR_ANY;
    scheduler.sin_port          = htons(BROADCAST_PORT);
    scheduler_len               = sizeof(scheduler);

    int     sockfd                  = 0;
    int     reuse                   = 1;
    int     bind_ret                = -1;
    int     opt_ret                 = -1;
    int     list_ret                = -1;
    ssize_t bytes_recv              = 0;
    ssize_t bytes_sent              = 0;

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (0 == sockfd)
    {
        perror("socket");
        return EXIT_FAILURE;
    }

    opt_ret = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR | SO_BROADCAST, &reuse, sizeof(reuse));
    if (-1 == opt_ret)
    {
        perror("setsockopt");
        close(sockfd);
        return EXIT_FAILURE;
    }

    char msg[MAX_BUFF]          = { 0 };
    char hostname[HOSTNAME_LEN] = { 0 };
    int  hostname_ret           = 0;

    bind_ret = bind(sockfd, (const struct sockaddr *)&scheduler, scheduler_len);
    if (0 > bind_ret)
    {
        perror("bind");
        return EXIT_FAILURE;
    }

    bytes_recv = recvfrom(sockfd, msg, MAX_BUFF, 0,
                         (struct sockaddr *)&submitter, &submitter_len);
    if (0 > bytes_recv)
    {
        perror("recvfrom");
        close(sockfd);
        return EXIT_FAILURE;
    }

    printf("msg recv: %s\n", msg);
    int port = get_port();
    printf("port generated: %d\n", port);
    char str_port[6] = { 0 };
    sprintf(str_port, "%d", port);

    bytes_sent = sendto(sockfd, str_port, 6, 0, (struct sockaddr *)&submitter, submitter_len);
    if (0 >= bytes_sent)
    {
        perror("sendto");
        close(sockfd);
        return EXIT_FAILURE;
    }
    printf("scheduler sent %ld bytes\n", bytes_sent);
    close(sockfd);

    return EXIT_SUCCESS;
}

