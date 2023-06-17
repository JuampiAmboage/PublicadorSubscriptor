#ifdef __linux__
#include <arpa/inet.h>
#elif _WIN32
#include <Winsock2.h>
#endif

#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>

#include "proxy/proxy_broker.h"

int main(int argc, char *argv[])
{

    char *mode;
    char *ip = "0.0.0.0";
    int port = 6666;

    int server_socket = init_server(port);

    return 0;
}
