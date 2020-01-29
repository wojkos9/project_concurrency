#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>


pthread_mutex_t mut = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mut1 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t santa_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t elf_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t reindeer_mutex = PTHREAD_MUTEX_INITIALIZER;

pthread_cond_t reindeer_travel = PTHREAD_COND_INITIALIZER;
pthread_cond_t elves_consulted = PTHREAD_COND_INITIALIZER;
pthread_cond_t reindeer_can_rest = PTHREAD_COND_INITIALIZER;
pthread_cond_t elves_can_rest = PTHREAD_COND_INITIALIZER;
pthread_cond_t santa_wakeup = PTHREAD_COND_INITIALIZER;
int num_reindeer = 0;
int num_elves = 0;

char santa_state;
char ready[2];

int santa_op_time = 1000000;
int average_rest_time = 1000000;

void* santa(void *p) {

    pthread_mutex_lock(&santa_mutex);
    while(1) {
        if (ready[0]) {
            pthread_mutex_lock(&mut); // locks reindeer's entrance
            pthread_mutex_lock(&reindeer_mutex);
            ready[0] = 0;
            pthread_cond_broadcast(&reindeer_travel);
            pthread_mutex_unlock(&reindeer_mutex);

            santa_state = 'T';
            usleep(santa_op_time);
            santa_state = 'E';
            while (pthread_mutex_lock(&reindeer_mutex), num_reindeer) {
                pthread_cond_broadcast(&reindeer_can_rest);
                pthread_mutex_unlock(&reindeer_mutex);
            }
            santa_state = 'L';
            pthread_mutex_unlock(&reindeer_mutex);
            pthread_mutex_unlock(&mut);
        } else if (ready[1]) {
            pthread_mutex_lock(&mut1); // locks elves' entrance
            pthread_mutex_lock(&elf_mutex);
            ready[1] = 0;
            pthread_cond_broadcast(&elves_consulted);
            pthread_mutex_unlock(&elf_mutex);

            santa_state = 'c';
            usleep(santa_op_time);
            santa_state = 'e';
            while (pthread_mutex_lock(&elf_mutex), num_elves) {
                pthread_cond_broadcast(&elves_can_rest);
                pthread_mutex_unlock(&elf_mutex);
            }
            santa_state = 'l';
            
            pthread_mutex_unlock(&elf_mutex);
            pthread_mutex_unlock(&mut1);
        } else {
            santa_state = 'S';
            pthread_cond_wait(&santa_wakeup, &santa_mutex);
        }
    }
    pthread_mutex_unlock(&santa_mutex);

}

char reindeer_states[9];
char elf_states[10];


void* reindeer(void *p) {
    int id = *(int*)p;
    unsigned int seed = time(NULL)+id;

    while(1) {
        int t;
        reindeer_states[id] = 'r';
        t = rand_r(&seed)%20;
        usleep(t * average_rest_time/10);
        reindeer_states[id] = 'W';

        pthread_mutex_lock(&mut);
        pthread_mutex_lock(&reindeer_mutex);
        pthread_mutex_unlock(&mut);

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

void* elf(void *p) {
    int id = *(int*)p;
    unsigned int seed = time(NULL)+id+9;
    while(1) {
        int t;
        elf_states[id] = 'r';
        t = rand_r(&seed)%20;
        usleep(t * average_rest_time/10);
        elf_states[id] = 'W';

        pthread_mutex_lock(&mut1);
        pthread_mutex_lock(&elf_mutex);
        pthread_mutex_unlock(&mut1);
        
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

int main(int argc, char *argv[]) {
    int i;
    pthread_t threads[20];
    int ids[9];
    int elf_ids[10];

    if (argc > 1)
        santa_op_time = atoi(argv[1])*1000;

    if (argc > 2)
        average_rest_time = atoi(argv[2])*1000;

    for (i = 0; i < 9; i++) {
        ids[i] = i;
        pthread_create(&threads[i], NULL, reindeer, (void*)&ids[i]);
    }
    for (i = 0; i < 10; i++) {
        elf_ids[i] = i;
        pthread_create(&threads[9+i], NULL, elf, (void*)&elf_ids[i]);
    }
    pthread_create(&threads[19], NULL, santa, NULL);

    while(1) {
        printf("REINDEER\tELVES\tSANTA: %c\n", santa_state);
        for (i = 0; i < 10; i++) {
            if (i < 9)
                printf("%d:%c", 1+i, reindeer_states[i]);
            
            printf("\t\t%2d:%c\n", 1+i, elf_states[i]);
        }
        //printf("%s\t\t%s\n", ready[0]?"READY":"     ", ready[1]?"READY":"     ");
        printf("%c\t\t%c\n", santa_state=='T'?'X':' ', santa_state=='c'?'X':' ');
        printf("%2d\t\t%2d\n", num_reindeer, num_elves);
        usleep(1000000 / 60);
        printf("\033[13A\r");
    }

    for (i = 0; i < 20; i++) {
        pthread_join(threads[i], NULL);
    }

    return 0;
}