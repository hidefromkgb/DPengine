//#include <CoreFoundation/CoreFoundation.h>
#include <objc/objc-runtime.h>
#include <objc/message.h>
#include <objc/objc.h>
#include <sys/time.h>

#include <core.h>
#include <ogl/oglstd.h>



void RestartEngine(ENGD *engd, ulong rscm) {
    engd->draw = rscm;
}



void ShowMainWindow(ENGD *engd, ulong show) {
}



inline MENU *OSSpecificMenu(ENGD *engd) {
    return NULL;
}



void EngineOpenContextMenu(MENU *menu) {
}



char *LoadFile(char *name, long *size) {
    char *retn = 0;
    long file, flen;

    if ((file = open(name, O_RDONLY)) > 0) {
        flen = lseek(file, 0, SEEK_END);
        lseek(file, 0, SEEK_SET);
        retn = malloc(flen);
        read(file, retn, flen);
        close(file);
        if (size)
            *size = flen;
    }
    return retn;
}



char *ConvertUTF8(char *utf8) {
    return strdup(utf8);
}



long CountCPUs() {
    return min(sizeof(SEM_TYPE) * 8, max(1, sysconf(_SC_NPROCESSORS_ONLN)));
}



void MakeThread(THRD *thrd) {
    pthread_t pthr;
    pthread_create(&pthr, 0, (void *(*)(void*))ThrdFunc, thrd);
}



void FreeSemaphore(SEMD *retn, long nthr) {
    pthread_cond_destroy(&retn->cvar);
    pthread_mutex_destroy(&retn->cmtx);
}



void MakeSemaphore(SEMD *retn, long nthr, SEM_TYPE mask) {
    pthread_mutex_init(&retn->cmtx, 0);
    pthread_cond_init(&retn->cvar, 0);
    retn->full = (1 << nthr) - 1;
    retn->list = retn->full & mask;
}



long PickSemaphore(ENGD *engd, long open, SEM_TYPE mask) {
    SEMD *drop = (open)? &engd->osem : &engd->isem,
         *pick = (open)? &engd->isem : &engd->osem;

    open = (__sync_fetch_and_and(&drop->list, ~(drop->full & mask)) & mask)?
            TRUE : FALSE;
    __sync_or_and_fetch(&pick->list, pick->full & mask);

    pthread_mutex_lock(&pick->cmtx);
    pthread_cond_broadcast(&pick->cvar);
    pthread_mutex_unlock(&pick->cmtx);
    return open;
}



SEM_TYPE WaitSemaphore(ENGD *engd, long open, SEM_TYPE mask) {
    SEMD *wait = (open)? &engd->osem : &engd->isem;

    pthread_mutex_lock(&wait->cmtx);
    if (mask != SEM_NULL)
        while ((wait->list ^ wait->full) & mask)
            pthread_cond_wait(&wait->cvar, &wait->cmtx);
    else
        while (!wait->list)
            pthread_cond_wait(&wait->cvar, &wait->cmtx);
    mask = wait->list;
    pthread_mutex_unlock(&wait->cmtx);
    return mask;
}



uint64_t TimeFunc() {
    struct timeval spec = {};

    gettimeofday(&spec, 0);
    return spec.tv_sec * 1000 + spec.tv_usec / 1000;
}



void InitRenderer(ENGD *engd) {
    switch (engd->rscm) {
        case SCM_RSTD:
            SwitchThreads(engd, 1);
            break;

        case SCM_ROGL: {
            engd->rndr = MakeRendererOGL(engd->uarr, engd->uniq,
                                         engd->size, 0);
            SizeRendererOGL(engd->rndr, engd->pict.xdim, engd->pict.ydim);
            break;
        }
    }
}



#define C(x) objc_getClass(x)
#define N(x) sel_registerName(x)
#define M(o, ...) objc_msgSend(o, __VA_ARGS__)
#define W(r, o, ...) objc_msgSend_stret((void*)r, (void*)o, __VA_ARGS__)

#define countof(a) (sizeof(a) / sizeof(*(a)))

#define NSBorderlessWindowMask         (0     )
#define NSTitledWindowMask             (1 << 0)
#define NSClosableWindowMask           (1 << 1)
#define NSMiniaturizableWindowMask     (1 << 2)
#define NSResizableWindowMask          (1 << 3)
#define NSTexturedBackgroundWindowMask (1 << 8)

#define NSBackingStoreRetained    0
#define NSBackingStoreNonretained 1
#define NSBackingStoreBuffered    2

#define NSApplicationActivationPolicyRegular 1



typedef struct _OVER {
    char *name, *prms;
    void (*func);
} OVER;



void Overload(id whom, char *prev, char *next, OVER *with, long size) {
    Class chld = objc_allocateClassPair((Class)C(prev), next, 0);

    for (; size >= 0; size--)
        class_addMethod(chld,
                        N(with[size].name), with[size].func, with[size].prms);
    objc_registerClassPair(chld);
    M(whom, N("setDelegate:"),
      M(class_createInstance(chld, 0), N("autorelease")));
}



bool OnQuit(void *thrd) {
    printf("Bye-bye world =(\n");
    return true;
}



void RunMainLoop(ENGD *engd) {
    INCBIN("../core/icon.gif", MainIcon);

    struct objc_selector *R = N("autorelease");

    M(C("NSAutoreleasePool"), N("new"));
    id thrd = M(M(C("NSApplication"), N("sharedApplication")), R);
    M(thrd, N("setActivationPolicy:"), NSApplicationActivationPolicyRegular);

    struct {
        double x, y, w, h;
    } dims = {};
    W(&dims, M(M(C("NSScreen"), N("mainScreen")), R), N("visibleFrame"));

    id hwnd = M(M(M(C("NSWindow"), N("alloc")),
                  N("initWithContentRect:styleMask:backing:defer:"), dims,
                  NSClosableWindowMask | NSTitledWindowMask, NSBackingStoreBuffered, false), R);

    id bclr = M(M(C("NSColor"), N("colorWithCalibratedRed:green:blue:alpha:"),
                  0.0, 0.5, 1.0, 1.0 / 16.0), R);

    M(hwnd, N("setOpaque:"), false);
    M(hwnd, N("setHasShadow:"), false);
    M(hwnd, N("setBackgroundColor:"), bclr);
    M(hwnd, N("makeKeyAndOrderFront:"), NULL);

    OVER quit[] = {
        {"applicationShouldTerminateAfterLastWindowClosed:", 0, OnQuit},
    };
    Overload(thrd, "NSApplication", "NSAppExitDelegate", quit, countof(quit));

    M(thrd, N("activateIgnoringOtherApps:"), true);
    M(thrd, N("run"));

    /// never executed!
}
