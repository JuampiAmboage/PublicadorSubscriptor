#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>

#define MAX_TOPICS 10
#define MAX_PUBLISHERS 100
#define MAX_SUBSCRIBERS 900
#define MAX_QUEUE_SIZE 100

typedef struct {
    int id;
    char message[100];
    int topic;
} Message;

typedef struct {
    int id;
    int topic;
} Subscriber;

typedef struct {
    int topic;
    int messageCount;
    int messageIndex;
    sem_t mutex;
    sem_t messageSem;
    Message messages[MAX_QUEUE_SIZE];
    Subscriber subscribers[MAX_SUBSCRIBERS];
    pthread_barrier_t barrier;
} Topic;

Topic topics[MAX_TOPICS];
sem_t topicMutex;

void initTopics() {
    int i;
    for (i = 0; i < MAX_TOPICS; i++) {
        topics[i].topic = i;
        topics[i].messageCount = 0;
        topics[i].messageIndex = 0;
        sem_init(&topics[i].mutex, 0, 1);
        sem_init(&topics[i].messageSem, 0, 0);
        pthread_barrier_init(&topics[i].barrier, NULL, 2);
    }
    sem_init(&topicMutex, 0, 1);
}

void* publisher(void* args) {
    int publisherId = *(int*)args;
    int topic = rand() % MAX_TOPICS;
    char message[100];
    sprintf(message, "Mensaje del publicador %d", publisherId);

    sem_wait(&topicMutex);
    Topic* selectedTopic = &topics[topic];
    sem_post(&topicMutex);

    sem_wait(&selectedTopic->mutex);
    if (selectedTopic->messageCount < MAX_QUEUE_SIZE) {
        Message newMessage;
        newMessage.id = publisherId;
        newMessage.topic = topic;
        sprintf(newMessage.message, "%s", message);

        selectedTopic->messages[selectedTopic->messageCount] = newMessage;
        selectedTopic->messageCount++;

        sem_post(&selectedTopic->messageSem);
    }
    sem_post(&selectedTopic->mutex);

    pthread_barrier_wait(&selectedTopic->barrier);

    pthread_exit(NULL);
}

void* subscriber(void* args) {
    int subscriberId = *(int*)args;
    int topic = rand() % MAX_TOPICS;

    sem_wait(&topicMutex);
    Topic* selectedTopic = &topics[topic];
    sem_post(&topicMutex);

    Subscriber newSubscriber;
    newSubscriber.id = subscriberId;
    newSubscriber.topic = topic;

    sem_wait(&selectedTopic->mutex);
    selectedTopic->subscribers[selectedTopic->messageCount] = newSubscriber;
    sem_post(&selectedTopic->mutex);

    pthread_barrier_wait(&selectedTopic->barrier);

    sem_wait(&selectedTopic->mutex);
    Message message = selectedTopic->messages[selectedTopic->messageIndex];
    selectedTopic->messageIndex++;
    sem_post(&selectedTopic->mutex);

    printf("Suscriptor %d recibió el mensaje: %s\n", subscriberId, message.message);

    pthread_exit(NULL);
}

int main() {
    int i;

    initTopics();

    pthread_t publishers[MAX_PUBLISHERS];
    pthread_t subscribers[MAX_SUBSCRIBERS];

    for (i = 0; i < MAX_PUBLISHERS; i++) {
        int *publisherId = malloc(sizeof(int));
        *publisherId = i;
        pthread_create(&publishers[i], NULL, publisher, publisherId);
    }

    for (i = 0; i < MAX_SUBSCRIBERS; i++) {
        int *subscriberId = malloc(sizeof(int));
        *subscriberId = i;
        pthread_create(&subscribers[i],NULL,subscriber,subscriberId);
    }
}