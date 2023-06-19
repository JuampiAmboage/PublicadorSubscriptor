#ifdef __linux__
#include <arpa/inet.h>
#elif _WIN32
#include <Winsock2.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <getopt.h> //para getopt_long
#include <time.h>

#include "proxy/proxyPubSub.h"

int terminated = 0;

struct sockaddr_in getServer(int client_or_server);

void sigintHandler(int sig_num){
    signal(SIGINT, sigintHandler);
    terminated = 1;
    fflush(stdout);
}

void sleep_ms(unsigned int milliseconds) {
    struct timespec ts;
    ts.tv_sec = milliseconds / 1000;
    ts.tv_nsec = (milliseconds % 1000) * 1000000;

    nanosleep(&ts, NULL);
}

int main(int argc, char *argv[]) {
    signal(SIGINT, sigintHandler);
    setbuf(stdout, NULL);

    int opt = 0;
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

    setIpPort(ip,port);
    //estructura del tipo sockaddr para server, guarda info del server
    struct sockaddr_in server;



    server = getServer(0);
    printf("---> TOPIC: %s\n\n",topic);

    connectPublisher(server);

    if(sendPublisherRegistration(topic) < 0) {
        printf("No hay mas lugar para publicadores\n");
        exit(EXIT_FAILURE);
    }

    while(!terminated) {
        sleep_ms(1000);
        sendPublication(topic);
    }

    unregister(1);
    printf("TERMINADO\n");

    return 0;
}

