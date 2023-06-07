#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
//#include <Winsock2.h>
#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <semaphore.h>
#include <arpa/inet.h>

#include "newProxy.h"

struct ip_port info;

struct message msgToBroker;
struct message regPublisher;

struct response resFromBroker;

int fd_socket = 0, fd = 0, pub_fd = 0, sub_fd = 0;

void setIpPort (char* ip, unsigned int port){
    info.ip_process = ip;
    info.port_process = port;
}

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

//tratamos de abrir el socket
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

//tratamos de conectarnos al servidor
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

void trySendingMessage(struct message toSend){
    if( send(fd_socket , &toSend , sizeof(toSend) , 0) < 0){
        printf("Send failed\n");
        exit(EXIT_FAILURE);
    }
    else
        printf("Message for registration succesfully send\n");
}

struct timespec connectClient(struct sockaddr_in server) {
    struct timespec expectedTime;
    clock_gettime( CLOCK_REALTIME , &expectedTime);
    double pub_t = expectedTime.tv_nsec;

    trySocketCreation();
    tryServerConnection(server);
}

void connectPublisher(struct sockaddr_in server){
    struct timespec expectedTime = connectClient(server);
    printf("[%ld.%ld] Publisher conectado con el broker correctamente.\n",expectedTime.tv_sec,expectedTime.tv_nsec);
}

void connectSubscriber(struct sockaddr_in server){
    struct timespec expectedTime = connectClient(server);
    printf("[%ld.%ld] Suscriptor conectado con el broker correctamente.\n",expectedTime.tv_sec,expectedTime.tv_nsec);
}

struct timespec registerClient(char* topic){
    struct timespec expectedTime;
    clock_gettime( CLOCK_REALTIME , &expectedTime);
    double pub_t = expectedTime.tv_nsec;

    strcpy(msgToBroker.topic, topic);

    trySendingMessage(msgToBroker);

    resFromBroker.id = pub_fd;
    resFromBroker.response_status = OK;

    if( recv(fd_socket , &resFromBroker , sizeof(resFromBroker) , 0) < 0){
        printf("Send failed\n");
        exit(EXIT_FAILURE);
    }
    printf("ID: %d\n", resFromBroker.id);
    printf("STATUS: %d\n", resFromBroker.response_status);

    printf("[%ld.%ld] Registrado correctamente con ID: %d para topic %s\n",expectedTime.tv_sec,expectedTime.tv_nsec,resFromBroker.id,msgToBroker.topic );
}

void registerPublisher(char* topic){
    msgToBroker.action = REGISTER_PUBLISHER;
    registerClient(topic);
}

void registerSubscriber(char* topic){
    msgToBroker.action = REGISTER_SUBSCRIBER;
    registerClient(topic);
}

void processNewRegistration(){
    pthread_t threads[50];
    int incoming_clients = 0;

    struct message requestedAction;

    fd = accept(fd_socket,(struct sockaddr*)NULL, NULL);
    if (fd == -1) {
        printf("error en accept()\n");
        exit(EXIT_FAILURE);
    }
    else{
        printf("%s","Conexión aceptada\n" );
    }
    if ((recv(fd, &requestedAction, sizeof(requestedAction),0)) < 0){
        printf("recv failed");
    }
    else {

        printf("%i\n", fd);

            if(requestedAction.action == REGISTER_PUBLISHER){
                printf("PUB: %d\n",requestedAction.action );
                pub_fd = fd;
            }
            else if(requestedAction.action == REGISTER_SUBSCRIBER){
                printf("SUB: %d\n",requestedAction.action );
                sub_fd = fd;
            }

            //para cada solicitud del cliente crea un hilo y le asigna la solicitud del cliente para procesar
            //para que el hilo principal pueda manejar la próxima solicitud
            //int *pclient = malloc(sizeof(int));
            //*pclient = fd_client_socket;
            //thread = pthread_create(&threads[incoming_clients + 1], NULL, socketThread, pclient);

            /*int thread = pthread_create(&threads[incoming_clients + 1], NULL, socketThread, NULL);
            incoming_clients += 1;

            if (thread) {
                printf("ERROR; return code from pthread_create() is %d\n", thread);
                exit(EXIT_FAILURE);
            }*/
        }
    }

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

void serverClosing(){
    close(fd_socket);
}

void clients_closing(){

    close(fd_socket);
}