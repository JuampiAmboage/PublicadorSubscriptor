#include <stdlib.h>
#include <sys/types.h>
//#include <sys/socket.h>
//#include <netinet/in.h>
#include <unistd.h>
#include <signal.h>
#include <arpa/inet.h>
//#include <Winsock2.h>

#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <semaphore.h>

#include "proxy.h"

#define SIZE_BUFFER 500


struct ip_port info;
struct message pub2broker;
struct message else2broker;
struct message sub2broker;
struct response broker2pub;
struct response broker2pub_response;
struct response broker2sub_response;

int fd_socket = 0, fd = 0, pub_fd = 0, sub_fd = 0;

struct sockaddr_in getDetail(int client_or_server);


/*Esta función devuelve una estructura struct sockaddr_in que contiene los detalles del servidor
 * o cliente según el parámetro client_or_server. Establece la dirección IP, el puerto y otra información
 * necesaria. La implementación es la siguiente:*/

struct sockaddr_in getDetail(int client_or_server){
    // temp structure variable
    struct sockaddr_in server;

    //0 cliente
    //1 servidor
    server.sin_family = AF_INET; //Familia Ipv4

    if(client_or_server == 0){
        server.sin_addr.s_addr = inet_addr(info.ip_process);
    }
    else{
        server.sin_addr.s_addr = htonl(INADDR_ANY);//Cualquier interfaz(IP) del server
    }

    server.sin_port = htons(info.port_process);
    return server;
}

/*Esta función establece la dirección IP y el puerto para el servidor o cliente.
 * Actualiza la estructura info con la dirección IP y el puerto proporcionados.*/

void set_ip_port (char* ip, unsigned int port){
    info.ip_process = ip;
    info.port_process = port;
}
void set_port (unsigned int port){
    info.port_process = port;
}

/*Esta función crea un socket de cliente, lo conecta al servidor especificado en el parámetro server
y envía un mensaje de registro. Establece una conexión con el servidor y envía el mensaje pub2broker.*/

void connect_client(struct sockaddr_in server) {
    struct timespec time_ex;

    clock_gettime( CLOCK_REALTIME , &time_ex);

    double pub_t = time_ex.tv_nsec;

    fd_socket = socket(AF_INET, SOCK_STREAM, 0);
    if ((fd_socket ) < 0){
        perror("Socket open error");
        exit(EXIT_FAILURE);
    }
    else{
        printf("Socket successfully created...\n");
    }
    //nos conectamos al servidor
    int connection = connect(fd_socket, (struct sockaddr *)&server,sizeof(server));
    if(connection == -1){
        printf("connect() error\n");
        exit(EXIT_FAILURE);
    }
    else{
        printf("connected to the server...\n");
    }


    printf("[%ld.%ld] Publisher conectado con el broker correctamente.\n",time_ex.tv_sec,time_ex.tv_nsec  );
    pub2broker.action = REGISTER_PUBLISHER;

    printf("%d\n",pub2broker.action );

    if( send(fd_socket , &pub2broker , sizeof(pub2broker) , 0) < 0){
        printf("Send failed\n");
        exit(EXIT_FAILURE);
    }
}

/*Esta función crea un socket de cliente, lo conecta al servidor especificado en el parámetro server
 * y envía un mensaje de registro. Establece una conexión con el servidor y envía el mensaje pub2broker.*/

void connect_server(struct sockaddr_in server){
    //srand(time(0));
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

/*Esta función se llama en el contexto del servidor y acepta conexiones entrantes de clientes.
 * Crea un hilo separado para cada conexión aceptada y llama a la función socketThread.*/

void client_accept(char* mode){
    printf("%s\n",mode );
    pthread_t threads[50];
    int incoming_clients = 0;

    struct message client_mode;

    while(1){
        fd = accept(fd_socket,(struct sockaddr*)NULL, NULL);
        if (fd == -1) {
            printf("error en accept()\n");
            exit(EXIT_FAILURE);
        }
        else{
            printf("%s","Conexión aceptada\n" );
        }
        if ((recv(fd, &client_mode, sizeof(client_mode),0)) < 0){
            printf("recv failed");
        }
        else{

            printf("%i\n", fd);

            if(client_mode.action == 0){
                printf("PUB: %d\n",client_mode.action );
                pub_fd = fd;
            }
            else if(client_mode.action == 2){
                printf("SUB: %d\n",client_mode.action );
                sub_fd = fd;
            }

            //para cada solicitud del cliente crea un hilo y le asigna la solicitud del cliente para procesar
            //para que el hilo principal pueda manejar la próxima solicitud
            //int *pclient = malloc(sizeof(int));
            //*pclient = fd_client_socket;
            //thread = pthread_create(&threads[incoming_clients + 1], NULL, socketThread, pclient);
            int thread = pthread_create(&threads[incoming_clients + 1], NULL, socketThread, NULL);
            incoming_clients += 1;

            if (thread) {
                printf("ERROR; return code from pthread_create() is %d\n", thread);
                exit(EXIT_FAILURE);
            }
        }
    }
}

/*Esta función se ejecuta en un hilo separado para cada conexión de cliente aceptada.
Recibe datos del cliente y realiza acciones específicas en función de los datos recibidos.*/

void * socketThread(void *arg) {


    if ((recv(fd, &else2broker, sizeof(else2broker),0)) < 0){
        printf("recv failed");
    }
    else{

        if(else2broker.action == 0){
            pubs_conex();
        }
        //printf("TOPIC: %s\n", else2broker.topic);
        //printf("ACTION: %d\n", else2broker.action);
    }

}

/*Esta función se llama cuando un nuevo cliente publicador se conecta al servidor.
 * Envía una respuesta de vuelta al publicador con un ID único.*/
void pubs_conex(){

    struct timespec time_ex;

    clock_gettime(CLOCK_REALTIME, &time_ex);
    double pub_t = time_ex.tv_nsec;


    printf("[%ld.%ld] Nuevo cliente (%d) Publicador conectado : %s \n",time_ex.tv_sec,time_ex.tv_nsec,pub_fd ,else2broker.topic );

    broker2pub.id = pub_fd;
    broker2pub.response_status = OK;

    printf("ID: %d\n",broker2pub.id );
    printf("STATUS: %d\n",broker2pub.response_status);

    if( send(pub_fd , &broker2pub , sizeof(broker2pub) , 0) < 0){
        printf("Send failed\n");
        exit(EXIT_FAILURE);
    }
}

/*Esta función se utiliza para enviar un mensaje de registro al servidor con un tema específico.
Actualiza la estructura pub2broker con el tema proporcionado y envía el mensaje al servidor.*/

void data_pub_message(char* topic){
    struct timespec time_ex;

    clock_gettime( CLOCK_REALTIME , &time_ex);
    double pub_t = time_ex.tv_nsec;


    strcpy(pub2broker.topic, topic);

    if( send(fd_socket , &pub2broker , sizeof(pub2broker) , 0) < 0){
        printf("Send failed\n");
        exit(EXIT_FAILURE);
    }

    if( recv(fd_socket , &broker2pub_response , sizeof(broker2pub_response) , 0) < 0){
        printf("Send failed\n");
        exit(EXIT_FAILURE);
    }

    //printf("ID: %d\n", broker2pub_response.id);
    //printf("STATUS: %d\n", broker2pub_response.response_status);

    printf("[%ld.%ld] Registrado correctamente con ID: %d para topic %s\n",time_ex.tv_sec,time_ex.tv_nsec,broker2pub_response.id,pub2broker.topic );
}





void server_closing(){

    close(fd_socket);
}

void clients_closing(){

    close(fd_socket);
}

void data_sub_message(char* topic){
    struct timespec time_ex;

    clock_gettime( CLOCK_REALTIME , &time_ex);
    double pub_t = time_ex.tv_nsec;


    strcpy(else2broker.topic, topic);

    if( send(fd_socket , &sub2broker , sizeof(sub2broker) , 0) < 0){
        printf("Send failed\n");
        exit(EXIT_FAILURE);
    }

    if( recv(fd_socket , &broker2sub_response , sizeof(broker2sub_response) , 0) < 0){
        printf("Send failed\n");
        exit(EXIT_FAILURE);
    }

    printf("ID: %d\n", broker2sub_response.id);
    printf("STATUS: %d\n", broker2sub_response.response_status);

    printf("[%ld.%ld] Registrado correctamente con ID: %d para topic %s\n",time_ex.tv_sec,time_ex.tv_nsec,broker2pub_response.id,pub2broker.topic );

}