#include <pthread.h>
#include <stdlib.h>

extern pthread_mutex_t mut, reindeer_mutex, santa_mutex;
extern pthread_cond_t santa_wakeup, reindeer_travel, reindeer_can_rest;
extern int reindeer_states[9], ready[2], average_rest_time, num_reindeer;
extern void random_sleep(int);

void* reindeer(void *p) {
    int id = *(int*)p;
    srand(time(NULL) + id);
    while(1) {
        reindeer_states[id] = 'r';
        random_sleep(average_rest_time);
        reindeer_states[id] = 'W';

        int npassed = 1;
        while (npassed) {
            pthread_mutex_lock(&mut);
            npassed = pthread_mutex_trylock(&reindeer_mutex); // to safely change state
            pthread_mutex_unlock(&mut);
        }
        ++num_reindeer;
        if (num_reindeer == 9) {
            pthread_mutex_unlock(&reindeer_mutex); // so others can enter
            pthread_mutex_lock(&santa_mutex); // ...because it will wait here
            ready[0] = 1;
            pthread_cond_signal(&santa_wakeup);
            pthread_mutex_lock(&reindeer_mutex); // so santa can't travel without this reindeer
            pthread_mutex_unlock(&santa_mutex);
        }
        
        reindeer_states[id] = 'w';
        pthread_cond_wait(&reindeer_travel, &reindeer_mutex); // at least 9 will reach this line before santa travels
        //santa has reindeer_mutex, signals travel and unlocks
        reindeer_states[id] = 'T';
        pthread_cond_wait(&reindeer_can_rest, &reindeer_mutex);
        --num_reindeer;
        pthread_mutex_unlock(&reindeer_mutex);
    }
}