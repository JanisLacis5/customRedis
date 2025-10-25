#include <cstdio>
#include "threadpool.h"

static void* work(void* arg) {
    ThreadPool* tp = (ThreadPool*)arg;

    while (true) {
        // Wait for something
        pthread_mutex_lock(&tp->mutex);
        while (tp->queue.empty()) {
            pthread_cond_wait(&tp->cond, &tp->mutex);
        }

        // If something is in the queue, execute
        ThreadPoolWork work = tp->queue.front();
        tp->queue.pop();

        pthread_mutex_unlock(&tp->mutex);
        work.f(work.arg);
    }
}

void threadpool_init(ThreadPool* tp, uint32_t thread_cnt) {
    int err = pthread_mutex_init(&tp->mutex, NULL);
    if (err != 0) {
        printf("Error in threadpool_init, mutex initialization\n");
        return;
    }
    err = pthread_cond_init(&tp->cond, NULL);
    if (err != 0) {
        printf("Error in threadpool_init, condition initialization\n");
        return;
    }

    tp->threads.resize(thread_cnt);
    for (size_t i = 0; i < thread_cnt; i++) {
        err = pthread_create(&tp->threads[i], NULL, &work, tp);
        if (err != 0) {
            printf("Error in threadpool_init, thread initialization\n");
            return;
        }
    }
}

void threadpool_produce(ThreadPool* tp, void (*f)(void*), void* arg) {
    pthread_mutex_lock(&tp->mutex);
    tp->queue.push(ThreadPoolWork{f, arg});
    pthread_cond_signal(&tp->cond);
    pthread_mutex_unlock(&tp->mutex);
}


