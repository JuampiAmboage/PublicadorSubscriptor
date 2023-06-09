#ifdef __linux__
#include <arpa/inet.h>
#elif _WIN32
#include <Winsock2.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
//#include <sys/socket.h>
//#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <pthread.h>

#include "proxy/newProxy.h"


#include <getopt.h> //para getopt_long
#include <stdbool.h>

struct sockaddr_in getServer(int client_or_server);


int main(int argc, char *argv[]) {
    setbuf(stdout, NULL);


    int opt= 0;
    int port;
    char *mode;

    static struct option long_options[] = {
            {"port",      required_argument,       0,  'a' },
            {"mode", required_argument,       0,  'b' },
    };

    int long_index = 0;
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

    char* ip = "0.0.0.0"; // 0.0.0.0 / localhost
    setIpPort(ip, port);

    //estructura del tipo sockaddr para server, guarda info del server
    struct sockaddr_in server;

    server = getServer(1);
    printf("---> MODE: %s\n",mode);

    connectServer(server);

    defineMutex();
    while(1){
        int clientSocket = acceptClient();
        processNewRegistration(clientSocket);
    }

    destroyMutex();
    return 0;

}
