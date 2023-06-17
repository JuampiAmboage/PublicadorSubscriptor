#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include <errno.h>
#include <stdbool.h>
#include <arpa/inet.h>

#include "proxy_broker.h"

void error(const char *message)
{
    perror(message);
    exit(EXIT_FAILURE);
}

int init_server(int port)
{

    struct sockaddr_in server;

    int server_socket;

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    server.sin_port = htons(port);

    if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        error("socket");

    const int enable = 1;
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
        error("setsockopt");

    if (bind(server_socket, (struct sockaddr *)&server, sizeof(server)) < 0)
        error("bind");

    if (listen(server_socket, 1000) < 0)
        error("listen");

    printf("Server listening at %i\n", port);

    return server_socket;
}
