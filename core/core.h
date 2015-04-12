#ifndef HDR_CORE
#define HDR_CORE

#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <engine.h>
#include <gif/gifstd.h>



#define _STRING(s) #s
#define STRING(s) _STRING(s)

#ifdef _WIN32
    #include <windows.h>
    #define THR_EXIT TRUE
    #define THR_FAIL FALSE
    #define THR_FUNC DWORD APIENTRY
    #define HDR_SEMD HANDLE *list;
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
#else
    #define THR_EXIT 0
    #define THR_FAIL 0
    #define THR_FUNC void *
    #define HDR_SEMD pthread_mutex_t cmtx; \
                     pthread_cond_t  cvar; \
                     SEM_TYPE list, full;
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



#ifndef min
#define min(a, b) (((a) < (b))? (a) : (b))
#endif

#ifndef max
#define max(a, b) (((a) > (b))? (a) : (b))
#endif



#define DEF_DSEP '/'

#define SEM_NULL 0
#define SEM_FULL ~0
#define SEM_TYPE uint64_t

#define MMI_RSTD 1
#define MMI_ROGL 2
#define MMI_OPAQ 3
#define MMI_STOP 4
#define MMI_HIDE 5
#define MMI_EXIT 6

#define TXT_HEAD  0
#define TXT_RSCM  1
#define TXT_SPEC  2
#define TXT_OPAQ  3
#define TXT_STOP  4
#define TXT_HIDE  5
#define TXT_EXIT  6
#define TXT_RSTD  7
#define TXT_ROGL  8
#define TXT_NONE  9
#define TXT_NOGL 10

#define TXT_CONS 11
#define TXT_IRGN 12
#define TXT_IBGR 13
#define TXT_IPBO 14
#define TXT_UOFO 15
#define TXT_UWGL 16

#define TXL_UANI "[%4u%c%c%c]"
#define TXL_UFPS "[%7.3f]"
#define TXL_EXIT "[//THR//]"
#define TXL_FAIL "[>>ERR<<]"
#define TXL_DUPL "[--DUP--]"
#define TXL_AEND "[==ANI==]"
#define TXL_ROGL "[++OGL++]"
#define TXL_RSTD "[++CPU++]"



/// AVL hash tree header
#define HDR_TREE \
    struct _TREE *coll, *prev, *next[2]; \
    long diff; \
    uint64_t hash;
typedef struct _TREE {
    HDR_TREE;
} TREE;

/// AVL hash tree iterator
typedef void (*ITER)(TREE*);

/// just a "convenience type"
typedef unsigned long ulong;

/// animation placement destination
typedef struct _DEST {
    struct _DEST *next;
    AINF *ainf;
} DEST;

/// filename tree element
typedef struct _SAVL {
    HDR_TREE;
    DEST *dest;
    char *path;
    typeof(((AINF*)0)->uuid) turn;
    struct _PAVL *epix;
} SAVL;

/// pixel-data tree element
typedef struct _PAVL {
    HDR_TREE;
    ASTD *anim;
    AINF  ainf;
    long  xoff, yoff, scal, tran;
} PAVL;

/// image wrapper, contains raw pixel data and dimensions
typedef struct _PICT {
    BGRA *bptr;
    ulong xdim, ydim;
} PICT;

/// elementary animation unit
typedef struct _UNIT {
    void *anim;    /// animation data (the format may vary)
    ulong scal;    /// scaling factor in powers of 2
    ulong tran;    /// number of semi-transparent pixels
    ulong offs[4]; /// offsets from the initial size: X_lf, X_rt, Y_up, Y_dn
} UNIT;

/// semaphore data
typedef struct _SEMD {
    HDR_SEMD;
} SEMD;

/// thread data
typedef struct _THRD {
    ulong loop;
    SEM_TYPE uuid;
    struct _ENGD *orig;
    void (*func)(struct _THRD*);
    union {
        long  ymin;
        SAVL *elem;
    };
    union {
        long  ymax;
        char *path;
    };
} THRD;

/// engine data
typedef struct _ENGD {
    uint64_t time,     /// current timestamp
             tfrm,     /// timestamp for the previous frame
             tfps;     /// timestamp for the previous FPS count
    ulong rscm,        /// rendering scheme
          flgs,        /// OS-specific options
          fram,        /// FPS counter
          msec,        /// frame delay
          draw,        /// rendering killswitch
          ncpu,        /// number of CPU cores the engine is allowed to occupy
          uniq,        /// number of unique animations
          size;        /// length of the main display list
    PICT  pict;        /// drawing area information
    UFRM  ufrm;        /// callback to update the state of a frame
    SEMD  isem,        /// incoming semaphore
          osem;        /// outgoing semaphore
    MENU *menu;        /// engine`s main menu
    THRD *thrd;        /// thread data array
    T4FV *data;        /// main display list (updated every frame)
    UNIT *uarr;        /// unique animations source in array form
    SAVL *hstr;        /// AVL tree for animation filename hashes & HPIX links
    PAVL *hpix;        /// AVL tree for animation pixel data (including hashes)
    void *rndr;        /// additional renderer (the exact type may vary)
    uint8_t **tran;    /// localized text array (ASCIIZ; last item is also 0)
    uintptr_t udat,    /// user-defined data to be passed to the frame updater
              user[4]; /// user-defined additional values, just in case
} ENGD;



long SelectUnit(UNIT *uarr, T4FV *data, long size, long xptr, long yptr);
void OutputFPS(ENGD *engd, char retn[]);

SEM_TYPE FindBit(SEM_TYPE inpt);
THR_FUNC ThrdFunc(THRD *data);
void StopThreads(ENGD *engd);
void SwitchThreads(ENGD *engd, long draw);

uint32_t LoadLocalization(uint8_t ***text, uint8_t *data, uint32_t size);
void ProcessMenuItem(MENU *item);
BGRA *ExtractRescaleSwizzleAlign(ASTD *anim, uint8_t swiz,
                                 long fram, long xdim, long ydim);



/// external functions, have to be implemented or imported
long CountCPUs();
uint64_t TimeFunc();
char *ConvertUTF8(char *utf8);
char *LoadFile(char *name, long *size);
MENU *OSSpecificMenu(ENGD *engd);
void MenuHandler(MENU *item);
void RestartEngine(ENGD *engd, ulong rscm);
void ShowMainWindow(ENGD *engd, ulong show);
void MakeThread(THRD *thrd);
void InitRenderer(ENGD *engd);
void RunMainLoop(ENGD *engd);

void FreeSemaphore(SEMD *retn, long nthr);
void MakeSemaphore(SEMD *retn, long nthr, SEM_TYPE mask);
long PickSemaphore(ENGD *engd, long open, SEM_TYPE mask);
SEM_TYPE WaitSemaphore(ENGD *engd, long open, SEM_TYPE mask);

#endif
