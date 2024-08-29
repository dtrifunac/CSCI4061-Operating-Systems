#ifndef QUEUE_H
#define QUEUE_H

#include <pthread.h>

#define MAX_QUEUE_SIZE 5
#define BUFLEN 512

typedef struct node {
    char message[BUFLEN];
    struct node *next;
} node_t;

typedef struct {
    node_t *head;
    unsigned size;
    pthread_mutex_t lock;
    pthread_cond_t queue_full;
    pthread_cond_t queue_empty;
} queue_t;

int queue_init(queue_t *queue);

int queue_add(queue_t *queue, const char *item);

int queue_remove(queue_t *queue, char *item_dest);

#endif // QUEUE_H
