#include <stdint.h>
#include <stdlib.h>



/// Palette flag (both global header and frame header)
#define GIF_FPAL 0x80
/// Interlace flag (frame header only)
#define GIF_FINT 0x40



#pragma pack(push, 1)
typedef struct _GHDR {    /// ============== GLOBAL GIF HEADER ==============
    uint32_t head;        /// 'GIF8' header signature
    uint16_t type;        /// '7a' or '9a', depending on the type
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
#pragma pack(pop)



/** _________________________________________________________________________
    Decoded frame transferrer and frame delay setter. NB: it has to recompute
    interlaced pictures based on GIF_FINT flag in FHDR->flgs: 1 = interlaced,
    0 = progressive.
    _________________________________________________________________________
    GHDR: animation global header
    FHDR: header of the resulting frame (the one just decoded)
    BACK: may take different values depending on the frame background mode
          0: no background needed (used in single-frame GIFs / first frames)
          1: background is a previous frame
          2: background is a frame before previous
          [actual FHDR]: previous frame + "hole" in the bounds of this FHDR
    CPAL: palette associated with the frame
    CLRS: number of colors in the palette
    BPTR: decoded array of color indices
    DATA: implementation-specific data (e.g. a structure or a pointer to it)
    NFRM: total frame count (may be partial; in this case it`s negative)
    TRAN: transparent color index (or -1 if there`s none)
    TIME: next frame delay, in GIF time units (1 unit = 10 ms); can be 0
    INDX: index of the resulting frame
 **/
typedef void (*GWFR)(GHDR *ghdr, FHDR *fhdr, FHDR *back, RGBX *cpal,
                     long clrs, uint8_t *bptr, void *data, long nfrm,
                     long tran, long time, long indx);



/** _________________________________________________________________________
    The main loading function. Returns the total number of frames if the data
    includes proper GIF ending, and otherwise it returns the number of frames
    loaded per current call, multiplied by -1. So, the data may be incomplete
    and in this case the function can be called again when more data arrives,
    just remember to keep SKIP up to date.
    _________________________________________________________________________
    DATA: raw data chunk, may be partial
    SIZE: size of the data chunk that`s currently present
    SKIP: number of frames to skip before resuming
    GWFR: callback function described above
    ANIM: implementation-specific data (e.g. a structure or a pointer to it)
 **/
long MakeAnim(void *data, long size, long skip, GWFR gwfr, void *anim);
