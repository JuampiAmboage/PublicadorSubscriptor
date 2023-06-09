#ifndef PUBLICADORSUBSCRIPTOR_PROXY_H
#define PUBLICADORSUBSCRIPTOR_PROXY_H

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



void connect_server(struct sockaddr_in server);
void connect_client(struct sockaddr_in client);
void set_ip_port (char* ip, unsigned int port);
void set_port (unsigned int port);
void client_accept(char* mode);
void * socketThread(void *fd);
void data_pub_message(char* topic);
void pubs_conex();





void server_closing();
void clients_closing();



#endif //PUBLICADORSUBSCRIPTOR_PROXY_H
