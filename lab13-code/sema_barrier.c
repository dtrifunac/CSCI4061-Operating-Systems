#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>

#define N_THREADS 10
#define MAX_DELAY 3000000

typedef struct {
    int id;
    unsigned delay;
} thread_args_t;

sem_t mutex_semaphore;
sem_t barrier_semaphore;

/*
 * Implements a thread barrier. When a thread calls barrier(), it will block
 * until all other threads have also reached and started a call to barrier().
 * Then, all threads are allowed to proceed.
 * Returns 0 on success or -1 on error.
 */
int barrier() {
    // TODO Implement the rest of this function
    // Uncomment the line below to use a static var to track waiting threads
    static int waiting_threads = 0;
    sem_wait(&mutex_semaphore);
    waiting_threads++;
    if(waiting_threads == N_THREADS){
        sem_post(&barrier_semaphore);
    }
    sem_post(&mutex_semaphore);
    sem_wait(&barrier_semaphore);
    sem_post(&barrier_semaphore);
    



    return 0;
}

void *thread_func(void * arg) {
    thread_args_t *arguments = (thread_args_t *) arg;
    usleep(arguments->delay);
    printf("Thread %d about to enter barrier\n", arguments->id);

    if (barrier() != 0) {
        fprintf(stderr, "Thread %d: barrier() failed\n", arguments->id);
        return NULL;
    }

    printf("Thread %d past barrier\n", arguments->id);
    return NULL;
}

int main() {
    // TODO Initialize semaphores here

    sem_init(&mutex_semaphore, 0, 1);
    sem_init(&barrier_semaphore, 0, 0);

    // Set up arguments for threads - an ID and a random initial delay
    srand(4061);
    thread_args_t thread_arguments[N_THREADS];
    for (int i = 0; i < N_THREADS; i++) {
        thread_arguments[i].id = i;
        thread_arguments[i].delay = rand() % MAX_DELAY;
    }

    // Start all threads
    pthread_t threads[N_THREADS];
    int result;
    for (int i = 0; i < N_THREADS; i++) {
        if ((result = pthread_create(threads + i, NULL, thread_func, thread_arguments + i)) != 0) {
            fprintf(stderr, "pthread_create: %s\n", strerror(result));
            sem_destroy(&mutex_semaphore);
            sem_destroy(&barrier_semaphore);
            return 1;
        }
    }

    // Wait for all threads to complete
    for (int i = 0; i < N_THREADS; i++) {
        if ((result = pthread_join(threads[i], NULL)) != 0) {
            fprintf(stderr, "pthread_join: %s\n", strerror(result));
            sem_destroy(&mutex_semaphore);
            sem_destroy(&barrier_semaphore);
            return 1;
        }
    }

    // TODO Clean up semaphores here
    sem_destroy(&mutex_semaphore);
    sem_destroy(&barrier_semaphore);
    return 0;
}
