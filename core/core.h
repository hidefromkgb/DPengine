#include "GIF/gifstd.h"



typedef struct _VEC2 {
    long x, y;
} VEC2;

typedef struct _PICT {
    BGRA *bptr;
    VEC2 size;
} PICT;

typedef struct _INST {
    void *anim;
    char *path;
    VEC2 cpos, cptr;
    unsigned time, scal;
    struct _INST *prev, *next;
    void *user;
} INST;

#define EMP_PICK (INST*)1



extern uint32_t seed;



uint32_t PRNG(uint32_t *seed);
INST *SortByY(INST **tail);
INST *UpdateFrame(INST **tail, INST **pick, unsigned *time, VEC2 cptr);
int BlendPixStdThrd(INST *head, PICT *draw, VEC2 *scrn);
int MakeAnimStdThrd(INST *head, PICT *none, VEC2 *loop);
