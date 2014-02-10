#include "gif/gifstd.h"



#ifndef max
#define max(a, b) (((a) > (b))? (a) : (b))
#endif

#ifndef min
#define min(a, b) (((a) < (b))? (a) : (b))
#endif



/// minimum allowed delay
#define MIN_WAIT 1
/// default frame delay
#define FRM_WAIT 40
/// signals the need to pick an object
#define EMP_PICK (UNIT*)1

/// if set, UNIT.ANIM and UNIT.PATH are copies, don`t free them
#define UCF_COPY 0x80000000
/// the unit is flipped horizontally
#define UCF_REVX 0x40000000
/// the unit is flipped vertically
#define UCF_REVY 0x20000000
/// the unit can be flipped horizontally
#define UCF_CANX 0x00800000
/// the unit can be flipped vertically
#define UCF_CANY 0x00400000
/// the unit is initially flipped horizontally
#define UCF_INIX 0x00200000
/// the unit is initially flipped vertically
#define UCF_INIY 0x00100000



/// two-dimensional integer vector
typedef struct _VEC2 {
    long x, y;
} VEC2;

/// image wrapper, contains raw pixel data and dimensions
typedef struct _PICT {
    BGRA *bptr;
    VEC2 size;
} PICT;

/// elementary animation unit
typedef struct _UNIT {
    void *anim;         /// animation data (the format may vary)
    char *path;         /// path to the original animation file
    uint32_t hash,      /// path hash used for finding copies
             flgs;      /// unit flags (UCF_ prefix)
    VEC2  cpos,         /// position of the unit`s lower-left corner
          cptr;         /// cursor coords on mousedown (for dragging)
    ulong fcur,         /// current frame of the animation
          time,         /// current frame timestamp (in ms)
          scal,         /// scaling factor in powers of 2
          prob,         /// unit relative probability
          mind,         /// minimum duration
          maxd;         /// maximum duration
    float dist,         /// movement per GIF time unit
          gone,         /// current movement accumulator
          xmov,         /// current movement X-component
          ymov;         /// current movement Y-component
    struct _UNIT *prev, /// previous unit in the list
                 *next; /// next unit in the list
    struct _ULIB *ulib; /// link to the parent unit library
} UNIT;

/// unit library
typedef struct _ULIB {
    char *path;         /// the folder from which the library was built
    UNIT **uarr;        /// array of animation units (also a linked list)
    ulong ucnt,         /// length of the array
          uses;         /// number of library`s units in the main display list
    uint32_t flgs;      /// library flags (ULF_ prefix)
    struct _ULIB *prev, /// previous library in the list
                 *next; /// next library in the list
} ULIB;

/// parameter structure for FillLibStdThrd()
typedef struct _FILL {
    ULIB *ulib;
    VEC2 scrn;
    long load, curr;
} FILL;

/// parameter structure for DrawPixStdThrd()
typedef struct _DRAW {
    UNIT *tail;
    PICT *pict;
    long ymin, ymax;
} DRAW;



extern uint32_t seed;



uint32_t PRNG(uint32_t *seed);
UNIT *SortByY(UNIT **tail);
UNIT *UpdateFrameStd(UNIT **tail, UNIT **pick, ulong *time, VEC2 cptr);

void FillLibStdThrd(FILL *fill);
void DrawPixStdThrd(DRAW *draw);

void MakeEmptyLib(ULIB **head, char *base, char *path);
void FreeLibList(ULIB **head, void (*adel)(void**));
void FreeUnitList(UNIT **tail, void (*adel)(void**));
void UnitListFromLib(ULIB *ulib, UNIT **tail);
