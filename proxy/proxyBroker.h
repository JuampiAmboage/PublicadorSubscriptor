#ifndef PUBLICADORSUBSCRIPTOR_PROXYBROKER_H
#define PUBLICADORSUBSCRIPTOR_PROXYBROKER_H


#ifndef PUBLICADORSUBSCRIPTOR_NEWPROXY_H
#define PUBLICADORSUBSCRIPTOR_NEWPROXY_H


#include <stdbool.h>

enum operations {
    REGISTER_PUBLISHER = 0,
    UNREGISTER_PUBLISHER,
    REGISTER_SUBSCRIBER,
    UNREGISTER_SUBSCRIBER,
    PUBLISH_DATA
};

struct publish {
    struct timespec time_generated_data;
    char data[100];
};

struct message {
    enum operations action;
    char topic[100];
    // Solo utilizado en mensajes de UNREGISTER
    int id;
    // Solo utilizado en mensajes PUBLISH_DATA
    struct publish data;
};


enum status {
    _ERROR = 0,
    LIMIT,
    OK
};
typedef struct{
    char content[100];
    int subscribersIds[900];
} Topic;

struct response {
    enum status response_status;
    int id;
};

struct ip_port{
    char* ip_process;
    unsigned int port_process;
};

struct signalMessage {
    int signalType; // Tipo de señal a enviar
};

struct receivedSignal {
    int signalType; // Tipo de señal a enviar
};

void setIpPort(char* ip, unsigned int port);
struct sockaddr_in getServer(int client_or_server);
void connectServer(struct sockaddr_in server);
void acceptClient();

void defineMutex();
void destroyMutex();

void processNewRegistration(int publishersIds[]);
int processNewPublisher(int publishersIds[]);
int searchTopic(char topicForSearch[]);
void processIncomingTopic(char topic[]);

void processNewSubscriber();

void *publisherThread();
void* registerPublisher();

void *contactSubscriber();
void *subscriberThread();

void serverClosing();

void lookForPublications(int publishersIds[]);
void resizeIds(int idFromPubToDelete, int publisherIDs[]);

void sendToSubscribers();

#endif //PUBLICADORSUBSCRIPTOR_NEWPROXY_H



#endif //PUBLICADORSUBSCRIPTOR_PROXYBROKER_H
