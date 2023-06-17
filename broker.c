#ifdef __linux__
#include <arpa/inet.h>
#elif _WIN32
#include <Winsock2.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "proxy/proxyBroker.h"
#include <getopt.h> //para getopt_long
#include <signal.h>

int terminated = 0;

struct sockaddr_in getServer(int client_or_server);

//estructura del tipo sockaddr para server, guarda info del server
struct sockaddr_in server;

pthread_t brokerThreadRegistrator;
pthread_t brokerThreadNotifier;

int publishersIds[100];

void sigintHandler(int sig_num){
    signal(SIGINT, sigintHandler);
    terminated = 1;

    pthread_cancel(brokerThreadRegistrator);
    pthread_cancel(brokerThreadNotifier);

    serverClosing();
    fflush(stdout);
}

void* handleRegistrations(){
    while(!terminated){
        acceptClient();
        processNewRegistration(publishersIds);
    }
}

void* handlePublications(){
    lookForPublications(publishersIds);
}


int main(int argc, char *argv[]) {
    signal(SIGINT, sigintHandler);
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

    server = getServer(1);
    printf("---> MODE: %s\n\n",mode);

    connectServer(server);

    pthread_create(&brokerThreadRegistrator, NULL,(void *) handleRegistrations,NULL);
    pthread_create(&brokerThreadNotifier, NULL,(void *) handlePublications, NULL);

    pthread_join(brokerThreadNotifier,NULL);
    pthread_join(brokerThreadRegistrator,NULL);

    return 0;
}
