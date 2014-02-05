#include "gif/gifstd.h"

#define EMP_PICK (UNIT*)1



typedef struct _VEC2 {
    long x, y;
} VEC2;

typedef struct _PICT {
    BGRA *bptr;
    VEC2 size;
} PICT;

/// elementary animation unit
typedef struct _UNIT {
    void *anim;         /// animation data (the format may vary)
    char *path;         /// path to the original animation file
    VEC2  cpos,         /// position of the unit`s lower-left corner
          cptr;         /// cursor coords on mousedown (for dragging)
    ulong fcur,         /// current frame of the animation
          time,         /// current frame timestamp (in ms)
          scal,         /// scaling factor in powers of 2
          flgs;         /// unit flags (UCF_ prefix)
    struct _UNIT *prev, /// previous unit in the list
                 *next; /// next unit in the list
    struct _ULIB *ulib; /// link to the parent unit library
} UNIT;

/// unit library
typedef struct _ULIB {
    char *path;         /// the folder from which the library was built
    UNIT **uarr;        /// array of animation units (also a linked list)
    ulong ucnt,         /// length of the array
          uses,         /// number of library`s units in the main display list
          flgs;         /// library flags (ULF_ prefix)
    struct _ULIB *prev, /// previous library in the list
                 *next; /// next library in the list
} ULIB;

/// parameter structure for FillLibStdThrd()
typedef struct _FILL {
    ULIB *ulib;
    VEC2 scrn;
    long curr, load;
} FILL;

/// parameter structure for DrawPixStdThrd()
typedef struct _DRAW {
    UNIT *tail;
    PICT *pict;
    long ymin, ymax;
} DRAW;

/// unit common flags
/// unit library flags



extern uint32_t seed;



uint32_t PRNG(uint32_t *seed);
UNIT *SortByY(UNIT **tail);
UNIT *UpdateFrameStd(UNIT **tail, UNIT **pick, ulong *time, VEC2 cptr);

void FillLibStdThrd(FILL *fill);
void DrawPixStdThrd(DRAW *draw);

void MakeEmptyLib(ULIB **head, char *base, char *path);
void FreeUnitList(UNIT **tail, void (*adel)(void**));
void FreeLibList(ULIB **tail, void (*adel)(void**));
void UnitListFromLib(ULIB *ulib, UNIT **tail);
