#include "../core.h"



#pragma pack(push, 1)
typedef struct _T2FV {
    float u, v;
} T2FV;

typedef struct _T3FV {
    float x, y, z;
} T3FV;

typedef struct _T4FV {
    float x, y, z, w;
} T4FV;

typedef struct _T2UV {
    uint32_t u, v;
} T2UV;

typedef struct _T3UV {
    uint32_t x, y, z;
} T3UV;

typedef struct _T4UV {
    uint32_t x, y, z, w;
} T4UV;
#pragma pack(pop)



long InitRendererOGL();
void MakeRendererOGL(ULIB *ulib, ulong uniq, T2UV *data, ulong size);
void SizeRendererOGL(ulong xscr, ulong yscr);
void DrawRendererOGL(T2UV *data, ulong size);
void FreeRendererOGL();
