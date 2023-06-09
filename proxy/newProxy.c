#ifdef __linux__
#include <arpa/inet.h>
#elif _WIN32
#include <Winsock2.h>
#endif

#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <semaphore.h>
#include <stdbool.h>

#include "newProxy.h"

#define MAX_PUBLISHERS 100
#define MAX_SUBSCRIBERS 900
#define MAX_TOPICS 10
#define SIZE_BUFFER 6000

struct ip_port info;

struct timespec expectedTime;
struct message msgToBroker;
struct message requestedAction;
struct response resFromBroker;
struct signalMessage signalMessage;
struct receivedSignal receivedSignal;

int receivedPublication = 0;

pthread_t publisherThreads[MAX_PUBLISHERS];
int registeredPublishers = 0;

pthread_t subscribersThreads[MAX_SUBSCRIBERS];
int registeredSubscribers = 0;

int fd_socket = 0, fd = 0, pub_fd = 0, sub_fd = 0;

pthread_mutex_t mutex;
pthread_cond_t cond;

void setIpPort (char* ip, unsigned int port){
    info.ip_process = ip;
    info.port_process = port;
}
//GETTER DEL SERVIDOR - LISTO
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

//ABRIMOS EL SOCKET - LISTO
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

//NOS CONECTAMOS AL SERVIDOR - LISTO
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

void trySendingMessage(struct message toSend) {
    clock_gettime( CLOCK_REALTIME , &expectedTime);
    double pub_t = expectedTime.tv_nsec;

    if (send(fd_socket, &toSend, sizeof(toSend), 0) < 0) {
        printf("Send failed\n");
        exit(EXIT_FAILURE);
    } else {
        printf("Message succesfully send\n");
    }
}
//CONEXIÓN DE USO COMÚN PARA PUBLICADOR Y SUBSCRIPTOR - LISTO
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

void sendRegistration(char* topic){
    struct timespec expectedTime;

    strcpy(msgToBroker.topic, topic);

    trySendingMessage(msgToBroker);

    if(recv(fd_socket , &resFromBroker , sizeof(resFromBroker) , 0) < 0){
        printf("Send failed\n");
        exit(EXIT_FAILURE);
    }

    printf("[%ld.%ld] Registrado correctamente con ID: %d para topic %s\n",expectedTime.tv_sec,expectedTime.tv_nsec,resFromBroker.id,msgToBroker.topic );
}

void sendPublisherRegistration(char* topic){
    msgToBroker.action = REGISTER_PUBLISHER;
    sendRegistration(topic);
}

void sendSubscriberRegistration(char* topic){
    msgToBroker.action = REGISTER_SUBSCRIBER;
    sendRegistration(topic);
}

void sendPublication(char* msg){
    strcpy(msgToBroker.data.data, msg);
    msgToBroker.action = PUBLISH_DATA;
    trySendingMessage(msgToBroker);
}

//DAR ID AL CLIENTE EN SERVIDOR - LISTO
int acceptClient(){
    int clientSocket = accept(fd_socket, (struct sockaddr*)NULL, NULL);

    if (clientSocket == -1) {
        printf("Error en accept()\n");
        exit(EXIT_FAILURE);
    } else {
        printf("Conexión aceptada\n");
    }
    return clientSocket;
}

int cantHilos = 0;
//CREACIÓN DE HILOS X PUB/SUB ENTRANTE - EN DESARROLLO
void processNewRegistration(int clientSocket){
    pub_fd = clientSocket;
    resFromBroker.id = clientSocket;
    pthread_t hilo1;
    pthread_t hilo2;

    // Crear un hilo para el cliente registrado
    if(registeredPublishers+1 > MAX_PUBLISHERS){
        resFromBroker.response_status = LIMIT;
        send(clientSocket , &resFromBroker , sizeof(resFromBroker) , 0);
    }
    else {
        pthread_t hilo;
        if (cantHilos == 0) {
            hilo = hilo1;
            cantHilos++;
        } else {
            hilo = hilo2;
        }
        int threadCreateResult = pthread_create(&hilo, NULL,
                                                (void *) registerPublisher, NULL);
        if (threadCreateResult != 0) {
            printf("Error creating thread for client %d\n", clientSocket);
        }
        registeredPublishers++;
    }

}

void defineMutex(){
    pthread_mutex_init(&mutex,NULL);
    pthread_cond_init(&cond,NULL);
}

void destroyMutex(){
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&cond);
}

//FUNCIÓN DE EJECUCIÓN DE HILO PARA PUBLICADOR - EN DESAROLLO
void *registerPublisher() {
    int myId = pub_fd;

    pthread_mutex_lock(&mutex);
    if ((recv(myId, &requestedAction, sizeof(requestedAction),0)) < 0) {
        resFromBroker.response_status = _ERROR;
    } else {
        struct timespec time_ex;

        clock_gettime(CLOCK_REALTIME, &time_ex);
        double pub_t = time_ex.tv_nsec;

        printf("[%ld.%ld] Nuevo cliente (%d) Publicador conectado : %s \n",time_ex.tv_sec,time_ex.tv_nsec,pub_fd ,requestedAction.topic );
        resFromBroker.response_status = OK;

        printf("ID: %d\n",resFromBroker.id );
        printf("STATUS: %d\n",resFromBroker.response_status);
    }
    pthread_mutex_unlock(&mutex);

    if( send(myId , &resFromBroker , sizeof(resFromBroker) , 0) < 0){
        printf("Send failed from broker\n");
        exit(EXIT_FAILURE);
    }

    do{
        pthread_mutex_lock(&mutex);

        recv(myId, &requestedAction, sizeof(receivedSignal), 0);
        if (requestedAction.action == PUBLISH_DATA)
            printf("PUBLICANDO: %s\n",requestedAction.data.data);

        pthread_mutex_unlock(&mutex);

    }while(requestedAction.action != UNREGISTER_PUBLISHER);
    pthread_exit(0);
}

//CONECTARSE AL SERVIDOR - LISTO
void connectServer(struct sockaddr_in server){
    int dir_socket, listen_socket;

    fd_socket = socket(AF_INET, SOCK_STREAM, 0);

    if ((fd_socket ) < 0){
        perror("Socket open error");
        exit(EXIT_FAILURE);
    }
    else{
        printf("Socket successfully created...\n");
    }

    const int enable = 1;
    if (setsockopt(fd_socket, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
        perror("setsockopt(SO_REUSEADDR) failed");

    //avisamos al sistema que se creó un socket
    dir_socket = bind(fd_socket,(struct sockaddr *) &server, sizeof(server) );

    if ((dir_socket ) < 0){
        printf("%i",dir_socket);
        printf("Bind error\n");
        exit(EXIT_FAILURE);
    }
    else{
        printf("Socket successfully binded...\n");
    }
    //establecemos el socket en modo escucha
    //listen en el socket, con 1000 solicitudes de conexión máximas en cola
    //set the backlog to 65635; instead it tells the kernel to use the biggest backlog that it feels comfortable with
    listen_socket = listen(fd_socket, 1000);//?????
    if(listen_socket == -1) {
        printf("Listen error\n");
        exit(EXIT_FAILURE);
    }
    else{
        printf("Server listening...\n");
    }
}

void * handlePublisherSignal(volatile sig_atomic_t flag){
    flag = 1;
    msgToBroker.action = UNREGISTER_PUBLISHER;
    trySendingMessage(msgToBroker);
    printf("[%ld.%ld] De-Registrado correctamente del broker.\n",expectedTime.tv_sec,expectedTime.tv_nsec);
    //falta id
}

void serverClosing(){
    close(fd_socket);
}

void clients_closing(){
    close(fd_socket);
}