#include <stdint.h>

#ifdef _WIN32
    #ifdef LIB_MAKE
        #define LIB_OPEN __attribute__((dllexport))
    #else
        #define LIB_OPEN __attribute__((dllimport))
    #endif
#else
    #define LIB_OPEN __attribute__((visibility("default")))
#endif



#define BRT_RSTD 0
#define BRT_ROGL 1

#define WIN_IBGR 1
#define WIN_IPBO 2



#pragma pack(push, 1)
typedef struct _T2FV {
    float x, y;
} T2FV;

typedef struct _T3FV {
    float x, y, z;
} T3FV;

typedef struct _T4FV {
    float x, y, z, w;
} T4FV;

typedef struct _T2UV {
    uint32_t x, y;
} T2UV;

typedef struct _T3UV {
    uint32_t x, y, z;
} T3UV;

typedef struct _T4UV {
    uint32_t x, y, z, w;
} T4UV;
#pragma pack(pop)



/** _________________________________________________________________________
    Callback function for the main program to modify the display list. Called
    each frame. This is just a typedef, not an actual function definition.
    _________________________________________________________________________
    DATA: display list; shares the format with <DATA> uniform (see the source
          in core/ogl/shad.c, look for "main vertex shader" tag), except that
          the lower 18 bits of Y belong to the element`s UUID
    TIME: pointer to the current time value in ms (may update asynchronously)
    FLGS: mouse button flags (0 = released, 1 = pressed)
          bit 0: left
          bit 1: middle
          bit 2: right
    XPTR: cursor X coordinate, relative to the window`s upper left corner
    YPTR: cursor Y coordinate, relative to the window`s upper left corner
    ISEL: index of the element under cursor in the existing list, < 0 if none
 **/
typedef void (*UFRM)(T2UV *data, uint64_t *time, uint32_t flgs,
                     int32_t xptr, int32_t yptr, int32_t isel);



/** _________________________________________________________________________
    Initializes the renderer.
    _________________________________________________________________________
    RNDR: renderer type
          BRT_RSTD - CPU
          BRT_ROGL - GPU
    XDIM: pointer to window width value (updated if the value is 0)
    YDIM: pointer to window height value (updated if the value is 0)
    FLGS: OS specific flags
          WIN_IBGR - [WIN32] use BGRA
          WIN_IPBO - [WIN32] use PBO
 **/
LIB_OPEN long EngineInitialize(uint32_t rndr,
                               uint32_t *xdim, uint32_t *ydim, uint32_t flgs);

/** _________________________________________________________________________
    Reads animations asynchronously. All pointers (except PATH) are to remain
    valid during runtime: they may be updated long after the call. The values
    that are initially passed in pointers are rewritten without being read.
    UUID will contain a positive integer on successful loading, 0 on error.
    _________________________________________________________________________
    PATH: full path to the loaded animation in UTF8 format
    UUID: animation`s uinque identifier
    XDIM: animation`s width, unchanged
    YDIM: animation`s height, unchanged
    FCNT: animation`s frame count
    TIME: animation`s frame delays array, allocated and freed internally
 **/
LIB_OPEN void EngineLoadAnimAsync(uint8_t *path, uint32_t *uuid,
                                  uint32_t *xdim, uint32_t *ydim,
                                  uint32_t *fcnt, uint32_t **time);

/** _________________________________________________________________________
    Ensures loading completion. After the call, all UUIDs, dimensions, delays
    and frame counts become valid.
    _________________________________________________________________________
 **/
LIB_OPEN void EngineFinishLoading();

/** _________________________________________________________________________
    Executes the main loop, frees all internal memory chunks on exit.
    _________________________________________________________________________
    FUNC: callback function described above (see UFRM typedef)
    MSEC: delay between frames in ms
    SIZE: initial display list size in elements
 **/
LIB_OPEN void EngineRunMainLoop(UFRM func, uint32_t msec, uint32_t size);



typedef typeof(EngineLoadAnimAsync) LOAD;
