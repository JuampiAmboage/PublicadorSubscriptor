#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
//#include <sys/socket.h>
//#include <netinet/in.h>
#include <Winsock2.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
//#include <arpa/inet.h>
#include <errno.h>
#include <pthread.h>

#include "proxy/proxy.h"
#include "./process/process.h"


#include <getopt.h> //para getopt_long

struct sockaddr_in getDetail(int client_or_server);

int main(int argc, char *argv[]) {
    setbuf(stdout, NULL);

    int opt= 0;
    int port;
    char *ip,*topic;

    // create process
    // Process publisherProcess = initializeProcess();

    static struct option long_options[] = {
            {"ip",      required_argument,       0,  'a' },
            {"port", required_argument,       0,  'b' },
            {"topic",    required_argument, 0,  'c' },
    };

    int long_index =0;
    while ((opt = getopt_long(argc, argv,"abc",long_options, &long_index )) != -1) {
        switch (opt) {
            case 'a' :
                ip = optarg;

                break;
            case 'b' :
                port = atoi(optarg);
                break;
            case 'c' :
                topic = optarg;
                break;

            default:
                printf("?? getopt returned character code 0%o ??\n", opt);
                exit(EXIT_FAILURE);
        }
    }



    set_ip_port(ip,port);
    //estructura del tipo sockaddr para server, guarda info del server
    struct sockaddr_in server;

    server = getDetail(0);
    connect_client(server);

    data_pub_message(topic);


    return 0;
}

