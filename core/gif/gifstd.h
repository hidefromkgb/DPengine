#include "common.h"



/// GIF animation
typedef struct _ASTD {
    union {
        uint8_t *indx;
        BGRA    *bgra;
    } bptr;          /// pixel data storage
    unsigned fcnt,   /// frame count
             fcur,   /// current frame
            *time;   /// frame delays
    BGRA    *bpal;   /// palette
    long xdim, ydim; /// frame size
} ASTD;



ASTD *MakeAnimStd(char *name, int indx);
void FreeAnimStd(ASTD **anim);
