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

void setIpPort(char* ip, unsigned int port);
struct sockaddr_in getServer(int client_or_server);
void connectServer(struct sockaddr_in server);

//COMUNICACION CON BROKER
void trySendingMessage();

//CONEXION INICIAL
void trySocketCreation();
void tryServerConnection(struct sockaddr_in server);
void connectClient(struct sockaddr_in server);
void connectPublisher(struct sockaddr_in server);
void connectSubscriber(struct sockaddr_in server);

//REGISTRO
void sendRegistration(char* topic);
void sendPublisherRegistration(char* topic);
void sendSubscriberRegistration(char* topic);

//PUBLICAR
void sendPublication(char* msg);

//RECIBIR PUBLICACIÓN
void listenForPublications();

//CIERRE
void clientsClosing();
void unregister(int isPublisher);

#endif //PUBLICADORSUBSCRIPTOR_PROXYPUBSUB_H
