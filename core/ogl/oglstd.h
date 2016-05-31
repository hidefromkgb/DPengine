#include <core.h>



long MakeRendererOGL(RNDR **rndr, ulong rgba, UNIT *uarr,
                     ulong uniq, ulong size, ulong xscr, ulong yscr);
void DrawRendererOGL(RNDR *rndr, UNIT *uarr, T4FV *data,
                     ulong size, ulong opaq);
void FreeRendererOGL(RNDR **rndr);

FRBO *MakeRBO(long xdim, long ydim);
void BindRBO(FRBO *robj, long bind);
void ReadRBO(FRBO *robj, void *pict, ulong flgs);
void FreeRBO(FRBO **robj);
