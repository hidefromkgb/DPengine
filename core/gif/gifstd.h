#include <stdint.h>
#include <stdlib.h>



/** main pixel format for palettes and output **/
#pragma pack(push, 1)
typedef union _BGRA {
    uint8_t  chnl[4];
    uint32_t bgra;
} BGRA;
#pragma pack(pop)

/** "standard" GIF animation type **/
typedef struct _ASTD {
    uint8_t *bptr; /** index data storage **/
    uint32_t xdim, /** frame width        **/
             ydim, /** frame height       **/
             fcnt, /** frame count        **/
            *time; /** frame delays       **/
    BGRA    *bpal; /** palette            **/
} ASTD;



ASTD *MakeAnimStd(char *data, long size);
void FreeAnimStd(ASTD **anim);
