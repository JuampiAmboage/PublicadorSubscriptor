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

int accept_client(int server_socket)
{
    struct sockaddr_in client;
    socklen_t len = sizeof(client);

    int client_socket = accept(server_socket, (struct sockaddr *)&client, &len);
    if (client_socket < 0)
        error("accept");

    return client_socket;
}

void receive_all(int socket, void *buffer, size_t length)
{
    char *ptr = buffer;
    size_t total = 0;
    ssize_t received = 0;

    while (total < length)
    {
        if ((received = recv(socket, ptr + total, length - total, 0)) <= 0)
            error("recv");

        total += received;
    }
}

message_t receive_message(int socket)
{
    message_t msg;
    receive_all(socket, &msg, sizeof(message_t));
    return msg;
}

bool can_launch_publisher(message_t msg, topic_t topics[10], int topic_count)
{
    int pub_count = 0;
    bool topic_exists = false;
    for (int i = 0; i < topic_count; i++)
    {
        pub_count += topics[i].pub_count;
        if (strcmp(msg.topic, topics[i].name) == 0)
        {
            topic_exists = true;
        }
    }

    return (pub_count <= 100) && (topic_exists || topic_count < 10);
}

bool can_launch_subscriber(message_t msg, topic_t topics[10], int topic_count)
{
    int sub_count = 0;
    bool topic_exists = false;
    for (int i = 0; i < topic_count; i++)
    {
        sub_count += topics[i].sub_count;
        if (strcmp(msg.topic, topics[i].name) == 0)
        {
            topic_exists = true;
        }
    }
    return (sub_count < 900) && topic_exists;
}

void add_publisher(char topic[100], topic_t topics[10], int *topic_count)
{
    for (int i = 0; i < *topic_count; i++)
    {
        if (strcmp(topic, topics[i].name) == 0)
        {
            topics[i].pub_count++;
            return;
        }
    }

    strcpy(topics[*topic_count].name, topic);
    topics[*topic_count].pub_count = 1;
    topics[*topic_count].sub_count = 0;
    (*topic_count)++;
}

void unregister_publisher(char topic[100], topic_t *topics, int *topic_count, pthread_mutex_t *topic_mutex)
{
    pthread_mutex_lock(topic_mutex);
    for (int i = 0; i < *topic_count; i++)
    {
        if (strcmp(topic, topics[i].name) == 0)
        {
            topics[i].pub_count--;
            if (topics[i].pub_count == 0)
            {
                // Close all subscriber sockets
                for (int j = 0; j < topics[i].sub_count; j++)
                {
                    close(topics[i].subs[j]);
                }

                // Remove topic
                for (int j = i; j < *topic_count - 1; j++)
                {
                    topics[j] = topics[j + 1];
                }

                (*topic_count)--;
            }
            return;
        }
    }
    pthread_mutex_unlock(topic_mutex);
}

typedef struct publisher
{
    char topic_name[100];
    int socket;
    topic_t *topics;
    int *topic_count;
    pthread_mutex_t *topic_mutex;
} publisher_t;

void send_all(int socket, const void *buffer, size_t length)
{
    const char *ptr = buffer;
    size_t total = 0;
    ssize_t sent = 0;

    while (total < length)
    {
        if ((sent = send(socket, ptr + total, length - total, 0)) == -1)
            error("send");

        total += sent;
    }
}

void publish_data_sequential(publisher_t *pub, message_t msg)
{
    pthread_mutex_lock(pub->topic_mutex);
    for (int i = 0; i < *pub->topic_count; i++)
    {
        if (strcmp(pub->topic_name, pub->topics[i].name) == 0)
        {
            for (int j = 0; j < pub->topics[i].sub_count; j++)
            {
                send_all(pub->topics[i].subs[j], &msg.data, sizeof(publish_t));
            }
            break;
        }
    }
    pthread_mutex_unlock(pub->topic_mutex);
}

void *publisher(void *arg)
{
    publisher_t *pub = (publisher_t *)arg;

    message_t message;
    do
    {
        message = receive_message(pub->socket);
        if (message.action == PUBLISH_DATA)
            publish_data_sequential(pub, message);
    } while (message.action != UNREGISTER_PUBLISHER);

    unregister_publisher(pub->topic_name, pub->topics, pub->topic_count, pub->topic_mutex);

    close(pub->socket);
    free(pub);
    return NULL;
}

void launch_publisher(message_t msg, topic_t topics[10], int *topic_count, int socket, pthread_mutex_t *topic_mutex)
{
    publisher_t *arg = malloc(sizeof(publisher_t));
    strcpy(arg->topic_name, msg.topic);
    arg->socket = socket;
    arg->topics = topics;
    arg->topic_count = topic_count;
    arg->topic_mutex = topic_mutex;
    pthread_t thread;
    if (pthread_create(&thread, NULL, publisher, arg))
        error("pthread_create");

    add_publisher(msg.topic, topics, topic_count);

    pthread_detach(thread);
}
