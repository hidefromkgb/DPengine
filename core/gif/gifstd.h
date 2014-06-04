#include <stdint.h>



typedef union _BGRA {
    struct {
        uint8_t B, G, R, A;
    };
    uint32_t BGRA;
} BGRA;



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



char *LoadFile(char *name, long *size);
ASTD *MakeAnimStd(char *name);
void FreeAnimStd(ASTD **anim);
