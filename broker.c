#ifdef __linux__
#include <arpa/inet.h>
#elif _WIN32
#include <Winsock2.h>
#endif

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>
#include <getopt.h>
#include <string.h>

#include "proxy/proxyBroker.h"

void parse_args(int argc, char *argv[], int *port, char **mode)
{
    // Set default values
    *port = 0;
    *mode = NULL;

    // Define long options
    static struct option long_options[] = {
        {"port", required_argument, 0, 'p'},
        {"mode", optional_argument, 0, 'm'},
        {0, 0, 0, 0} // Required to terminate the array
    };

    // Parse options
    int option;
    int option_index = 0;
    while ((option = getopt_long(argc, argv, "p:m::", long_options, &option_index)) != -1)
    {
        switch (option)
        {
        case 'p':
            *port = atoi(optarg);
            break;
        case 'm':
            if (optarg != NULL)
            {
                *mode = optarg;
            }
            break;
        default:
            // Handle invalid options or missing required arguments
            exit(EXIT_FAILURE);
        }
    }

    // Check if the required options are provided
    if (*port == 0)
    {
        fprintf(stderr, "Usage: %s --port PORT [--mode writer/reader]\n", argv[0]);
        exit(EXIT_FAILURE);
    }
}

int main(int argc, char *argv[])
{
    int port;
    char *mode_arg;

    parse_args(argc, argv, &port, &mode_arg);
    int mode = SEQUENTIAL;
    if (mode_arg)
    {
        if (strcmp(mode_arg, "parallel") == 0)
        {
            mode = PARALLEL;
        }
        else if (strcmp(mode_arg, "fair") == 0)
        {
            mode = FAIR;
        }
    }

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
        if (msg.action == REGISTER_PUBLISHER)
        {
            if (can_launch_publisher(msg, topics, topic_count))
            {
                launch_publisher(msg, topics, &topic_count, client_socket, &topic_mutex, mode);
            }
            else
            {
                respond_limit(client_socket);
                close(client_socket);
            }
        }
        else if (msg.action == REGISTER_SUBSCRIBER)
        {
            if (can_launch_subscriber(msg, topics, topic_count))
            {
                launch_subscriber(msg, topics, &topic_count, client_socket, &topic_mutex);
            }
            else
            {
                respond_limit(client_socket);
                close(client_socket);
            }
        }
        else
        {
            respond_error(client_socket);
            close(client_socket);
        }
    }

    return 0;
}
