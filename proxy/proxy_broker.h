#ifndef PROXY_BROKER_H
#define PROXY_BROKER_H

#include <pthread.h>

enum operations
{
    REGISTER_PUBLISHER = 0,
    UNREGISTER_PUBLISHER,
    REGISTER_SUBSCRIBER,
    UNREGISTER_SUBSCRIBER,
    PUBLISH_DATA
};

struct publish
{
    struct timespec time_generated_data;
    char data[100];
} publish_t;

struct message
{
    enum operations action;
    char topic[100];
    // Solo utilizado en mensajes de UNREGISTER
    int id;
    // Solo utilizado en mensajes PUBLISH_DATA
    struct publish data;
} message_t;

int init_server(int port);

#endif
