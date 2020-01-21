#ifndef NBVEC_H
#define NBVEC_H

#include <stdlib.h>
#ifndef MAX_READERS
#define MAX_READERS 8
#endif
#define NB (((MAX_READERS)>>3)+((MAX_READERS)&7?1:0))

typedef struct{int nset; char vec[NB];} nbvec;

int bset(nbvec *vec, int i) {
    char *byte = (vec->vec + (i >> 3));
    char mask = 1 << (i & 0x7);
    if (!(*byte & mask)) {
        *byte |= mask;
        ++(vec->nset);
    }
    return vec->nset;
}

int bunset(nbvec *vec, int i) {
    char *byte = (vec->vec + (i >> 3));
    char mask = 1 << (i & 0x7);
    if (*byte & mask) {
        *byte ^= mask;
        --(vec->nset);
    }
    return vec->nset;
} 

int isbset(nbvec *vec, int i) {
    return *(vec->vec + (i >> 3)) & (1 << (i & 0x7)) ? 1 : 0;
}
#endif