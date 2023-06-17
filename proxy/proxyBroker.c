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
#include "proxyBroker.h"

#define MAX_PUBLISHERS 100
#define MAX_SUBSCRIBERS 900
#define MAX_TOPICS 10
#define SIZE_BUFFER 6000

struct ip_port info;

struct timespec expectedTime;
struct message requestedAction;
struct response resFromBroker;

int receivedPublication = 0;

int registeredPublishers = 0;
int registeredSubscribers = 0;

int serverIsClosed = 1;

Topic topics[MAX_TOPICS];

int topicCounter = 0;
int fd_socket = 0, fd = 0, clientSocket = 0;

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
    serverIsClosed = 0;

    printf("---> IP: %s\n", info.ip_process);
    printf("---> STATUS_CODE: %d\n", server.sin_addr.s_addr);
    printf("---> PORT: %d\n",server.sin_port);

    return server;
}
//DAR ID AL CLIENTE EN SERVIDOR - LISTO
void acceptClient(){
    clientSocket = accept(fd_socket, (struct sockaddr*)NULL, NULL);

    if (clientSocket == -1) {
        printf("Error en accept()\n");
        exit(EXIT_FAILURE);
    } else {
        printf("Conexión aceptada\n");
    }
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

//POR CADA REGISTRO ENTRANTE CREAMOS UN HILO
void processNewRegistration(int publishersIds[]){
    if ((recv(clientSocket, &requestedAction, sizeof(requestedAction),0)) < 0) {
        resFromBroker.response_status = _ERROR;
        resFromBroker.id = -1;
    } else {
        if(requestedAction.action == REGISTER_PUBLISHER){
            processNewPublisher(publishersIds);

        } else if(requestedAction.action == REGISTER_SUBSCRIBER) {
            processNewSubscriber();
        }
    }
    if( send(clientSocket , &resFromBroker , sizeof(resFromBroker) , 0) < 0){
        printf("Send failed from broker\n");
        exit(EXIT_FAILURE);
    }

}

//SE CREA UN NUEVO HILO DE PUBLICADOR
int processNewPublisher(int publishersIds[]){
    if(registeredPublishers > MAX_PUBLISHERS || topicCounter > MAX_TOPICS){
        resFromBroker.response_status = LIMIT;
        resFromBroker.id = -1;
    }
    else{
        struct timespec time_ex;
        clock_gettime(CLOCK_REALTIME, &time_ex);
        double pub_t = time_ex.tv_nsec;

        printf("[%ld.%ld] Nuevo cliente (%d) Publicador conectado : %s \n",time_ex.tv_sec,time_ex.tv_nsec,clientSocket ,requestedAction.topic );
        resFromBroker.response_status = OK;
        resFromBroker.id = clientSocket;

        printf("ID: %d\n",resFromBroker.id );
        printf("STATUS: %d\n",resFromBroker.response_status);
        processIncomingTopic(requestedAction.topic);
        publishersIds[registeredPublishers] = clientSocket;
        registeredPublishers++;
    }

    return resFromBroker.id;
}

//HILO DE PUBLICADOR QUE SE BLOQUEA A LA ESPERA DE PUBLICACIONES
void *publisherThread(void* args){
    int myId = *(int*)args;

    while(requestedAction.action != UNREGISTER_PUBLISHER){

        //recv(myId, &requestedAction, sizeof(requestedAction), 0);
        if (requestedAction.action == PUBLISH_DATA) {
            printf("PUBLICANDO: %s\n", requestedAction.data.data);
        }
    }
    //reorganize() ->reorganizamos el vector de publishers liberando el index
    pthread_exit(0);

}

//BUSCAMOS SI EL TOPIC YA ESTABA EN EL ARREGLO
int searchTopic(char topicForSearch[]) {
    for (int i = 0; i < topicCounter; i++) {
        if (strcmp(topics[i].content, topicForSearch) == 0) {
            return i;
        }
    }
    return -1;
}

void processIncomingTopic(char topic[]){
    int topicIndex = searchTopic(topic);
    if (topicIndex == -1) { //primera vez que se registra un pub con este topic
        strcpy(topics[topicCounter].content, topic);
        topicCounter++;
    }
}

//MISMA FUNCIÓN QUE PARA PUBLICADOR->CREAR FUNCIÓN ÚNICA
void processNewSubscriber(){
    if(registeredSubscribers > MAX_SUBSCRIBERS) {
        resFromBroker.response_status = LIMIT;
        resFromBroker.id = -1;
    } else {
        struct timespec time_ex;
        clock_gettime(CLOCK_REALTIME, &time_ex);
        double pub_t = time_ex.tv_nsec;

        printf("[%ld.%ld] Nuevo cliente (%d) Suscriptor conectado : %s \n",time_ex.tv_sec,time_ex.tv_nsec,clientSocket ,requestedAction.topic );
        resFromBroker.response_status = OK;
        resFromBroker.id = clientSocket;

        printf("ID: %d\n",resFromBroker.id );
        printf("STATUS: %d\n",resFromBroker.response_status);
        int topicIndex = searchTopic(requestedAction.topic);
        topics[topicIndex].subscribersIds[registeredSubscribers] = clientSocket;
        registeredSubscribers++;
    }
}

void *contactSubscriber(){
    //variable de condicion
}

void *subscriberThread(){
    int myId = clientSocket;
    while(requestedAction.action != UNREGISTER_SUBSCRIBER){
        recv(myId, &requestedAction, sizeof(requestedAction), 0);
    }
    pthread_exit(0);
}


void serverClosing(){
    printf("\n");
    printf("server closed\n");
    serverIsClosed = 1;
    close(fd_socket);
}

void resizeIds(int idFromPubToDelete) {
    // Desplazar los elementos hacia la izquierda a partir del índice especificado
    for (int i = idFromPubToDelete; i < registeredPublishers - 1; i++) {
        arreglo[i] = arreglo[i + 1];
    }
}

void lookForPublications(int publishersIds[]) {
    while(!serverIsClosed) {
        for (int i = 0; i < registeredPublishers; i++) {
            recv(publishersIds[i], &requestedAction, sizeof(requestedAction), 0);
            if (requestedAction.action == PUBLISH_DATA) {
                printf("PUBLICANDO PARA SUBS: %s\n", requestedAction.data.data);
                sendToSubscribers();
            }
            else if(requestedAction.action == UNREGISTER_PUBLISHER){
                resizeIds(i);
                registeredPublishers--;
            }
        }
    }
}

void sendToSubscribers(){
    int topicIndex = searchTopic(requestedAction.topic);

    for (int i=0; i<registeredSubscribers;i++){
        if( send( topics[topicIndex].subscribersIds[i] , &requestedAction , sizeof(requestedAction) , 0) < 0){
            printf("Send failed from broker to sub \n");
            exit(EXIT_FAILURE);
        }
    }
}