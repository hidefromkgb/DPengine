#include "common.h"
#include <string.h>



/// Just a "convenience type"
typedef unsigned long ulong;

/// GIF animation
typedef struct _ASTD {
    uint8_t *bptr;   /// index data storage
    ulong fcnt,      /// frame count
         *time;      /// frame delays
    BGRA *bpal;      /// palette
    long xdim, ydim; /// frame size
} ASTD;



ASTD *MakeAnimStd(char *name);
void FreeAnimStd(ASTD **anim);
