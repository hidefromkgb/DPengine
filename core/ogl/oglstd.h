#include <core.h>
#include "common.h"



typedef struct _ROGL {
    FVBO *surf;
    T4FV *temp;
    T4FV  disz;
    T4FV  hitd;
    long  size;
} ROGL;



long MakeRendererOGL(ROGL **rndr, UNIT *uarr, ulong rgba,
                     ulong uniq, ulong size, ulong xscr, ulong yscr);
void DrawRendererOGL(ROGL *rndr, UNIT *uarr, T4FV *data,
                     ulong size, ulong opaq);
void FreeRendererOGL(ROGL **rndr);
