#include <stdio.h>
#include <string.h>
#include "connection_queue.h"

int connection_queue_init(connection_queue_t *queue) {          //initialize a queue
    // queue->client_fds = malloc(sizeof(int)*CAPACITY);
    queue->length = 0;
    queue->read_idx = 0;
    queue->write_idx = -1;
    queue->shutdown = 0;
    int result;
    if((result=pthread_mutex_init(&queue->lock, NULL))!=0){     //initialize the lock
        fprintf(stderr,"pthread_mutex_init %s\n",strerror(result));
        return -1;
    }
    if((result=pthread_cond_init(&queue->queue_full, NULL))!=0){        //and condition variables for if the queue is full or empty
        fprintf(stderr,"pthread_cond_init %s\n",strerror(result));
        return -1;
    }
    if((result=pthread_cond_init(&queue->queue_empty, NULL))!=0){
        fprintf(stderr,"pthread_cond_init %s\n",strerror(result));
        return -1;
    }
    return 0;
}

int connection_enqueue(connection_queue_t *queue, int connection_fd) {      //add a file to the queue
    int result;
    if ((result = pthread_mutex_lock(&queue->lock)) != 0) {     //first lock the mutex
        fprintf(stderr, "pthread_mutex_lock: %s\n", strerror(result));
        return -1;
    }
    while (queue->length == CAPACITY) {     //wait if the queue is full before adding to it
        if ((result = pthread_cond_wait(&queue->queue_full, &queue->lock)) != 0) {
            fprintf(stderr, "pthread_cond_wait: %s\n", strerror(result));
            return -1;
        }
        if(queue->shutdown == 1){       //check if the signal to shut down was sent
            if ((result = pthread_mutex_unlock(&queue->lock)) != 0) {
                fprintf(stderr, "pthread_mutex_unlock: %s\n", strerror(result));
            }
        return -1;
    }
    }
    

    queue->length++;        //increase queue length and add file to it
    queue->write_idx = (queue->write_idx+1) % CAPACITY;
    queue->client_fds[queue->write_idx] = connection_fd;



    if ((result = pthread_cond_signal(&queue->queue_empty)) != 0) {     //signal that the queue is non empty for any potential threads waiting on it
        fprintf(stderr, "pthread_cond_signal: %s\n", strerror(result));
        pthread_mutex_unlock(&queue->lock);
        return -1;
    }
    if ((result = pthread_mutex_unlock(&queue->lock)) != 0) {       //release the mutex
        fprintf(stderr, "pthread_mutex_unlock: %s\n", strerror(result));
        return -1;
    }
    return 0;
}

int connection_dequeue(connection_queue_t *queue) {     //dequeue an item from the queue
    int result;
    if ((result = pthread_mutex_lock(&queue->lock)) != 0) {     //acquire mutex lock
        fprintf(stderr, "pthread_mutex_lock: %s\n", strerror(result));
        return -1;
    }

    while (queue->length == 0) {        //wait if the queue is empty and there is no file to dequeue
        if ((result = pthread_cond_wait(&queue->queue_empty, &queue->lock)) != 0) {
            fprintf(stderr, "pthread_cond_wait: %s\n", strerror(result));
            pthread_mutex_unlock(&queue->lock);
            return -1;
        }
        if(queue->shutdown == 1){       //check to see if the signal to shut down got sent
            if ((result = pthread_mutex_unlock(&queue->lock)) != 0) {
                fprintf(stderr, "pthread_mutex_unlock: %s\n", strerror(result));
            }
        return -1;
    }
    }
    

    queue->length--;        //decrement queue length and get the client file descriptor
    int fd = queue->client_fds[queue->read_idx];
    queue->read_idx = (queue->read_idx+1)%CAPACITY;


    if ((result = pthread_cond_signal(&queue->queue_full)) != 0) {      //signal to any waiting processes that the queue is not full
        fprintf(stderr, "pthread_cond_signal: %s\n", strerror(result));
        pthread_mutex_unlock(&queue->lock);
        return -1;
    }
    if ((result = pthread_mutex_unlock(&queue->lock)) != 0) {       //release the mutex
        fprintf(stderr, "pthread_mutex_unlock: %s\n", strerror(result));
        return -1;
    }
    return fd;
}

int connection_queue_shutdown(connection_queue_t *queue) {      //shut down the queue if a SIGINT got setn to main
    int result;
    if ((result = pthread_mutex_lock(&queue->lock)) != 0) {     //first lock to set queue->shutdown
        fprintf(stderr, "pthread_mutex_lock: %s\n", strerror(result));
        return -1;
    }
    queue->shutdown = 1;
    if ((result = pthread_cond_broadcast(&queue->queue_empty)) != 0) {      //broadcast to any waiting threads
        fprintf(stderr, "pthread_cond_broadcast: %s\n", strerror(result));
        return -1;
    }
    if ((result = pthread_cond_broadcast(&queue->queue_full)) != 0) {
        fprintf(stderr, "pthread_cond_broadcast: %s\n", strerror(result));
        return -1;
    }
    if ((result = pthread_mutex_unlock(&queue->lock)) != 0) {       //and release the lock
        fprintf(stderr, "pthread_mutex_unlock: %s\n", strerror(result));
        return -1;
    }
    
    return 0;
}

int connection_queue_free(connection_queue_t *queue) {      //destroy any mutexes or condition variables associated with the queue
    int result;
    if((result=pthread_mutex_destroy(&queue->lock))!=0){
        fprintf(stderr, "pthread_mutex_destroy: %s\n", strerror(result));
        pthread_cond_destroy(&queue->queue_empty);
        pthread_cond_destroy(&queue->queue_full);
        return -1;
    }
    if((result=pthread_cond_destroy(&queue->queue_empty))!=0){
        fprintf(stderr, "pthread_cond_destroy: %s\n", strerror(result));
        pthread_cond_destroy(&queue->queue_full);
        return -1;
    }
    if((result=pthread_cond_destroy(&queue->queue_full))!=0){
        fprintf(stderr, "pthread_cond_destroy: %s\n", strerror(result));
        return -1;
    }
    return 0;
}
