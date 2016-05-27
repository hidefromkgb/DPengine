#include <core.h>



long MakeRendererOGL(RNDR **rndr, UNIT *uarr, ulong rgba,
                     ulong uniq, ulong size, ulong xscr, ulong yscr);
void DrawRendererOGL(RNDR *rndr, UNIT *uarr, T4FV *data,
                     ulong size, ulong opaq);
void FreeRendererOGL(RNDR **rndr);
