#ifndef HDR_CORE
#define HDR_CORE

#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include "engine.h"
#include "gif/gifstd.h"

#ifdef _WIN32
    #include <windows.h>
    #define THR_EXIT TRUE
    #define THR_FAIL FALSE
    #define THR_FUNC DWORD APIENTRY
    #define DEF_SEMD HANDLE *list;
    #define DEF_DSEP '/'
#else
    #define THR_EXIT
    #define THR_FAIL
    #define THR_FUNC void
    #define DEF_SEMD pthread_mutex_t cmtx; \
                     pthread_cond_t  cvar; \
                     SEM_TYPE list, full;
    #define DEF_DSEP '/'
#endif



#ifndef max
#define max(a, b) (((a) > (b))? (a) : (b))
#endif

#ifndef min
#define min(a, b) (((a) < (b))? (a) : (b))
#endif



#define TXT_EXIT "[//THR//]"
#define TXT_FFPS "[%3lu FPS]"
#define TXT_FANI "[%4u%c%c%c]"
#define TXT_FAIL "[>>ERR<<]"
#define TXT_DUPL "[--DUP--]"
#define TXT_AEND "[==ANI==] %lu objects, %lu ms: %0.3f ms/obj\n%s"
#define TXT_ROGL "[++OGL++]"
#define TXT_RSTD "[++CPU++]"

#define SEM_NULL 0
#define SEM_FULL ~SEM_NULL
#define SEM_TYPE uint64_t



/// hash tree data
typedef struct _TREE {
    uint64_t hash;
    long flgs, diff;
    union {
        struct {
            char *path;
            struct _TREE *epix;
        };
        struct {
            void *anim;
            uint32_t *xdim, *ydim, *uuid, *fcnt, **time,
                      xoff,  yoff,  scal;
        };
    };
    struct _TREE *fill, *coll, *prev, *next[2];
} TREE;

/// image wrapper, contains raw pixel data and dimensions
typedef struct _PICT {
    BGRA *bptr;
    ulong xdim, ydim;
} PICT;

/// elementary animation unit
typedef struct _UNIT {
    void *anim;    /// animation data (the format may vary)
    ulong scal;    /// scaling factor in powers of 2
    ulong offs[4]; /// offsets from the initial size: X_lf, X_rt, Y_up, Y_dn
} UNIT;

/// semaphore data
typedef struct _SEMD {
    DEF_SEMD;
} SEMD;

/// thread data
typedef struct _THRD {
    ulong loop;
    SEM_TYPE uuid;
    struct _TMRD *orig;
    void (*func)(struct _THRD*);
    union {
        ulong ymin;
        TREE *elem;
    };
    union {
        ulong ymax;
        char *path;
    };
} THRD;

/// timer data
typedef struct _TMRD {
    uint64_t time;
    SEMD isem, osem;
    ulong ncpu, rndr, draw, uniq, size, flgs;
    PICT  pict;
    UNIT *uarr;
    T2UV *data;
    THRD *thrd;
} TMRD;



extern TMRD tmrd;



uint32_t DownsampleAnimStd(ASTD *anim, uint32_t *xoff, uint32_t *yoff);
void RecolorPalette(BGRA *bpal, char *file, long size);
void DrawPixStdThrd(PICT *pict, UNIT *uarr, T2UV *data,
                    long size, long yinf, long ysup);

long TryUpdatePixTree(TREE *estr);

void FreeHashTrees();
void MakeUnitArray(UNIT **uarr);
void FreeUnitArray(UNIT **uarr);
long SelectUnit(UNIT *uarr, T2UV *data, long size, long xptr, long yptr);

SEM_TYPE FindBit(SEM_TYPE inpt);

THR_FUNC ThrdFunc(THRD *data);
void LTHR(THRD *data);
void PTHR(THRD *data);
void StopThreads(TMRD *tmrd);
void SwitchThreads(TMRD *tmrd, long draw);



/// external functions, have to be implemented or imported
uint64_t TimeFunc();
char *LoadFile(void *name);
void MakeThread(THRD *thrd);
void InitRenderer(TMRD *tmrd);
long PickSemaphore(TMRD *tmrd, long open, SEM_TYPE mask);
SEM_TYPE WaitSemaphore(TMRD *tmrd, long open, SEM_TYPE mask);


#endif
