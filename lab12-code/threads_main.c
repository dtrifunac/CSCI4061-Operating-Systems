#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "queue.h"

#define USLEEP_DURATION 500000
#define NUM_MESSAGES 10
#define NUM_CONSUMERS 5

// Information passed to consumer threads
typedef struct {
    int idx;
    queue_t *queue;
} thread_args_t;

// Messages exchanged between threads via shared queue
char *messages[] = {
    "Akerman",
    "Blegen",
    "Campbell",
    "Diehl",
    "Eddy",
    "Ford",
    "Gibson",
    "Heller",
    "Iona",
    "Jackson"
};

// Producer thread start function
void *producer_thread_func(void *arg) {
    queue_t *queue = (queue_t *) arg;
    for (int i = 0; i < NUM_MESSAGES; i++) {
        if (queue_add(queue, messages[i]) == -1) {
            exit(1);
        }
        usleep(USLEEP_DURATION);
    }
    return NULL;
}

// Consumer thread start function
void *consumer_thread_func(void *arg) {
    thread_args_t *args = (thread_args_t *) arg;
    char buf[BUFLEN];
    for (int i = 0; i < NUM_MESSAGES / NUM_CONSUMERS; i++) {
        if (queue_remove(args->queue, buf) == -1) {
            exit(1);
        }
        printf("Consumer %d: %s\n", args->idx, buf);
    }
    return NULL;
}

int main() {
    queue_t queue;
    if (queue_init(&queue) != 0) {
        fprintf(stderr, "Failed to initialize queue\n");
        return 1;
    }

    // Create single producer thread
    int result;
    pthread_t producer_thread;
    if ((result = pthread_create(&producer_thread, NULL, producer_thread_func, &queue)) != 0) {
        fprintf(stderr, "pthread_create: %s\n", strerror(result));
        return 1;
    }

    // Create multiple producer threads
    pthread_t consumer_threads[NUM_CONSUMERS];
    thread_args_t thread_args[NUM_CONSUMERS];
    for (int i = 0; i < NUM_CONSUMERS; i++) {
        thread_args[i].queue = &queue;
        thread_args[i].idx = i;
        if ((result = pthread_create(consumer_threads + i, NULL, consumer_thread_func, thread_args + i)) != 0) {
            fprintf(stderr, "pthread_create: %s\n", strerror(result));
            return 1;
        }
    }

    // Join all threads
    // Even if an individual join fails, try to complete remaining joins
    int exit_code = 0;
    if ((result = pthread_join(producer_thread, NULL)) != 0) {
        fprintf(stderr, "pthread_join: %s\n", strerror(result));
        exit_code = 1;
    }
    for (int i = 0; i < NUM_CONSUMERS; i++) {
        if ((result = pthread_join(consumer_threads[i], NULL)) != 0) {
            fprintf(stderr, "pthread_join: %s\n", strerror(result));
            exit_code = 1;
        }
    }

    return exit_code;
}
