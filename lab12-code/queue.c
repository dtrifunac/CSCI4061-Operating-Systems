#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "queue.h"

int queue_init(queue_t *queue) {
    queue->head = NULL;
    queue->size = 0;
    pthread_mutex_init(&queue->lock,NULL);
    pthread_cond_init(&queue->queue_full, NULL);
    pthread_cond_init(&queue->queue_empty, NULL);

    // TODO Initialize synchronization primitives here
    return 0;
}

int queue_add(queue_t *queue, const char *item) {
    // TODO Add synchronization logic before and after queue
    // modification code below as needed


    pthread_mutex_lock(&queue->lock);
    while(queue->size == MAX_QUEUE_SIZE){
        pthread_cond_wait(&queue->queue_full, &queue->lock);
    }
    if (queue->head == NULL) {
        if ((queue->head = malloc(sizeof(node_t))) == NULL) {
            fprintf(stderr, "malloc() failed\n");
            pthread_mutex_unlock(&queue->lock);
            return -1;
        }
        strncpy(queue->head->message, item, BUFLEN - 1);
        queue->head->message[BUFLEN - 1] = '\0';
        queue->head->next = NULL;
        queue->size = 1;
    } else {
        node_t *current = queue->head;
        while (current->next != NULL) {
            current = current->next;
        }
        if ((current->next = malloc(sizeof(node_t))) == NULL) {
            fprintf(stderr, "malloc() failed\n");
            pthread_mutex_unlock(&queue->lock);
            return -1;
        }
        strncpy(current->next->message, item, BUFLEN - 1);
        current->next->message[BUFLEN - 1] = '\0';
        current->next->next = NULL;
        queue->size++;
    }
    pthread_cond_signal(&queue->queue_empty);
    pthread_mutex_unlock(&queue->lock);

    return 0;
}

int queue_remove(queue_t *queue, char *item_dest) {
    // TODO Add synchronization logic before and after queue
    // modification code below as needed
    pthread_mutex_lock(&queue->lock);
    while (queue->size == 0) {
        pthread_cond_wait(&queue->queue_empty, &queue->lock);
    }
    strcpy(item_dest, queue->head->message);
    node_t *new_head = queue->head->next;
    free(queue->head);
    queue->head = new_head;
    queue->size--;
    pthread_cond_signal(&queue->queue_empty);
    pthread_mutex_unlock(&queue->lock);
    return 0;
}
