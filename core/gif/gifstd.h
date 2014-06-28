#include <stdint.h>



/// Just a "convenience type"
typedef unsigned long ulong;

/// Main pixel format for palettes and output
#pragma pack(push, 1)
typedef union _BGRA {
    struct {
        uint8_t B, G, R, A;
    };
    uint32_t BGRA;
} BGRA;
#pragma pack(pop)

/// GIF animation
typedef struct _ASTD {
    uint8_t *bptr; /// index data storage
    uint32_t xdim, /// frame width
             ydim, /// frame height
             fcnt, /// frame count
            *time; /// frame delays
    BGRA    *bpal; /// palette
} ASTD;



ASTD *MakeAnimStd(char *name);
void FreeAnimStd(ASTD **anim);
