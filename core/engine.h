#include <stdint.h>

#define _STRING(s) #s
#define STRING(s) _STRING(s)

#define LIB_OPEN __attribute__((visibility("default")))

#ifdef _WIN32
    #undef  LIB_OPEN
    #ifndef LIB_NONE
        #ifdef LIB_MAKE
            #define LIB_OPEN __attribute__((dllexport))
        #else
            #define LIB_OPEN __attribute__((dllimport))
        #endif
    #else
        #define LIB_OPEN
    #endif
    #define INCBIN(file, pvar)     \
    __asm__(                       \
        ".section .data;"          \
        ".global _"STRING(pvar)";" \
        "_"STRING(pvar)":"         \
        ".incbin \""file"\";"      \
        ".byte 0;"                 \
        ".align 4;"                \
        ".section .text;"          \
    );                             \
    extern char pvar[]
#elif __APPLE__
    #define INCBIN(file, pvar)     \
    __asm__(                       \
        ".section __DATA,__data\n" \
        ".globl _"STRING(pvar)"\n" \
        "_"STRING(pvar)":\n"       \
        ".incbin \""file"\"\n"     \
        ".byte 0\n"                \
        ".align 4\n"               \
        ".section __TEXT,__text\n" \
    );                             \
    extern char pvar[]
#else
    #define INCBIN(file, pvar)     \
    __asm__(                       \
        ".pushsection .data;"      \
        ".global "STRING(pvar)";"  \
        STRING(pvar)":"            \
        ".incbin \""file"\";"      \
        ".byte 0;"                 \
        ".align 4;"                \
        ".popsection;"             \
    );                             \
    extern char pvar[]
#endif



#define SCM_QUIT 0
#define SCM_RSTD 1
#define SCM_ROGL 2

#define COM_IOPQ  (1 << 31)

#define WIN_IBGR  (1 << 0)
#define WIN_IPBO  (1 << 1)
#define WIN_IRGN  (1 << 2)

/// UFRM flags

/// left mouse button
#define UFR_LBTN  (1 << 0)
/// middle mouse button
#define UFR_MBTN  (1 << 1)
/// right mouse button
#define UFR_RBTN  (1 << 2)
/// mouse input enabled
#define UFR_MOUS  (1 << 3)



#pragma pack(push, 1)
typedef unsigned long ulong;

typedef struct _T2FV {
    float x, y;
} T2FV;

typedef struct _T3FV {
    float x, y, z;
} T3FV;

typedef struct _T4FV {
    float x, y, z, w;
} T4FV;

typedef struct _T2IV {
    int32_t x, y;
} T2IV;

typedef struct _T3IV {
    int32_t x, y, z;
} T3IV;

typedef struct _T4IV {
    int32_t x, y, z, w;
} T4IV;

typedef struct _T2UV {
    uint32_t x, y;
} T2UV;

typedef struct _T3UV {
    uint32_t x, y, z;
} T3UV;

typedef struct _T4UV {
    uint32_t x, y, z, w;
} T4UV;

/// animation unit info
typedef struct _AINF {
    uint32_t uuid,   /// unique animation identifier
             xdim,   /// frame width (actual, non-modified)
             ydim,   /// frame height (actual, non-modified)
             fcnt,   /// number of animation`s frames
            *time;   /// frame delays array, allocated and freed by the engine
} AINF;
#pragma pack(pop)



/** _________________________________________________________________________
    Callback function for the main program to modify the display list. Called
    each frame, returns new length of the updated list.
    _________________________________________________________________________
    ENGH: handle of the engine object the call refers to
    USER: user-defined data pointer
    DATA: callee-managed display list; shares the format with <DATA> uniform
          (see comments in ./ogl/oglstd.c, look for "main vertex shader" tag)
          except that W is just the element`s UUID
    TIME: pointer to the current time value in ms (may update asynchronously)
    FLGS: mouse button flags (UFR_ prefix)
    XPTR: cursor X coordinate, relative to the window`s upper left corner
    YPTR: cursor Y coordinate, relative to the window`s upper left corner
    ISEL: index of the element under cursor in the existing list, < 0 if none
 **/
typedef uint32_t (*UFRM)(uintptr_t engh, uintptr_t user,
                         T4FV **data, uint64_t *time, uint32_t flgs,
                         int32_t xptr, int32_t yptr, int32_t isel);



/** _________________________________________________________________________
    Allocates a new engine object and returns a handle to it.
    _________________________________________________________________________
 **/
LIB_OPEN uintptr_t EngineInitialize();

/** _________________________________________________________________________
    Reads animations asynchronously. All AINF`s have to remain valid till the
    call to EngineFinishLoading(), since they are only updated there.
    AINF::UUID will contain a positive integer on success, 0 on error.
    _________________________________________________________________________
    ENGH: handle of the engine object the call refers to
    PATH: full path to the loaded animation in UTF8 format
    AINF: pointer to the structure to receive animation properties
 **/
LIB_OPEN void EngineLoadAnimAsync(uintptr_t engh, uint8_t *path, AINF *ainf);

/** _________________________________________________________________________
    Blocks rendering and allows adding new sprites to the existing base.
    _________________________________________________________________________
    ENGH: handle of the engine object the call refers to
 **/
LIB_OPEN void EngineBeginAddition(uintptr_t engh);

/** _________________________________________________________________________
    Ensures loading completion. After the call, all UUIDs, dimensions, delays
    and frame counts become valid. Unblocks rendering.
    _________________________________________________________________________
    ENGH: handle of the engine object the call refers to
 **/
LIB_OPEN void EngineFinishLoading(uintptr_t engh);

/** _________________________________________________________________________
    Executes the main loop.
    _________________________________________________________________________
    ENGH: handle of the engine object the call refers to
    XDIM: window width
    YDIM: window height
    FLGS: OS specific flags
          WIN_IBGR - [WIN32] use BGRA
          WIN_IPBO - [WIN32] use PBO
    MSEC: delay between frames in ms
    RSCM: rendering scheme
          SCM_RSTD - CPU
          SCM_ROGL - GPU
    LANG: localization file name (0 if default)
    USER: user data pointer to be passed to the callback
    FUNC: callback function described above (see UFRM typedef)
 **/
LIB_OPEN void EngineRunMainLoop(uintptr_t engh, int32_t xpos, int32_t ypos,
                                uint32_t xdim, uint32_t ydim, uint32_t flgs,
                                uint32_t msec, uint32_t rscm, uintptr_t user,
                                UFRM func);

/** _________________________________________________________________________
    Frees all resources allocated by the target engine object and invalidates
    its handle.
    _________________________________________________________________________
    ENGH: handle of the engine object the call refers to
 **/
LIB_OPEN void EngineDeinitialize(uintptr_t engh);
