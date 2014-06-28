#define _WIN32_WINNT 0x0501

#include <windows.h>

#include "../../core/core.h"
#include "../../core/ogl/core.h"



/// thread data
typedef struct _THRD {
    HANDLE *isem, *osem;
    ulong loop;
    void (*func)(struct _THRD*);
    union {
        struct _TMRD *orig;
        TREE *elem;
    };
    union {
        ulong ymin;
        char *path;
    };
    ulong ymax;
} THRD;

/// timer data
typedef struct _TMRD {
    HGLRC mwrc;
    HDC devc, mwdc;
    HWND hwnd, hogl;
    HBITMAP hdib;
    HANDLE *isem, *osem;
    UINT flgs;

    uint64_t time;
    ulong ncpu, fram, rndr, uniq, size;
    PICT  pict;
    UNIT *uarr;
    T2UV *data;
    THRD *thrd;
} TMRD;

/// renderbuffer-based framebuffer object
typedef struct _FRBO {
    GLuint fbuf,    /// framebuffer
           rbuf[2], /// renderbuffers for pixel and depth data
           pbuf[2]; /// pixel-transfer buffer array
    GLint  xdim,    /// width
           ydim,    /// height
           swiz;    /// pixel buffer switcher
} FRBO;



UFRM UpdateFrame;
TMRD tmrd;



FRBO *MakeRBO(long xdim, long ydim) {
    FRBO *retn = malloc(sizeof(FRBO));
    GLint data;

    retn->xdim = xdim;
    retn->ydim = ydim;
    retn->swiz = 0;

    glGenFramebuffers(1, &retn->fbuf);
    glBindFramebuffer(GL_FRAMEBUFFER, retn->fbuf);

    glGenRenderbuffers(2, retn->rbuf);
    glBindRenderbuffer(GL_RENDERBUFFER, retn->rbuf[0]);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA,
                          retn->xdim, retn->ydim);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                              GL_RENDERBUFFER, retn->rbuf[0]);
    glBindRenderbuffer(GL_RENDERBUFFER, retn->rbuf[1]);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16,
                          retn->xdim, retn->ydim);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                              GL_RENDERBUFFER, retn->rbuf[1]);
    data = retn->xdim * retn->ydim * 4;

    glGenBuffersARB(2, retn->pbuf);
    glBindBufferARB(GL_PIXEL_PACK_BUFFER_ARB, retn->pbuf[0]);
    glBufferDataARB(GL_PIXEL_PACK_BUFFER_ARB, data, NULL, GL_STREAM_READ_ARB);
    glBindBufferARB(GL_PIXEL_PACK_BUFFER_ARB, retn->pbuf[1]);
    glBufferDataARB(GL_PIXEL_PACK_BUFFER_ARB, data, NULL, GL_STREAM_READ_ARB);
    glBindBufferARB(GL_PIXEL_PACK_BUFFER_ARB, 0);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    return retn;
}



void ReadRBO(FRBO *robj, PICT *pict, ulong flgs) {
    void *bptr;

    if (flgs & WIN_IPBO)
        glBindBufferARB(GL_PIXEL_PACK_BUFFER_ARB, robj->pbuf[robj->swiz]);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, robj->fbuf);
    glReadPixels(0, 0, robj->xdim, robj->ydim,
                (flgs & WIN_IBGR)? GL_BGRA : GL_RGBA,
                 GL_UNSIGNED_BYTE, (flgs & WIN_IPBO)? NULL : pict->bptr);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);

    if (flgs & WIN_IPBO) {
        robj->swiz ^= 1;
        glBindBufferARB(GL_PIXEL_PACK_BUFFER_ARB, robj->pbuf[robj->swiz]);
        bptr = glMapBufferARB(GL_PIXEL_PACK_BUFFER_ARB, GL_READ_ONLY_ARB);
        if (bptr) {
            memcpy(pict->bptr, bptr, pict->xdim * pict->ydim * sizeof(BGRA));
            glUnmapBufferARB(GL_PIXEL_PACK_BUFFER_ARB);
        }
        glBindBufferARB(GL_PIXEL_PACK_BUFFER_ARB, 0);
    }
}



void FreeRBO(FRBO **robj) {
    if (robj && *robj) {
        glBindBufferARB(GL_PIXEL_PACK_BUFFER_ARB, 0);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glDeleteRenderbuffers(2, (*robj)->rbuf);
        glDeleteFramebuffers(1, &(*robj)->fbuf);
        glDeleteBuffersARB(2, (*robj)->pbuf);
        free(*robj);
        *robj = NULL;
    }
}



TREE *LoadUST(TREE *elem, char *path) {
    TREE *retn = NULL;
    long iter = WaitForMultipleObjects(tmrd.ncpu, tmrd.osem, FALSE, INFINITE);
    if ((iter -= WAIT_OBJECT_0) < MAXIMUM_WAIT_OBJECTS) {
        retn = tmrd.thrd[iter].elem;
        tmrd.thrd[iter].elem = elem;
        tmrd.thrd[iter].path = path;
        ResetEvent(*tmrd.thrd[iter].osem);
        SetEvent(*tmrd.thrd[iter].isem);
    }
    return retn;
}



void LTHR(THRD *data) {
    char *path, *apal, *file;
    long  iter;
    TREE *elem;
    ASTD *retn;

    retn = 0;
    path = data->path;
    elem = data->elem;
    if (path) {
        iter = strlen(path);
        if (((path[iter - 3] == 'g') || (path[iter - 3] == 'G'))
        &&  ((path[iter - 2] == 'i') || (path[iter - 2] == 'I'))
        &&  ((path[iter - 1] == 'f') || (path[iter - 1] == 'F'))
        &&  (retn = MakeAnimStd(path))) {
            *elem->epix->xdim = retn->xdim;
            *elem->epix->ydim = retn->ydim;
            *elem->epix->fcnt = retn->fcnt;
            *elem->epix->time = retn->time;
             elem->epix->scal = DownsampleAnimStd(retn, &elem->epix->xoff,
                                                        &elem->epix->yoff);
            apal = strdup(path);
            apal[iter - 3] = 'a';
            apal[iter - 2] = 'r';
            apal[iter - 1] = 't';
            file = LoadFile(apal, &iter);
            RecolorPalette(retn->bpal, file, iter);
            free(apal);
            free(file);
        }
        elem->epix->anim = retn;
    }
}



void PTHR(THRD *data) {
    DrawPixStdThrd(&data->orig->pict, data->orig->uarr, data->orig->data,
                   data->orig->size, data->ymin, data->ymax);
}



DWORD APIENTRY ThrdFunc(LPVOID data) {
    #define data ((THRD*)data)
    do {
        WaitForSingleObject(*data->isem, INFINITE);
        ResetEvent(*data->isem);
        if (data->loop)
            data->func(data);
        else
            printf(TXT_EXIT"\n");
        SetEvent(*data->osem);
    } while (data->loop);
    return TRUE;
    #undef data
}



LRESULT APIENTRY WindowProc(HWND hWnd, UINT uMsg, WPARAM wPrm, LPARAM lPrm) {
    switch (uMsg) {
        case WM_CREATE:
            SetTimer(hWnd, 1, 1000, 0);
            return 0;


        case WM_KEYDOWN:
            if (wPrm == VK_ESCAPE)
                SendMessage(hWnd, WM_CLOSE, 0, 0);
            return 0;


        case WM_LBUTTONDOWN:
            SetCapture(hWnd);
            return 0;


        case WM_LBUTTONUP:
            ReleaseCapture();
            return 0;


        case WM_TIMER: {
            static char fout[24];
            sprintf(fout, TXT_FFPS, tmrd.fram);
            SetWindowText(hWnd, fout);
            printf("%s\n", fout);
            tmrd.fram = 0;
            return 0;
        }

        case WM_DESTROY:
            KillTimer(hWnd, 1);
            PostQuitMessage(0);
            return 0;


        default:
            return DefWindowProc(hWnd, uMsg, wPrm, lPrm);
    }
}



void DeinitGL() {
    wglMakeCurrent(NULL, NULL);
    wglDeleteContext(tmrd.mwrc);
    ReleaseDC(tmrd.hogl, tmrd.mwdc);
    DeleteDC(tmrd.mwdc);
    DestroyWindow(tmrd.hogl);
    DestroyWindow(tmrd.hwnd);
}



LIB_OPEN long EngineInitialize(uint32_t rndr,
                               uint32_t *xdim, uint32_t *ydim, uint32_t flgs) {
    WNDCLASSEX wndc = {sizeof(wndc), CS_HREDRAW | CS_VREDRAW,
                       WindowProc, 0, 0, GetModuleHandle(NULL), NULL,
                       LoadCursor(NULL, IDC_HAND), NULL, NULL, "-", NULL};
    PIXELFORMATDESCRIPTOR ppfd = {sizeof(ppfd), 1,
                                  PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL |
                                  PFD_DOUBLEBUFFER, PFD_TYPE_RGBA, 32};
    GLint attr[] = {WGL_CONTEXT_MAJOR_VERSION_ARB, 3,
                    WGL_CONTEXT_MINOR_VERSION_ARB, 0,
                    WGL_CONTEXT_FLAGS_ARB,         0,
                    0};
    HGLRC APIENTRY (*wglMakeCtx)(HDC, HGLRC, GLint*);
    HGLRC rtmp;

    BITMAPINFO bmpi = {{sizeof(bmpi.bmiHeader), 0, 0,
                        1, 8 * sizeof(BGRA), BI_RGB}};
    SYSTEM_INFO syin = {};
    long iter;

    tmrd.rndr = rndr;
    tmrd.flgs = flgs;
    tmrd.pict.xdim =
        *xdim = (*xdim)? *xdim : GetSystemMetrics(SM_CXVIRTUALSCREEN);
    tmrd.pict.ydim =
        *ydim = (*ydim)? *ydim : GetSystemMetrics(SM_CYVIRTUALSCREEN);

    RegisterClassEx(&wndc);
    tmrd.hwnd = CreateWindowEx(WS_EX_LAYERED | WS_EX_TOPMOST,
                               wndc.lpszClassName, NULL,
                               WS_POPUP | WS_VISIBLE,
                               0, 0, tmrd.pict.xdim, tmrd.pict.ydim,
                               NULL, NULL, wndc.hInstance, NULL);
    switch (tmrd.rndr) {
        case BRT_RSTD:
            break;

        case BRT_ROGL:
            wndc.lpszClassName = "+";
            wndc.hbrBackground = GetStockObject(BLACK_BRUSH);
            wndc.lpfnWndProc = DefWindowProc;
            RegisterClassEx(&wndc);
            tmrd.hogl = CreateWindowEx(WS_EX_APPWINDOW, wndc.lpszClassName,
                                       NULL, WS_POPUP, 0, 0, 0, 0,
                                       NULL, NULL, wndc.hInstance, NULL);
            tmrd.mwdc = GetDC(tmrd.hogl);
            ppfd.iLayerType = PFD_MAIN_PLANE;
            SetPixelFormat(tmrd.mwdc,
                           ChoosePixelFormat(tmrd.mwdc, &ppfd), &ppfd);
            wglMakeCurrent(tmrd.mwdc, tmrd.mwrc = wglCreateContext(tmrd.mwdc));

            if ((wglMakeCtx = (typeof(wglMakeCtx))
                 wglGetProcAddress("wglCreateContextAttribsARB"))
            &&  (rtmp = wglMakeCtx(tmrd.mwdc, 0, attr))) {
                wglMakeCurrent(NULL, NULL);
                wglDeleteContext(tmrd.mwrc);
                wglMakeCurrent(tmrd.mwdc, tmrd.mwrc = rtmp);
            }
            if (!InitRendererOGL()) {
                char errm[48];
                sprintf(errm, "Can`t init OpenGL v%d.%d!", attr[1], attr[3]);
                MessageBox(NULL, errm, NULL, MB_OK | MB_ICONEXCLAMATION);
                DeinitGL();
                return 0;
            }
            break;
    }
    LoadUnitStdThrd = LoadUST;
    GetSystemInfo(&syin);
    tmrd.ncpu = min(MAXIMUM_WAIT_OBJECTS, max(1, syin.dwNumberOfProcessors));
    tmrd.isem = LocalAlloc(LMEM_FIXED, tmrd.ncpu * sizeof(*tmrd.isem));
    tmrd.osem = LocalAlloc(LMEM_FIXED, tmrd.ncpu * sizeof(*tmrd.osem));
    tmrd.thrd = LocalAlloc(LMEM_FIXED, tmrd.ncpu * sizeof(*tmrd.thrd));
    for (iter = 0; iter < tmrd.ncpu; iter++) {
        tmrd.isem[iter] = CreateEvent(NULL, FALSE, FALSE, NULL);
        tmrd.osem[iter] = CreateEvent(NULL, FALSE,  TRUE, NULL);
        tmrd.thrd[iter] = (THRD){&tmrd.isem[iter], &tmrd.osem[iter],
                                 TRUE, LTHR};
        CreateThread(NULL, 0, ThrdFunc, &tmrd.thrd[iter], 0, NULL);
    }
    tmrd.devc = CreateCompatibleDC(NULL);
    bmpi.bmiHeader.biWidth = tmrd.pict.xdim;
    bmpi.bmiHeader.biHeight =
        (tmrd.rndr == BRT_RSTD)? -tmrd.pict.ydim : tmrd.pict.ydim;
    tmrd.hdib = CreateDIBSection(tmrd.devc, &bmpi, DIB_RGB_COLORS,
                                (void*)&tmrd.pict.bptr, 0, 0);
    SelectObject(tmrd.devc, tmrd.hdib);
    tmrd.time = GetTickCount();
    return ~0;
}



LIB_OPEN void EngineLoadAnimAsync(uint8_t *path, uint32_t *uuid,
                                  uint32_t *xdim, uint32_t *ydim,
                                  uint32_t *fcnt, uint32_t **time) {
    TryLoadUnit(path, uuid, xdim, ydim, fcnt, time);
}



LIB_OPEN void EngineFinishLoading() {
    long iter;

    WaitForMultipleObjects(tmrd.ncpu, tmrd.osem, TRUE, INFINITE);
    for (iter = 0; iter < tmrd.ncpu; iter++) {
        TryUpdatePixTree(tmrd.thrd[iter].elem);
        tmrd.thrd[iter].loop = FALSE;
        ResetEvent(*tmrd.thrd[iter].osem);
        SetEvent(*tmrd.thrd[iter].isem);
    }
    WaitForMultipleObjects(tmrd.ncpu, tmrd.osem, TRUE, INFINITE);
    MakeUnitArray(&tmrd.uarr);
}



LIB_OPEN void EngineRunMainLoop(UFRM func, uint32_t msec, uint32_t size) {
    BLENDFUNCTION bfun = {AC_SRC_OVER, 0, 255, AC_SRC_ALPHA};
    POINT cpos, zpos = {};
    MSG pmsg = {};
    RECT scrr;
    SIZE dims;

    LONG iter;
    uint64_t mtmp;
    FRBO *fram = NULL;

    if (tmrd.uarr) {
        mtmp = GetTickCount() - tmrd.time;
        UpdateFrame = func;
        tmrd.uniq = tmrd.uarr[0].scal - 1;
        tmrd.data = calloc(((size >> 12) + 2) * 4096, sizeof(*tmrd.data));
        tmrd.size = size;
        printf(TXT_AEND"\n", tmrd.uniq, (ulong)mtmp, (float)mtmp / tmrd.uniq,
              (tmrd.rndr == BRT_ROGL)? TXT_ROGL : TXT_RSTD);
        scrr =
            (RECT){0, 0, dims.cx = tmrd.pict.xdim, dims.cy = tmrd.pict.ydim};

        switch (tmrd.rndr) {
            case BRT_RSTD:
                mtmp = (tmrd.pict.ydim / tmrd.ncpu) + 1;
                for (iter = 0; iter < tmrd.ncpu; iter++) {
                    tmrd.thrd[iter] =
                        (THRD){tmrd.thrd[iter].isem, tmrd.thrd[iter].osem,
                               TRUE, PTHR, {&tmrd}, {mtmp * iter},
                               mtmp * (iter + 1)};
                    CreateThread(NULL, 0, ThrdFunc, &tmrd.thrd[iter], 0, NULL);
                }
                tmrd.thrd[tmrd.ncpu - 1].ymax = tmrd.pict.ydim;
                break;

            case BRT_ROGL:
                MakeRendererOGL(tmrd.uarr, tmrd.uniq, tmrd.data,
                                tmrd.size, !(tmrd.flgs & WIN_IBGR));
                SizeRendererOGL(tmrd.pict.xdim, tmrd.pict.ydim);
                fram = MakeRBO(tmrd.pict.xdim, tmrd.pict.ydim);
                glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fram->fbuf);
                glViewport(0, 0, fram->xdim, fram->ydim);
                glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
                break;
        }
        while (TRUE) {
            tmrd.time = GetTickCount();
            if (PeekMessage(&pmsg, 0, 0, 0, PM_REMOVE)) {
                if (pmsg.message == WM_QUIT)
                    break;
                TranslateMessage(&pmsg);
                DispatchMessage(&pmsg);
                continue;
            }
            GetCursorPos(&cpos);
            ScreenToClient(tmrd.hwnd, &cpos);
            iter = ((GetAsyncKeyState(VK_LBUTTON))? 1 : 0)
                 | ((GetAsyncKeyState(VK_MBUTTON))? 2 : 0)
                 | ((GetAsyncKeyState(VK_RBUTTON))? 4 : 0);
            UpdateFrame(tmrd.data, &tmrd.time, iter, cpos.x, cpos.y,
                        SelectUnit(tmrd.uarr, tmrd.data,
                                   tmrd.size, cpos.x, cpos.y));
            switch (tmrd.rndr) {
                case BRT_RSTD:
                    FillRect(tmrd.devc, &scrr, GetStockObject(BLACK_BRUSH));
                    for (iter = 0; iter < tmrd.ncpu; iter++) {
                        ResetEvent(*tmrd.thrd[iter].osem);
                        SetEvent(*tmrd.thrd[iter].isem);
                    }
                    WaitForMultipleObjects(tmrd.ncpu, tmrd.osem,
                                           TRUE, INFINITE);
                    break;

                case BRT_ROGL:
                    ReadRBO(fram, &tmrd.pict, tmrd.flgs);
                    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fram->fbuf);
                    DrawRendererOGL(tmrd.data, tmrd.size);
                    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
                    break;
            }
            UpdateLayeredWindow(tmrd.hwnd, NULL, &zpos, &dims,
                                tmrd.devc, &zpos, 0, &bfun, ULW_ALPHA);
            tmrd.fram++;
            while (GetTickCount() - tmrd.time < msec)
                Sleep(1);
        }
    }
    else
        printf("No animation base found! Exiting...\n");

    if (tmrd.rndr == BRT_ROGL) {
        FreeRendererOGL();
        DeinitGL();
    }
    FreeUnitArray(&tmrd.uarr);
    DeleteDC(tmrd.devc);
    DeleteObject(tmrd.hdib);
    for (iter = 0; iter < tmrd.ncpu; iter++) {
        CloseHandle(tmrd.isem[iter]);
        CloseHandle(tmrd.osem[iter]);
    }
    LocalFree(tmrd.isem);
    LocalFree(tmrd.osem);
    LocalFree(tmrd.thrd);
    free(tmrd.data);
}
