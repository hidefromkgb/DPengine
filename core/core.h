#ifndef HDR_CORE
#define HDR_CORE

#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <limits.h>

#include <engine.h>
#include <gif/gifstd.h>



#ifdef _WIN32
    #include <windows.h>
    #define THR_EXIT TRUE
    #define THR_FAIL FALSE
    #define THR_FUNC DWORD APIENTRY
#else
    #include <pthread.h>
    #define THR_EXIT 0
    #define THR_FAIL 0
    #define THR_FUNC void *
#endif

#define PFR_HALT (1 << 31)
#define PFR_SKIP (1 << 30)
#define PFR_PICK (1 << 29)



/// semaphore control type
typedef uint64_t SEM_TYPE;

/// semaphore data, defined externally
typedef struct SEMD SEMD;

/// elementary animation unit
typedef struct {
    void *anim;    /// animation data (the format may vary)
    ulong scal;    /// scaling factor in powers of 2
    ulong tran;    /// indicator of semi-transparent pixels
    ulong offs[4]; /// offsets from the initial size: X_lf, X_rt, Y_up, Y_dn
} UNIT;



uint32_t cPrepareFrame(ENGD *engd, long xptr, long yptr, uint32_t attr);
void cOutputFrame(ENGD *engd, long frbo);
void cDeallocFrame(ENGD *engd, long frbo);
void cOutputFPS(ENGD *engd, char retn[]);
SEM_TYPE cFindBit(SEM_TYPE inpt);
THR_FUNC cThrdFunc(void *user);



/// external functions, have to be implemented or imported
long lCountCPUs();
uint64_t lTimeFunc();
char *lLoadFile(char *name, long *size);
void lMakeThread(void *thrd);
void lRestartEngine(ENGD *engd);
void lShowMainWindow(ENGD *engd, long show);
void lRunMainLoop(ENGD *engd, long xpos, long ypos, long xdim, long ydim,
                  BGRA **bptr, intptr_t *data, uint32_t flgs);
void lFreeSemaphore(SEMD **retn, long nthr);
void lMakeSemaphore(SEMD **retn, long nthr, SEM_TYPE mask);
long lPickSemaphore(SEMD *drop, SEMD *pick, SEM_TYPE mask);
SEM_TYPE lWaitSemaphore(SEMD *wait, SEM_TYPE mask);

#endif
