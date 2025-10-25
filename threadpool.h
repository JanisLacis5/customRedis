#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <queue>
#include <stdint.h>
#include <pthread.h>

struct ThreadPoolWork {
    void (*f)(void* arg) = NULL;
    void* arg = NULL;
};

struct ThreadPool {
    std::vector<pthread_t> threads;
    std::queue<ThreadPoolWork> queue;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
};

void threadpool_init(ThreadPool* tp, uint32_t thread_cnt);
void threadpool_produce(ThreadPool* tp, void (*f)(void*), void* arg);

#endif
