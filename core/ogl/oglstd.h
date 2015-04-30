#include <core.h>
#include "common.h"



typedef struct _ROGL {
    FVBO *surf;
    T4FV *temp;
    T4FV  disz;
    T4FV  hitd;
    long  size;
} ROGL;



long MakeRendererOGL(ROGL **rndr, UNIT *uarr,
                     ulong uniq, ulong size, ulong rgba);
void DrawRendererOGL(ROGL *rndr, UNIT *uarr, T4FV *data,
                     ulong size, ulong opaq);
void SizeRendererOGL(ROGL *rndr, ulong xscr, ulong yscr);
void FreeRendererOGL(ROGL **rndr);
