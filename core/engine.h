#include <stdint.h>

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
        ".global _"#pvar";"        \
        "_"#pvar":"                \
        ".incbin \""file"\";"      \
        "_"#pvar"_end:"            \
        ".byte 0;"                 \
        ".align 4;"                \
        ".section .text;"          \
    );                             \
    extern char pvar[]
#elif __APPLE__
    #define INCBIN(file, pvar)     \
    __asm__(                       \
        ".section __DATA,__data\n" \
        ".globl _"#pvar"\n"        \
        "_"#pvar":\n"              \
        ".incbin \""file"\"\n"     \
        "_"#pvar"_end:\n"          \
        ".byte 0\n"                \
        ".align 4\n"               \
        ".section __TEXT,__text\n" \
    );                             \
    extern char pvar[]
#else
    #define INCBIN(file, pvar)     \
    __asm__(                       \
        ".pushsection .data;"      \
        ".global "#pvar";"         \
        #pvar":"                   \
        ".incbin \""file"\";"      \
        #pvar"_end:"               \
        ".byte 0;"                 \
        ".align 4;"                \
        ".popsection;"             \
    );                             \
    extern char pvar[]
#endif



enum {
    ECB_INIT = 0,
    ECB_GUSR,
    ECB_GFLG,
    ECB_SFLG,
    ECB_DRAW,
    ECB_TEST,
    ECB_LOAD,
    ECB_QUIT,
};
enum {
    ELA_DISK = 0,
    ELA_LOAD,
    ELA_AINF,
};
#define COM_RGPU  (1 << 31)
#define COM_DRAW  (1 << 30)
#define COM_SHOW  (1 << 29)
#define COM_OPAQ  (1 << 28)
#define COM_DDDD  (COM_RGPU)  /** deferred flags mask **/

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

typedef union {
    struct {
        float x, y;
    };
    struct {
        float u, v;
    };
} T2FV;

typedef union {
    struct {
        float x, y, z;
    };
    struct {
        float r, g, b;
    };
} T3FV;

typedef union {
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

typedef union {
    struct {
        int32_t x, y;
    };
    struct {
        int32_t u, v;
    };
} T2IV;

typedef union {
    struct {
        int32_t x, y, z;
    };
    struct {
        int32_t r, g, b;
    };
} T3IV;

typedef union {
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
typedef struct {
    intptr_t uuid;   /// unique animation identifier (or data)
    uint32_t xdim,   /// frame width (actual, non-modified)
             ydim,   /// frame height (actual, non-modified)
             fcnt,   /// number of animation`s frames
            *time;   /// frame delays array, managed by the creator
} AINF;
#pragma pack(pop)



/** _________________________________________________________________________
    Callback function for the main program to modify the display list. Called
    each frame, returns new length of the updated list.
    _________________________________________________________________________
    ENGD: handle of the engine object the call refers to
    DATA: callee-managed display list; shares the format with <DATA> uniform
          (see comments in ./ogl/oglstd.c, look for "main vertex shader" tag)
          except that W is just the element`s UUID
    SIZE: total allocated length of DATA (i.e. maximum capacity)
    TIME: the current time value in msec
    USER: user-defined data (may be a pointer)
    ATTR: control attributes (UFR_ prefix)
    XPTR: cursor X coordinate, relative to the window`s upper left corner
    YPTR: cursor Y coordinate, relative to the window`s upper left corner
    ISEL: index of the element under cursor in the existing list, < 0 if none
 **/
typedef uint32_t (*UFRM)(ENGD *engd, T4FV **data, uint32_t *size,
                         uint64_t time, intptr_t user, uint32_t attr,
                         int32_t xptr, int32_t yptr, int32_t isel);

typedef uint32_t (*UFLG)(ENGD *engd, intptr_t user, uint32_t flgs);



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
          ECB_DRAW: "immediate" CPU-to-RAM draw call, DATA -> AINF:
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
LIB_OPEN void cEngineCallback(ENGD *engd, uint32_t ecba, intptr_t data);

/** _________________________________________________________________________
    Reads animations asynchronously. All AINF`s have to remain valid till the
    call to EngineCallback(ECB_LOAD), since they are only updated there.
    AINF::UUID will contain a positive integer on success, 0 on error.
    _________________________________________________________________________
    ENGD: handle of the engine object the call refers to
    AINF: pointer to the structure to receive animation properties
    NAME: unique animation identifier; must be a string
    DATA: data structure pointer; see FLGS
    FLGS: data types in DATA:
          ELA_DISK: full path to the animation in UTF8 format
          ELA_LOAD: preloaded GIF file data
          ELA_AINF: AINF structure, with AINF::UUID as a linearized BGRA
                    frame buffer, AINF::XDIM x (AINF::YDIM * AINF::FCNT)
    UDIS: data discarder function; usually just free()
 **/
LIB_OPEN void cEngineLoadAnimAsync(ENGD *engd, AINF *ainf,
                                   uint8_t *name, void *data, uint32_t flgs,
                                   void (*udis)(void*));

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
    UFRM: (see UFRM typedef)
    UFLG: (see UFLG typedef) [NOT NECESSARILY NEEDED, MAY BE 0]
 **/
LIB_OPEN void cEngineRunMainLoop(ENGD *engd, int32_t xpos, int32_t ypos,
                                 uint32_t xdim, uint32_t ydim,
                                 uint32_t flgs, uint32_t msec,
                                 intptr_t user, UFRM ufrm, UFLG uflg);
