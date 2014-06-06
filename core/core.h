#ifndef HDR_CORE
#define HDR_CORE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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
#define EMP_PICK ((UNIT*)1)

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



/// image wrapper, contains raw pixel data and dimensions
typedef struct _PICT {
    BGRA *bptr;
    long dimx, dimy;
} PICT;

/// elementary animation unit
typedef struct _UNIT {
    void *anim;         /// animation data (the format may vary)
    char *path;         /// path to the original animation file
    uint32_t hash,      /// path hash used for finding copies
             flgs;      /// unit flags (UCF_ prefix)
    long  posx, posy,   /// position of the unit`s lower-left corner
          ptrx, ptry;   /// cursor coords on mousedown (for dragging)
    ulong fcur,         /// current frame of the animation
          time,         /// current frame timestamp (in ms)
          scal,         /// scaling factor in powers of 2
          uuid;         /// unique unit identifier
    struct _UNIT *prev, /// previous unit in the list
                 *next, /// next unit in the list
                 *orig; /// original unit (NULL if this unit is not a copy)
    struct _ULIB *ulib; /// link to the parent unit library
} UNIT;

/// unit library
typedef struct _ULIB {
    char *path;         /// the folder from which the library was built
    UNIT **uarr;        /// array of animation units (also a linked list)
    ulong ucnt;         /// length of the array
    uint32_t flgs;      /// library flags (ULF_ prefix)
    struct _ULIB *prev, /// previous library in the list
                 *next; /// next library in the list
} ULIB;

/// parameter structure for FillLibStdThrd()
typedef struct _FILL {
    ULIB *ulib;
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
UNIT *UpdateFrameStd(UNIT **tail, UNIT **pick,
                     ulong *time, long xptr, long yptr);

void  FillLibStdThrd(FILL *fill);
void  DrawPixStdThrd(DRAW *draw);

void  MakeEmptyLib(ULIB **head, char *base, char *path);
void  FreeLibList(ULIB **head, void (*adel)(void**));
void  FreeUnitList(UNIT **tail, void (*adel)(void**));
void  UnitListFromLib(ULIB *ulib, UNIT **tail, ulong  uses,
                      long  dimx, long   dimy, ulong *uniq, ulong *size);

/// Imported from ./gif/common.c
char *LoadFile(char *name, long *size);

#endif
