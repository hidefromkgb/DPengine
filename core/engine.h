#include <stdint.h>

#ifdef _WIN32
    #ifndef LIB_NONE
        #ifdef LIB_MAKE
            #define LIB_OPEN __attribute__((dllexport))
        #else
            #define LIB_OPEN __attribute__((dllimport))
        #endif
    #else
        #define LIB_OPEN
    #endif
#else
    #define LIB_OPEN __attribute__((visibility("default")))
#endif



#define SCM_QUIT 0
#define SCM_RSTD 1
#define SCM_ROGL 2

#define COM_IOPQ (1 << 31)

#define WIN_IBGR (1 << 0)
#define WIN_IPBO (1 << 1)
#define WIN_IRGN (1 << 2)

/// menu item is disabled
#define MFL_GRAY  (1 << 0)
/// current checkbox value
#define MFL_VCHK  (1 << 1)
/// menu item has a checkbox
#define MFL_CCHK  (1 << 2)
/// checkbox is a radiobutton
#define MFL_RCHK ((1 << 3) | MFL_CCHK)



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
             fcnt,   /// number of animation`s frames
            *time;   /// frame delays array, allocated and freed by the engine
    T2UV dims;       /// frame dimensions (actual, non-modified)
} AINF;
#pragma pack(pop)

typedef struct _MENU {
    uint8_t *text;
    void (*func)(struct _MENU *);
    uintptr_t data;
    uint32_t flgs, uuid;
    struct _MENU *chld;
} MENU;



/** _________________________________________________________________________
    Callback function for the main program to modify the display list. Called
    each frame, returns new length of the updated list.
    _________________________________________________________________________
    ENGH: handle of the engine object the call refers to
    USER: user-defined data pointer
    DATA: display list; shares the format with <DATA> uniform (see the source
          in core/ogl/shad.c, look for "main vertex shader" tag), except that
          W is just the element`s UUID
    TIME: pointer to the current time value in ms (may update asynchronously)
    FLGS: mouse button flags (0 = released, 1 = pressed)
          bit 0: left
          bit 1: middle
          bit 2: right
    XPTR: cursor X coordinate, relative to the window`s upper left corner
    YPTR: cursor Y coordinate, relative to the window`s upper left corner
    ISEL: index of the element under cursor in the existing list, < 0 if none
 **/
typedef uint32_t (*UFRM)(uintptr_t engh, uintptr_t user,
                         T4FV *data, uint64_t *time, uint32_t flgs,
                         int32_t xptr, int32_t yptr, int32_t isel);



/** _________________________________________________________________________
    Creates a menu from a given menu template and returns it.
    _________________________________________________________________________
    MENU: menu template
 **/
LIB_OPEN MENU *EngineMenuFromTemplate(MENU *tmpl);

/** _________________________________________________________________________
    Frees all memory that EngineMenuFromTemplate() used to create the menu.
    _________________________________________________________________________
    MENU: pointer to where the menu structure is located
 **/
LIB_OPEN void EngineFreeMenu(MENU **menu);

/** _________________________________________________________________________
    Shows the menu and lets the user make a choice.
    _________________________________________________________________________
    MENU: menu structure
 **/
LIB_OPEN void EngineOpenContextMenu(MENU *menu);

/** _________________________________________________________________________
    Allocates a new engine object and returns a handle to it.
    _________________________________________________________________________
 **/
LIB_OPEN uintptr_t EngineInitialize();

/** _________________________________________________________________________
    Reads animations asynchronously. All pointers (except PATH) are to remain
    valid till EngineFinishLoading() call, since they are only updated there.
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
    USER: user data pointer to be passed to the callback
    LANG: localization file name (0 if default)
    SIZE: initial display list size in elements
    FUNC: callback function described above (see UFRM typedef)
 **/
LIB_OPEN void EngineRunMainLoop(uintptr_t engh, uint32_t xdim, uint32_t ydim,
                                uint32_t  flgs, uint32_t msec, uint32_t rscm,
                                uintptr_t user, uint8_t *lang, uint32_t size,
                                UFRM func);

/** _________________________________________________________________________
    Frees all resources allocated by the target engine object and invalidates
    its handle.
    _________________________________________________________________________
    ENGH: handle of the engine object the call refers to
 **/
LIB_OPEN void EngineDeinitialize(uintptr_t engh);
