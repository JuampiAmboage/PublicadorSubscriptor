#ifdef __linux__
#include <arpa/inet.h>
#elif _WIN32
#include <Winsock2.h>
#endif

#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>

#include "proxyPubSub.h"

#define MAX_PUBLISHERS 100
#define MAX_SUBSCRIBERS 900
#define MAX_TOPICS 10
#define SIZE_BUFFER 6000

struct ip_port info;

struct timespec expectedTime;
struct message msgToBroker;
struct message incomingPublication;
struct response resFromBroker;

int receivedPublication = 0;

pthread_t publisherThreads[MAX_PUBLISHERS];
int registeredPublishers = 0;

pthread_t subscribersThreads[MAX_SUBSCRIBERS];
int registeredSubscribers = 0;

int fd_socket = 0, fd = 0, pub_fd = 0, sub_fd = 0;


void setIpPort (char* ip, unsigned int port){
    info.ip_process = ip;
    info.port_process = port;
}
//GETTER DEL SERVIDOR
struct sockaddr_in getServer(int client_or_server){
    // temp structure variable
    struct sockaddr_in server;
    server.sin_family = AF_INET; //Familia Ipv4

    if(client_or_server == 0) //0 cliente
        server.sin_addr.s_addr = inet_addr(info.ip_process);
    else  //1 servidor
        server.sin_addr.s_addr = htonl(INADDR_ANY);//Cualquier interfaz(IP) del server

    server.sin_port = htons(info.port_process);


    printf("---> IP: %s\n", info.ip_process);
    printf("---> STATUS_CODE: %d\n", server.sin_addr.s_addr);
    printf("---> PORT: %d\n",server.sin_port);

    return server;
}

//ABRIMOS EL SOCKET
void trySocketCreation(){
    fd_socket = socket(AF_INET, SOCK_STREAM, 0);
    if ((fd_socket ) < 0){
        perror("Socket open error");
        exit(EXIT_FAILURE);
    }
    else{
        printf("Socket successfully created...\n");
    }
}

//NOS CONECTAMOS AL SERVIDOR
void tryServerConnection(struct sockaddr_in server){
    int connection = connect(fd_socket, (struct sockaddr *)&server,sizeof(server));
    if(connection == -1){
        printf("connect() error\n");
        exit(EXIT_FAILURE);
    }
    else{
        printf("connected to the server...\n");
    }
}
//CLIENTE LE ENVÍA A BROKER
void trySendingMessage() {
    clock_gettime( CLOCK_REALTIME , &expectedTime);
    double pub_t = expectedTime.tv_nsec;

    if (send(fd_socket, &msgToBroker, sizeof(msgToBroker), 0) < 0) {
        printf("Send failed\n");
        exit(EXIT_FAILURE);
    } else {
        printf("Message succesfully send\n");
    }
}
//CONEXIÓN PARA PUBLICADOR Y SUBSCRIPTOR - LISTO
void connectClient(struct sockaddr_in server) {
    clock_gettime( CLOCK_REALTIME , &expectedTime);
    double pub_t = expectedTime.tv_nsec;

    trySocketCreation();
    tryServerConnection(server);
}
//CONECTARSE COMO PUBLICADOR - LISTO
void connectPublisher(struct sockaddr_in server){
    connectClient(server);
    printf("[%ld.%ld] Publisher conectado con el broker correctamente.\n",expectedTime.tv_sec,expectedTime.tv_nsec);
}

//CONECTARSE COMO SUBSCRIPTOR - LISTO
void connectSubscriber(struct sockaddr_in server){
    connectClient(server);
    printf("[%ld.%ld] Suscriptor conectado con el broker correctamente.\n",expectedTime.tv_sec,expectedTime.tv_nsec);
}

//ENVIAR REGISTRO A BROKER
void sendRegistration(char* topic){
    strcpy(msgToBroker.topic, topic);

    trySendingMessage();

    if(recv(fd_socket , &resFromBroker , sizeof(resFromBroker) , 0) < 0){
        printf("Reception failed\n");
        exit(EXIT_FAILURE);
    }

    printf("[%ld.%ld] Registrado correctamente con ID: %d para topic %s\n",expectedTime.tv_sec,expectedTime.tv_nsec,resFromBroker.id,msgToBroker.topic );
}

//REGISTRO COMO PUBLICAOR
void sendPublisherRegistration(char* topic){
    msgToBroker.action = REGISTER_PUBLISHER;
    sendRegistration(topic);
}
//REGISTRO COMO SUSCRIPTOR
void sendSubscriberRegistration(char* topic){
    msgToBroker.action = REGISTER_SUBSCRIBER;
    sendRegistration(topic);
}
//PUBLICAMOS
void sendPublication(char* msg){
    strcpy(msgToBroker.data.data, msg);
    msgToBroker.action = PUBLISH_DATA;
    trySendingMessage();
}

void unregister(int isPublisher){
    if(isPublisher) {
        msgToBroker.action = UNREGISTER_PUBLISHER;
    } else {
        msgToBroker.action = UNREGISTER_SUBSCRIBER;
    }

    trySendingMessage();
    printf("[%ld.%ld] De-Registrado correctamente del broker.\n",expectedTime.tv_sec,expectedTime.tv_nsec);
    //falta id
}

void listenForPublications(){
    clock_gettime( CLOCK_REALTIME , &expectedTime);
    double pub_t = expectedTime.tv_nsec;

    if(recv(fd_socket , &incomingPublication , sizeof(resFromBroker) , 0) < 0){
        printf("Reception in sub failed\n");
        exit(EXIT_FAILURE);
    }
    else {
        printf("[%d.%d] Recibido mensaje topic: %s - mensaje: %s\n",
               expectedTime.tv_sec, expectedTime.tv_nsec, incomingPublication.topic, incomingPublication.data.data);
    }
    //FALTA: Generó: $time_generated_data - Recibido: $time_received_data - Latencia: $latency
}

void clientsClosing() {
    close(fd_socket);
}