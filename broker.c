#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <arpa/inet.h>
//#include <Winsock2.h>
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
    char *mode;

    // create process
    // Process brokerProcess = initializeProcess();

    static struct option long_options[] = {
            {"port",      required_argument,       0,  'a' },
            {"mode", required_argument,       0,  'b' },

    };

    int long_index =0;
    while ((opt = getopt_long(argc, argv,"ab",long_options, &long_index )) != -1) {
        switch (opt) {
            case 'a' :
                port = atoi(optarg);
                break;
            case 'b' :
                mode = optarg;
                break;

            default:
                printf("?? getopt returned character code 0%o ??\n", opt);
                exit(EXIT_FAILURE);
        }
    }


    printf("%i\n",port );
    printf("%s\n",mode );

    set_port(port);
    //estructura del tipo sockaddr para server, guarda info del server
    struct sockaddr_in server;
    server = getDetail(1);

    connect_server(server);

    while(1){
        client_accept(mode);
    }
    return 0;
}
