
broker: ./broker.c ./proxy/proxyBroker.o
	gcc -pthread broker.c ./proxy/proxyBroker.o -o broker

proxyBroker.o: ./proxy/proxyBroker.h ./proxy/proxyBroker.c
	gcc -pthread ./proxy/proxyBroker.h ./proxy/proxyBroker.c -o proxyBroker.o

publisher: ./publisher.c ./proxy/proxyPubSub.o
	gcc -pthread publisher.c ./proxy/proxyPubSub.o -o publisher

subscriber: ./subscriber.c ./proxy/proxyPubSub.o
	gcc -pthread subscriber.c ./proxy/proxyPubSub.o -o subscriber

proxyBroker.o: ./proxy/proxyPubSub.h ./proxy/proxyPubSub.c
	gcc -pthread ./proxy/proxyPubSub.h ./proxy/proxyPubSub.c -o proxyPubSub.o
