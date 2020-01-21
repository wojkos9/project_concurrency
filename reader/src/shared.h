#ifndef SHARED_H
#define SHARED_H

#include <sys/msg.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "types.h"

#define SHM_KEY 123
#define MSQ_KEY 124
#define SEM_KEY 125

int msqid;
int shmid;
int semid;
void *shm_base;
state_t *states;
nbvec *vecs;
int *n_readers;
int *n_writers;

int n_proc;
int book_cap;

void raise_sem(int semnum) {
    static struct sembuf buf = {0, 1, 0};
    buf.sem_num = semnum;
    if (semop(semid, &buf, 1) == -1) {
        char msg[64];
        snprintf(msg, 64, "Raising sem %d on %d", semnum, semid);
        perror(msg);
    }
}
void lower_sem(int semnum) {
    static struct sembuf buf = {0, -1, 0};
    buf.sem_num = semnum;
    if (semop(semid, &buf, 1) == -1) {
        char msg[64];
        snprintf(msg, 64, "Lowering sem %d on %d", semnum, semid);
        perror(msg);
    }
}

int try_lower_sem(int semnum) {
    static struct sembuf buf = {0, -1, IPC_NOWAIT};
    buf.sem_num = semnum;
    if (semop(semid, &buf, 1) == -1) {
        if (errno != EAGAIN)
            perror("Trying to lower sem");
        return errno;
    }
    return 0;
}

void configure_shared(int add_flag) {
    int nb;
    msqid = msgget(MSQ_KEY, 0600|add_flag);
    if (msqid == -1) {
        perror("msq");
        exit(1);
    }
    nb = n_proc*sizeof(state_t)+book_cap*sizeof(nbvec) + 2*sizeof(int);
    shmid = shmget(SHM_KEY, nb, 0600|add_flag);
    if (shmid == -1) {
        perror("shmget");
        exit(1);
    }
    shm_base = shmat(shmid, NULL, 0);
    n_readers = (int*)shm_base;
    n_writers = n_readers+1;
    states = (state_t*)(n_writers+1);
    vecs = (nbvec*)(states+n_proc);
    

    semid = semget(SEM_KEY, 8, 0600|add_flag);
    if (semid == -1) {
        perror("semget");
        exit(1);
    }
}

extern int n_printed_lines;
void wait_and_clean(int unused) {
    while(wait(NULL) > 0);
    shmdt(shm_base);
    shmctl(shmid, IPC_RMID, NULL);
    semctl(semid, 0, IPC_RMID);
    msgctl(msqid, IPC_RMID, NULL);
    printf("\033[%dB", n_printed_lines);
    exit(0);
}

#endif