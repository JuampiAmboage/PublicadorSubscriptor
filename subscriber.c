#include <errno.h>
#include <pthread.h>
#include <getopt.h> //para getopt_long
#include <stdio.h>
#include <stdlib.h>

#include "proxy/newProxy.h"
#include <Winsock2.h>



struct sockaddr_in getDetail(int client_or_server);

int main(int argc, char *argv[]) {
    setbuf(stdout, NULL);

    int opt= 0;
    int port;
    char *ip,*topic;

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
    printf("%s\n",ip );
    printf("%i\n",port );
    printf("%s\n",topic );


    set_ip_port(ip, port);
    struct sockaddr_in server;

    server = getDetail(0);
    connect_client(server);

    return 0;
}

