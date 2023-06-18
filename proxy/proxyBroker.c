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

#include "proxyBroker.h"

typedef struct publisher
{
    char topic_name[100];
    int socket;
    topic_t *topics;
    int *topic_count;
    pthread_mutex_t *topic_mutex;
    int mode;
} publisher_t;

typedef struct subscriber
{
    char topic_name[100];
    int socket;
    topic_t *topics;
    int *topic_count;
    pthread_mutex_t *topic_mutex;
} subscriber_t;

enum status
{
    ERROR = 0,
    LIMIT,
    OK
};

typedef struct response
{
    enum status response_status;
    int id;
} response_t;

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

void respond_ok(int socket, int id)
{
    response_t msg;
    msg.id = id;
    msg.response_status = OK;
    send_all(socket, &msg, sizeof(response_t));
}

int add_publisher(char topic[100], topic_t topics[10], int *topic_count)
{
    for (int i = 0; i < *topic_count; i++)
    {
        if (strcmp(topic, topics[i].name) == 0)
        {
            topics[i].pub_count++;
            return topics[i].pub_count;
        }
    }

    strcpy(topics[*topic_count].name, topic);
    topics[*topic_count].pub_count = 1;
    topics[*topic_count].sub_count = 0;
    (*topic_count)++;
    return 1;
}

void close_subscriber(int socket)
{
    message_t msg;
    msg.action = UNREGISTER_SUBSCRIBER;
    send_all(socket, &msg, sizeof(message_t));
    close(socket);
}

void print_client_update(char *topic, int id, topic_t topics[10], int *topic_count, char *client_type, char *action)
{
    struct timespec now;
    clock_gettime(CLOCK_REALTIME, &now);

    printf("[%ld.%09ld] %s cliente (%i) %s conectado: %s\n", now.tv_sec, now.tv_nsec, action, id, client_type, topic);
    printf("Resumen:\n");
    for (int i = 0; i < *topic_count; i++)
    {
        printf("%s: %i Subscriptores - %i Publicadores\n", topics[i].name, topics[i].sub_count, topics[i].pub_count);
    }
}

void unregister_publisher(char topic[100], topic_t *topics, int *topic_count, pthread_mutex_t *topic_mutex, int id)
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
                    close_subscriber(topics[i].subs[j]);
                }

                // Remove topic
                for (int j = i; j < *topic_count - 1; j++)
                {
                    topics[j] = topics[j + 1];
                }

                (*topic_count)--;
            }
        }
    }
    print_client_update(topic, id, topics, topic_count, "Publicador", "Desconectado");
    pthread_mutex_unlock(topic_mutex);
}

void publish_data_sequential(publisher_t *pub, message_t msg)
{
    pthread_mutex_lock(pub->topic_mutex);
    for (int i = 0; i < *pub->topic_count; i++)
    {
        if (strcmp(pub->topic_name, pub->topics[i].name) == 0)
        {
            struct timespec now;
            clock_gettime(CLOCK_REALTIME, &now);
            printf("[%ld.%09ld] Enviando mensaje en topic %s a %i suscriptores\n", now.tv_sec, now.tv_nsec, msg.topic, pub->topics[i].sub_count);
            for (int j = 0; j < pub->topics[i].sub_count; j++)
            {
                message_t send;
                send.action = PUBLISH_DATA;
                strcpy(send.topic, msg.topic);
                send.data = msg.data;
                send.id = msg.id;
                send_all(pub->topics[i].subs[j], &send, sizeof(message_t));
            }
            break;
        }
    }
    pthread_mutex_unlock(pub->topic_mutex);
}

typedef struct
{
    int socket;
    message_t msg;
    pthread_barrier_t *barrier;
} thread_send_message_args_t;

void *thread_send_message(void *arg)
{
    thread_send_message_args_t *args = (thread_send_message_args_t *)arg;
    if (args->barrier)
        pthread_barrier_wait(args->barrier);
    send_all(args->socket, &args->msg, sizeof(message_t));
    free(args);
    return NULL;
}

void publish_data_parallel(publisher_t *pub, message_t msg)
{
    pthread_t thread;
    pthread_mutex_lock(pub->topic_mutex);
    for (int i = 0; i < *pub->topic_count; i++)
    {
        if (strcmp(pub->topic_name, pub->topics[i].name) == 0)
        {
            for (int j = 0; j < pub->topics[i].sub_count; j++)
            {
                message_t send;
                send.action = PUBLISH_DATA;
                strcpy(send.topic, msg.topic);
                send.data = msg.data;
                send.id = msg.id;
                thread_send_message_args_t *args = malloc(sizeof(thread_send_message_args_t));
                args->socket = pub->topics[i].subs[j];
                args->msg = send;
                args->barrier = NULL;
                if (pthread_create(&thread, NULL, thread_send_message, args))
                    error("pthread_create");
                pthread_detach(thread);
            }
            break;
        }
    }
    pthread_mutex_unlock(pub->topic_mutex);
}

void publish_data_fair(publisher_t *pub, message_t msg)
{
    pthread_t threads[900];
    pthread_barrier_t barrier;

    pthread_mutex_lock(pub->topic_mutex);
    for (int i = 0; i < *pub->topic_count; i++)
    {
        if (strcmp(pub->topic_name, pub->topics[i].name) == 0)
        {
            pthread_barrier_init(&barrier, NULL, pub->topics[i].sub_count);

            for (int j = 0; j < pub->topics[i].sub_count; j++)
            {
                message_t send;
                send.action = PUBLISH_DATA;
                strcpy(send.topic, msg.topic);
                send.data = msg.data;
                send.id = msg.id;
                thread_send_message_args_t *args = malloc(sizeof(thread_send_message_args_t));
                args->socket = pub->topics[i].subs[j];
                args->msg = send;
                args->barrier = &barrier;
                if (pthread_create(&threads[j], NULL, thread_send_message, args))
                    error("pthread_create");
            }
            for (int j = 0; j < pub->topics[i].sub_count; j++)
            {
                pthread_join(threads[j], NULL);
            }
            pthread_barrier_destroy(&barrier);
            break;
        }
    }
    pthread_mutex_unlock(pub->topic_mutex);
}

void *publisher(void *arg)
{
    publisher_t *pub = (publisher_t *)arg;

    void (*publisher_mode)(publisher_t *, message_t) = &publish_data_sequential;

    if (pub->mode == PARALLEL)
    {
        publisher_mode = &publish_data_parallel;
    }
    else if (pub->mode == FAIR)
    {
        publisher_mode = &publish_data_fair;
    }

    message_t message;
    do
    {
        message = receive_message(pub->socket);
        struct timespec now;
        clock_gettime(CLOCK_REALTIME, &now);
        printf("[%ld.%09ld] Recibido mensaje para publicar en topic: %s - mensaje: %s - Genero: %ld.%09ld\n",
               now.tv_sec,
               now.tv_nsec,
               message.data.data,
               message.data.time_generated_data.tv_sec,
               message.data.time_generated_data.tv_nsec);

        if (message.action == PUBLISH_DATA)
            publisher_mode(pub, message);
    } while (message.action != UNREGISTER_PUBLISHER);

    unregister_publisher(pub->topic_name, pub->topics, pub->topic_count, pub->topic_mutex, message.id);

    close(pub->socket);
    free(pub);
    return NULL;
}

void launch_publisher(message_t msg, topic_t topics[10], int *topic_count, int socket, pthread_mutex_t *topic_mutex, int mode)
{
    publisher_t *arg = malloc(sizeof(publisher_t));
    strcpy(arg->topic_name, msg.topic);
    arg->socket = socket;
    arg->topics = topics;
    arg->topic_count = topic_count;
    arg->topic_mutex = topic_mutex;
    arg->mode = mode;
    pthread_t thread;
    if (pthread_create(&thread, NULL, publisher, arg))
        error("pthread_create");

    pthread_mutex_lock(topic_mutex);
    int id = add_publisher(msg.topic, topics, topic_count);
    respond_ok(socket, id);
    print_client_update(msg.topic, id, topics, topic_count, "Publicador", "Nuevo");
    pthread_mutex_unlock(topic_mutex);

    pthread_detach(thread);
}

int add_subscriber(char topic[100], topic_t topics[10], int *topic_count, int socket)
{
    for (int i = 0; i < *topic_count; i++)
    {
        if (strcmp(topic, topics[i].name) == 0)
        {
            topics[i].subs[topics[i].sub_count++] = socket;
            respond_ok(socket, topics[i].sub_count);
            return topics[i].sub_count;
        }
    }
}

void unregister_subscriber(char topic[100], topic_t *topics, int *topic_count, pthread_mutex_t *topic_mutex, int socket, int id)
{
    pthread_mutex_lock(topic_mutex);
    for (int i = 0; i < *topic_count; i++)
    {
        if (strcmp(topic, topics[i].name) == 0)
        {
            for (int j = 0; j < topics[i].sub_count; j++)
            {
                if (topics[i].subs[j] == socket)
                {
                    for (int k = j; k < topics[i].sub_count - 1; k++)
                    {
                        topics[i].subs[k] = topics[i].subs[k + 1];
                    }
                    topics[i].sub_count--;
                    print_client_update(topic, id, topics, topic_count, "Subscriptor", "Eliminado");
                    break;
                }
            }
            break;
        }
    }
    pthread_mutex_unlock(topic_mutex);
}

void *subscriber(void *arg)
{
    subscriber_t *sub = (subscriber_t *)arg;

    message_t message;
    do
    {
        message = receive_message(sub->socket);
    } while (message.action != UNREGISTER_SUBSCRIBER);

    unregister_subscriber(sub->topic_name, sub->topics, sub->topic_count, sub->topic_mutex, sub->socket, message.id);

    close(sub->socket);
    free(sub);
    return NULL;
}

void launch_subscriber(message_t msg, topic_t topics[10], int *topic_count, int socket, pthread_mutex_t *topic_mutex)
{
    subscriber_t *arg = malloc(sizeof(subscriber_t));
    strcpy(arg->topic_name, msg.topic);
    arg->socket = socket;
    arg->topics = topics;
    arg->topic_count = topic_count;
    arg->topic_mutex = topic_mutex;
    pthread_t thread;
    if (pthread_create(&thread, NULL, subscriber, arg))
        error("pthread_create");

    pthread_mutex_lock(topic_mutex);
    int id = add_subscriber(msg.topic, topics, topic_count, socket);
    print_client_update(msg.topic, id, topics, topic_count, "Suscriptor", "Nuevo");
    pthread_mutex_unlock(topic_mutex);

    pthread_detach(thread);
}

void respond_error(int socket)
{
    response_t msg;
    msg.response_status = ERROR;
    msg.id = -1;
    send_all(socket, &msg, sizeof(response_t));
}

void respond_limit(int socket)
{
    response_t msg;
    msg.response_status = LIMIT;
    msg.id = -1;
    send_all(socket, &msg, sizeof(response_t));
}
