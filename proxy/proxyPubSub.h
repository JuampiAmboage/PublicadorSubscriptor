
#ifndef PUBLICADORSUBSCRIPTOR_PROXYPUBSUB_H
#define PUBLICADORSUBSCRIPTOR_PROXYPUBSUB_H

#include <stdbool.h>
#include <sys/timeb.h>
#include <signal.h>

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

struct sockaddr_in getServer(int client_or_server);
void trySocketCreation();
void tryServerConnection(struct sockaddr_in server);
void trySendingMessage();
void connectClient(struct sockaddr_in server);
void connectPublisher(struct sockaddr_in server);
void connectSubscriber(struct sockaddr_in server);
void connectServer(struct sockaddr_in server);
void sendRegistration(char* topic);
void sendPublisherRegistration(char* topic);
void sendSubscriberRegistration(char* topic);
void serverClosing();
void setIpPort(char* ip, unsigned int port);
void sendPublication(char* msg);

void * handlePublisherSignal(volatile sig_atomic_t flag);

#endif //PUBLICADORSUBSCRIPTOR_PROXYPUBSUB_H