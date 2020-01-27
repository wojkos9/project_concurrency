#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>
#include <getopt.h>
#include <sys/wait.h>
#include <errno.h>

#include "nbvec.h"
#include "shared.h"
#include "diag.h"


int t_read, t_write, t_rest;

void random_sleep(int t) {
    usleep((int)(2.f*t/20 * (rand()%20)));
}

int publish(book_t book) {
    int book_id;
    int i;
    book_msg_t book_msg;
    nbvec *vec;

    //lower_sem(0);
    for (book_id = 0; book_id < book_cap; book_id++) {
        lower_sem(2);
        if (vecs[book_id].nset == 0)
            goto found_id;
        raise_sem(2);
    }
    raise_sem(0);
    return -2;

    found_id:  // sem 2 lowered
    book_msg.mtype = book_id + 1;
    book_msg.book = book;
    vec = &vecs[book_id];
    for (i = 0; i < n_proc; i++) {
        if ((states[i].state&0xdf) == 'R') {
            bset(vec, i);
            lower_sem(5);
            states[i].dedicated++;
            raise_sem(5);
        }  
    }
    
    if (vec->nset > 0) { // if there are any readers
        msgsnd(msqid, &book_msg, sizeof(book_t), 0);
        raise_sem(2);
        return book_msg.mtype;
    } else {
        raise_sem(0);
        raise_sem(2);
        return -1;
    }
}

int read_book(int id) {
    int book_id;
    book_msg_t book_msg;
    nbvec *vec;
    for (book_id = 0; book_id < book_cap; book_id++) {
        if (isbset(&vecs[book_id], id)) {
            if (msgrcv(msqid, &book_msg, sizeof(book_t), book_id+1, IPC_NOWAIT) != -1) {
                goto found_book;
            }
        }
    }
    return 0;
    found_book:
    random_sleep(t_read);
    lower_sem(5);
    states[id].dedicated--;
    raise_sem(5);
    vec = &vecs[book_id];
    lower_sem(2);
    if (bunset(vec, id) != 0) {
        msgsnd(msqid, &book_msg, sizeof(book_t), 0);
    }
    else {
        raise_sem(0);
    }
    raise_sem(2);
    return book_id+1;
}

void reader(int id) {
    // 0 - can publish
    // 1 - can enter the library
    // 2 - lock space on bookshelf
    // 3 - protect n_readers
    // 4 - protect service queue (prevents starvation)
    // 5 - protect n_writers
    char state;
    srand(time(NULL)+id);
    configure_shared(0);
    state = 0;
    while (1) {
        random_sleep(t_rest);
        state = rand()%2 ? 'R' : 'W';
        states[id].state = state;
        if (state == 'R') { // reader
            lower_sem(4);
            lower_sem(3);
            if (*n_readers == 0)
                lower_sem(1);
            (*n_readers)++;
            raise_sem(4);
            raise_sem(3);
            
            states[id].state = 'r';
            states[id].b = read_book(id);

            lower_sem(3);
            (*n_readers)--;
            states[id].state = 'R';
            if (*n_readers == 0)
                raise_sem(1);
            raise_sem(3);
        } else { // writer
            book_t book;
            int bid; // book id

            // [lower bookshelf semaphore before even attempting to write]
            if (try_lower_sem(0) == EAGAIN)
                continue;

            lower_sem(4);
            lower_sem(1);

            // only needed for diagnostics
            lower_sem(5);
            (*n_writers)++;
            raise_sem(5);

            raise_sem(4);

            states[id].state = 'w';
            
            // read (at most) 1 book
            states[id].b = read_book(id);

            // create a book
            //states[id].state = 'b';
            random_sleep(t_write);
            book.author = id;
            book.text[0] = 'A'+rand()%26;
            book.text[1] = 'a'+rand()%26;
            

            // publish
            bid = publish(book);
            
            if (bid == -1) {
                states[id].state = 'X';
            } else if (bid == -2) {
                fprintf(stderr, "ERROR. Bookshelf unexpectedly full.\n"); // impossible
                states[id].state = 'D';
                exit(1);
            }
            lower_sem(5);
            (*n_writers)--;
            raise_sem(5);
            
            // [release the lock on library]
            raise_sem(1);
        }
    }


}


void parse_times(char *s_times) {
    char *ptr;
    int i = 0;
    int *times[] = {&t_read, &t_write, &t_rest};
    while ((ptr = strtok(s_times, ",")) != (char*)-1 && i < sizeof(times)/sizeof(times[0])) {
        *times[i++] = atof(ptr) * 1000;
    }
}

int main(int argc, char* argv[]) {
    int pp;
    char c;
    pp = 1;
    n_proc = 10;
    book_cap = 12;

    while ((c = getopt(argc, argv, "n:b:pt:")) != -1) {
        switch (c)
        {
        case 'n':
            n_proc = atoi(optarg);
            break;
        case 'b':
            book_cap = atoi(optarg);
            break;
        case 'p':
            pp = 1-pp;
            break;
        case 't':
            parse_times(optarg);
            break;
        default:
            break;
        }
    }

    n_printed_lines = book_cap+5;
    
    configure_shared(IPC_CREAT);
    
    memset(states, 0, n_proc * sizeof(state_t));
    memset(vecs, 0, book_cap * sizeof(nbvec));

    semctl(semid, 0, SETVAL, book_cap);
    semctl(semid, 1, SETVAL, 1);
    semctl(semid, 2, SETVAL, 1);
    semctl(semid, 3, SETVAL, 1);
    semctl(semid, 4, SETVAL, 1);
    semctl(semid, 5, SETVAL, 1);
    for (int i = 0; i < n_proc; i++) {
        if (fork() == 0)
            reader(i);
    }

    signal(SIGINT, wait_and_clean);

    while(1) {
        if (pp != 0)
            print_state();
        usleep(1000000/60);
    }
    return 0;
}