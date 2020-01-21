#ifndef DIAG_H
#define DIAG_H

#include <stdio.h>
#include <string.h>
#include "shared.h"
#include "types.h"
#include "nbvec.h"

#define printfx(x, format, ...) printf("\r\033[%dC" format, (x), __VA_ARGS__)
int n_printed_lines;

void peek_books() {
    book_msg_t *books = (book_msg_t*)calloc(book_cap, sizeof(book_msg_t));
    int n_books = 0;
    printf("BOOKS: ");
    for (int i = 1; i <= book_cap; i++)
        if(msgrcv(msqid, &books[n_books], sizeof(book_t), i, IPC_NOWAIT) != -1)
            ++n_books;
    for (int i = 0; i < n_books; i++) {
        book_t book = books[i].book;
        printf("%ld: %d%c%c%s", books[i].mtype, book.author, book.text[0], book.text[1], (i==n_books-1)?"":", ");
        msgsnd(msqid, &books[i], sizeof(book_t), 0);
    }
    printf("\nEND\n");
    free(books);
}

void print_state() {
    int i, j;
    int field_len = 2;
    for (i = 0; i < n_printed_lines; i++)
        printf("\033[2K\n");
    printf("\033[%dA", n_printed_lines);
    printf("R: %d\tW: %d\tSEM0: %d\tid/state/last/left\n", *n_readers, *n_writers, semctl(semid, 0, GETVAL));
    for (i=0; i < n_proc; i++) {
        printfx(1+i*field_len, "%d", i);
    }
    printf("\n");
    for (i=0; i < n_proc; i++) {
        char st = states[i].state;
        printfx(1+i*field_len, "%c", st);
    }
    printf("\n");
    for (i=0; i < n_proc; i++) {
        char b = states[i].b;
        printfx(1+i*field_len, "%d", b);
    }
    printf("\n");
    for (i=0; i < n_proc; i++) {
        printfx(1+i*field_len, "%x", states[i].dedicated);
    }
    for (i=0; i < book_cap; i++) {
        printf("\n%d(%d):\t", i+1, vecs[i].nset);
        for (j=0; j < n_proc; j++) {
            if (isbset(&vecs[i], j))
                printf("%2d ", j);
            
        }
    }
    printf("\n");    
    printf("\033[%dA", n_printed_lines);
}

#endif