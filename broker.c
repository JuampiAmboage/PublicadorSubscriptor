#ifdef __linux__
#include <arpa/inet.h>
#elif _WIN32
#include <Winsock2.h>
#endif

#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "proxy/proxy_broker.h"

int main(int argc, char *argv[])
{

    char *mode;
    char *ip = "0.0.0.0";
    int port = 6666;

    topic_t topics[10];
    int topic_count = 0;

    int server_socket = init_server(port);

    // init topic mutex
    pthread_mutex_t topic_mutex;
    pthread_mutex_init(&topic_mutex, NULL);

    while (true)
    {
        int client_socket = accept_client(server_socket);
        message_t msg = receive_message(client_socket);
        if (msg.action == REGISTER_PUBLISHER && can_launch_publisher(msg, topics, topic_count))
        {
            launch_publisher(msg, topics, &topic_count, client_socket, &topic_mutex);
        }
        else if (msg.action == REGISTER_SUBSCRIBER)
        {
        }
        else
        {
        }
    }

    return 0;
}
