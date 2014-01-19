#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>



/// Palette flag (both global header and frame header)
#define FLG_FPAL 0x80
/// Interlace flag (frame header only)
#define FLG_FINT 0x40

/// MakeAnim() flags, described in FLGS section of MakeAnim() documentation
#define FLG_FILE 0x00
#define FLG_GGET 0x01
#define FLG_AIND 0x02



#pragma pack(push, 1)
typedef struct _GHDR {    /// ============== GLOBAL GIF HEADER ==============
    uint8_t head[6];      /// 'GIF87a' or 'GIF89a' signature
    uint16_t xdim, ydim;  /// total image width, total image height
    uint8_t flgs;         /** FLAGS:
                              GlobalPlt    bit 7     1: global palette exists
                                                     0: local in each frame
                              ClrRes       bit 6-4   bits/channel = ClrRes+1
                              [reserved]   bit 3     0
                              PixelBits    bit 2-0   |Plt| = 2 * 2^PixelBits
                          **/
    uint8_t bkgd;         /// background color index
    uint8_t aspr;         /// aspect ratio; usually 0
} GHDR;

typedef struct _FHDR {    /// ============== MAIN FRAME HEADER ==============
    uint16_t xoff, yoff;  /// offset of this frame in a "full" image
    uint16_t xdim, ydim;  /// frame width, frame height
    uint8_t flgs;         /** FLAGS:
                              LocalPlt     bit 7     1: local palette exists
                                                     0: global is used
                              Interlaced   bit 6     1: interlaced frame
                                                     0: non-interlaced frame
                              Sorted       bit 5     usually 0
                              [reserved]   bit 4-3   [undefined]
                              PixelBits    bit 2-0   |Plt| = 2 * 2^PixelBits
                          **/
} FHDR;

typedef struct _RGBX {
    uint8_t R, G, B;
} RGBX;

typedef union _BGRA {
    struct {
        uint8_t B, G, R, A;
    };
    uint32_t BGRA;
} BGRA;
#pragma pack(pop)



/** _________________________________________________________________________
    Initial reader. Returns raw GIF data (beginning with main header) if the
    stream is loaded, NULL otherwise. Can be left NULL if the implementation
    does not support anything else except file reading (which may be reading
    from resources or from raw byte streams).
    _________________________________________________________________________
    INPT: may be anything, from simple CHAR* to a complex structure of custom
          design, or even a single identifier. Anything that fits in VOID* :)
          If FLGS of MakeAnim() contains FLG_GGET, then this INPT will be the
          same INPT that was passed to MakeAnim(). Otherwise, GGET() will not
          be called at all.
 **/
typedef GHDR* (*GGET)(void *inpt);

/** _________________________________________________________________________
    Animation structure initializer. Returns positive values if the structure
    is successfully initialized, negative otherwise.
    _________________________________________________________________________
    GHDR: animation global header
    ANIM: implementation-specific data (i.e. a structure or a pointer to it)
    CFRM: the total number of frames in animation
 **/
typedef int (*GINI)(GHDR *ghdr, void *anim, int cfrm);

/** _________________________________________________________________________
    Decoded frame transferrer and frame delay setter. Returns positive values
    on success, negative otherwise. Note that it shall detect and recalculate
    interlaced pictures (FHDR->flgs & FLG_FINT).
    _________________________________________________________________________
    GHDR: animation global header
    FHDR: header of the resulting frame (the one just decoded)
    ANIM: implementation-specific data (i.e. a structure or a pointer to it)
    BPTR: decoded data pointer; data format is BGRA8888
    TRAN: transparent color index (or -1 if there`s none)
    TIME: next frame delay, in GIF time units (1 unit = 10 ms); can be 0
    CURR: index of the resulting frame
    FROM: the frame that serves as background for the next frame
          = 0: no frame, just background color
          > 0: [actual frame index] + 1
          < 0: no backing needed
 **/
typedef int (*GWFR)(GHDR *ghdr, FHDR *fhdr, void *anim, BGRA *bptr,
                    int tran, int time, int curr, int from);

/** _________________________________________________________________________
    Animation finalizer. Returns frame count if the animation is successfully
    finalized (i.e. all intermediate values are freed, etc.), zero otherwise.
    Can be left NULL if freeing is done elsewhere.
    _________________________________________________________________________
    DATA: the location where animation data resides
    ANIM: implementation-specific data (i.e. a structure or a pointer to it)
 **/
typedef int (*GPUT)(void *data, void *anim);



/** _________________________________________________________________________
    The main loading function. Returns 0 when the animation could not be read
    at all, or frame count in case of successful loading; otherwise the value
    returned is negative and equals -[index of the erroneous frame] - 1.
    _________________________________________________________________________
    INPT: ASCIIZ-string if FLGS (see below) does not have FLG_GGET, otherwise
          it may contain anything; see GGET() documentation given above
    FLGS: flags
          FLG_FILE: [DEFAULT FLAG] reading shall be performed from a file
          FLG_GGET: reading shall be routed through GGET()
          FLG_AIND: set pixels` alpha-components to their palette indices
    ANIM: implementation-specific data (i.e. a structure or a pointer to it)
    GGET,
    GINI,
    GWFR,
    GPUT: implementations of callback functions described above
 **/
int MakeAnim(void *inpt, int flgs, void *anim,
             GGET gget, GINI gini, GWFR gwfr, GPUT gput);
