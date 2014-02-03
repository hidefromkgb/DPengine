#include "common.h"



/// Just a "convenience type"
typedef unsigned long ulong;

/// GIF animation
typedef struct _ASTD {
    union {
        uint8_t *indx;
        BGRA    *bgra;
    } bptr;          /// pixel data storage
    ulong fcnt,      /// frame count
         *time;      /// frame delays
    BGRA *bpal;      /// palette
    long xdim, ydim; /// frame size
} ASTD;



ASTD *MakeAnimStd(char *name, long indx);
void FreeAnimStd(ASTD **anim);
