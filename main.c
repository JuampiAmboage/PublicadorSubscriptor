#include <stdio.h>
#include "./broker.c"
#include "./subscriber.c"
#include "./publisher.c"

int main(int argc, char* argv[]) {

    /*printf("%d\n", argc);
    for (int i = 0; i<argc; i++) {
        printf("%s\n", argv[i]);
    }*/

    // no funca asi
    broker(argc, argv);
    publisher(argc, argv);
    subscriber(argc, argv);


    return 0;
}