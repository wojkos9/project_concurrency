#include <pthread.h>
#include <stdlib.h>

extern pthread_mutex_t mut1, elf_mutex, santa_mutex;
extern pthread_cond_t santa_wakeup, elves_consulted, elves_can_rest;
extern int elf_states[10], ready[2], average_rest_time, num_elves;
extern void random_sleep(int);

void* elf(void *p) {
    int id = *(int*)p;
    srand(time(NULL)+id+9);
    while(1) {
        int t;
        elf_states[id] = 'r';
        random_sleep(average_rest_time);
        elf_states[id] = 'W';
        
        int npassed = 1;
        while (npassed) {
            pthread_mutex_lock(&mut1);
            npassed = pthread_mutex_trylock(&elf_mutex); // to safely change state
            pthread_mutex_unlock(&mut1);
        }
        
        ++num_elves;
        if (num_elves == 3) {
            pthread_mutex_unlock(&elf_mutex); // so others can enter
            pthread_mutex_lock(&santa_mutex); // ...because it will wait here
            ready[1] = 1;
            pthread_cond_signal(&santa_wakeup);
            pthread_mutex_lock(&elf_mutex); // so santa can't travel without this elf
            pthread_mutex_unlock(&santa_mutex);
        }
        
        elf_states[id] = 'w';
        pthread_cond_wait(&elves_consulted, &elf_mutex); // at least 3 will reach this line before santa travels
        //santa has elf_mutex, signals travel and unlocks
        elf_states[id] = 'C';
        pthread_cond_wait(&elves_can_rest, &elf_mutex);
        --num_elves;
        pthread_mutex_unlock(&elf_mutex);
    }
}