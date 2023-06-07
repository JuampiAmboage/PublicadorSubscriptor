

#ifndef PUBLICADORSUBSCRIPTOR_NEWPROXY_H
#define PUBLICADORSUBSCRIPTOR_NEWPROXY_H


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


struct sockaddr_in getServer(int client_or_server);
void trySocketCreation();
void tryServerConnection(struct sockaddr_in server);
void trySendingMessage(struct message toSend);
struct timespec connectClient(struct sockaddr_in server);
void connectPublisher(struct sockaddr_in server);
void connectSubscriber(struct sockaddr_in server);
struct timespec registerClient(char* topic);
void registerPublisher(char* topic);
void registerSubscriber(char* topic);
void processNewRegistration(char* mode);
void serverClosing();
//void * socketThread(void *fd);






void server_closing();
void clients_closing();


#endif //PUBLICADORSUBSCRIPTOR_NEWPROXY_H