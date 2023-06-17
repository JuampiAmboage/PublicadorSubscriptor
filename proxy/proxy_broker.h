#ifndef PROXY_BROKER_H
#define PROXY_BROKER_H

#include <pthread.h>

typedef struct topic
{
    char name[100];
    int pub_count;
    int sub_count;
    int subs[900];
} topic_t;

enum operations
{
    REGISTER_PUBLISHER = 0,
    UNREGISTER_PUBLISHER,
    REGISTER_SUBSCRIBER,
    UNREGISTER_SUBSCRIBER,
    PUBLISH_DATA
};

typedef struct publish
{
    struct timespec time_generated_data;
    char data[100];
} publish_t;

typedef struct message
{
    enum operations action;
    char topic[100];
    // Solo utilizado en mensajes de UNREGISTER
    int id;
    // Solo utilizado en mensajes PUBLISH_DATA
    struct publish data;
} message_t;

int init_server(int port);

int accept_client(int server_socket);

message_t receive_message(int socket);

bool can_launch_publisher(message_t msg, topic_t topics[10], int topic_count);

#endif
