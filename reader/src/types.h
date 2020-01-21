#ifndef TYPES_H
#define TYPES_H

typedef struct{short author; char text[2];} book_t;
typedef struct{char state; char b; short dedicated;} state_t;
typedef struct {long mtype; book_t book;} book_msg_t;

#endif