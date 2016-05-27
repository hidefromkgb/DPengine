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
        "_"STRING(pvar)"_end:"     \
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
        "_"STRING(pvar)"_end:\n"   \
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
        STRING(pvar)"_end:"        \
        ".byte 0;"                 \
        ".align 4;"                \
        ".popsection;"             \
    );                             \
    extern char pvar[]
#endif



#define ECB_INIT   0
#define ECB_GUSR   1
#define ECB_GFLG   2
#define ECB_SFLG   3
#define ECB_DRAW   4
#define ECB_LOAD   5
#define ECB_QUIT   6

#define COM_RGPU  (1 << 31)
#define COM_DRAW  (1 << 30)
#define COM_SHOW  (1 << 29)
#define COM_OPAQ  (1 << 28)

#define COM_HALT  (1 << 16)

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
/// engine data
typedef struct ENGD ENGD;

typedef unsigned long ulong;

typedef union _T2FV {
    struct {
        float x, y;
    };
    struct {
        float u, v;
    };
} T2FV;

typedef union _T3FV {
    struct {
        float x, y, z;
    };
    struct {
        float r, g, b;
    };
} T3FV;

typedef union _T4FV {
    struct {
        float x, y, z, w;
    };
    struct {
        float r, g, b, a;
    };
    struct {
        float xpos, ypos, xdim, ydim;
    };
} T4FV;

typedef union _T2IV {
    struct {
        int32_t x, y;
    };
    struct {
        int32_t u, v;
    };
} T2IV;

typedef union _T3IV {
    struct {
        int32_t x, y, z;
    };
    struct {
        int32_t r, g, b;
    };
} T3IV;

typedef union _T4IV {
    struct {
        int32_t x, y, z, w;
    };
    struct {
        int32_t r, g, b, a;
    };
    struct {
        int32_t xpos, ypos, xdim, ydim;
    };
} T4IV;

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
    ENGD: handle of the engine object the call refers to
    USER: user-defined data (may be a pointer)
    DATA: callee-managed display list; shares the format with <DATA> uniform
          (see comments in ./ogl/oglstd.c, look for "main vertex shader" tag)
          except that W is just the element`s UUID
    TIME: pointer to the current time value in ms (may update asynchronously)
    FLGS: mouse button flags (UFR_ prefix)
    XPTR: cursor X coordinate, relative to the window`s upper left corner
    YPTR: cursor Y coordinate, relative to the window`s upper left corner
    ISEL: index of the element under cursor in the existing list, < 0 if none
 **/
typedef uint32_t (*UFRM)(ENGD *engd, uintptr_t user,
                         T4FV **data, uint64_t *time, uint32_t flgs,
                         int32_t xptr, int32_t yptr, int32_t isel);



/** _________________________________________________________________________
    Provides the entry gate for all asynchronous engine functionality (except
    main loop execution and image loading itself), e.g. window manipulations.
    _________________________________________________________________________
    ENGD: handle of the engine object the call refers to
    ECBA: callback action to perform:
          ECB_INIT: initialize the new engine; may be called with ENGD == 0
                    DATA: pointer to the var that receives the new object
          ECB_GUSR: get the current user array
                    DATA: pointer to the var that receives the array
          ECB_GFLG: get the current state flags
                    DATA: pointer to the var that receives the flags
          ECB_SFLG: set the current state flags
                    DATA: new flags
          ECB_DRAW: "immediate" CPU-to-RAM draw call; DATA -> AINF:
                    UUID: ID of the target animation
                    XDIM: width of the draw area
                    YDIM: height of the draw area
                    FCNT: frame that needs to be drawn
                    TIME: pointer to the draw area
          ECB_LOAD: DATA != 0: interrupt the main loop to load more anims
                    DATA == 0: wait for anim loading completion
          ECB_QUIT: DATA != 0: just stop the main loop, do not deallocate
                    DATA == 0: terminate everything, deallocate resources
    DATA: accompanying data for the action; may be anything (see above)
 **/
LIB_OPEN void EngineCallback(ENGD *engd, uint32_t ecba, uintptr_t data);

/** _________________________________________________________________________
    Reads animations asynchronously. All AINF`s have to remain valid till the
    call to EngineCallback(ECB_LOAD), since they are only updated there.
    AINF::UUID will contain a positive integer on success, 0 on error.
    _________________________________________________________________________
    ENGD: handle of the engine object the call refers to
    PATH: full path to the loaded animation in UTF8 format
    LOAD: preloaded GIF data (if any), 0 otherwise
    AINF: pointer to the struxture to receive animation properties
 **/
LIB_OPEN void EngineLoadAnimAsync(ENGD *engd,
                                  uint8_t *path, uint8_t *load, AINF *ainf);

/** _________________________________________________________________________
    Executes the main loop.
    _________________________________________________________________________
    ENGD: handle of the engine object the call refers to
    XPOS: window position X
    YPOS: window position Y
    XDIM: window width
    YDIM: window height
    FLGS: flags
          COM_RGPU - GPU rendering (CPU otherwise)
          WIN_IBGR - [WIN32] use BGRA
          WIN_IPBO - [WIN32] use PBO
    MSEC: delay between frames in ms
    USER: user data pointer to be passed to the callback
    FUNC: callback function described above (see UFRM typedef)
 **/
LIB_OPEN void EngineRunMainLoop(ENGD *engd, int32_t xpos, int32_t ypos,
                                uint32_t xdim, uint32_t ydim, uint32_t flgs,
                                uint32_t msec, uintptr_t user, UFRM func);
