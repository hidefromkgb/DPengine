#ifndef HDR_CORE
#define HDR_CORE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "engine.h"
#include "gif/gifstd.h"



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



extern TREE *(*LoadUnitStdThrd)(TREE *itmp, char *path);



uint32_t DownsampleAnimStd(ASTD *anim, uint32_t *xoff, uint32_t *yoff);
void RecolorPalette(BGRA *bpal, char *file, long size);
void DrawPixStdThrd(PICT *pict, UNIT *uarr, T2UV *data,
                    long size, long yinf, long ysup);

long TryUpdatePixTree(TREE *estr);
void TryLoadUnit(uint8_t *path, uint32_t *uuid, uint32_t *xdim, uint32_t *ydim,
                 uint32_t *fcnt, uint32_t **time);

void MakeUnitArray(UNIT **uarr);
void FreeUnitArray(UNIT **uarr);
long SelectUnit(UNIT *uarr, T2UV *data, long size, long xptr, long yptr);

char *LoadFile(char *name, long *size);



#endif
