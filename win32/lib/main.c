#define _WIN32_WINNT 0x0501

#include <windows.h>

#include "../../core/core.h"
#include "../../core/ogl/core.h"



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

HGLRC mwrc;
HDC devc, mwdc;
HWND hwnd, hogl;
HBITMAP hdib;
ulong fram;



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



LPWSTR UTF16(char *utf8) {
    long size = strlen(utf8) * 4 + 2;
    LPWSTR retn = calloc(size, sizeof(*retn));
    MultiByteToWideChar(CP_UTF8, 0, utf8, -1, retn, size);
    return retn;
}



char *LoadFile(void *name) {
    struct {
        char *name;
        long size;
    } *data = name;
    char *retn = 0;
    DWORD  temp;
    LPWSTR wide = UTF16(data->name);
    HANDLE file = CreateFileW(wide, GENERIC_READ, FILE_SHARE_READ, 0,
                              OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    if (file != INVALID_HANDLE_VALUE) {
        data->size = GetFileSize(file, NULL);
        retn = malloc(data->size);
        ReadFile(file, retn, data->size, &temp, NULL);
        CloseHandle(file);
    }
    free(wide);
    return retn;
}



void MakeThread(THRD *thrd) {
    CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)ThrdFunc, thrd, 0, NULL);
}



void InitRenderer(TMRD *tmrd) {
    switch (tmrd->rndr) {
        case BRT_RSTD:
            SwitchThreads(tmrd, 1);
            break;

        case BRT_ROGL: {
            MakeRendererOGL(tmrd->uarr, tmrd->uniq,
                            tmrd->size, !(tmrd->flgs & WIN_IBGR));
            SizeRendererOGL(tmrd->pict.xdim, tmrd->pict.ydim);
            break;
        }
    }
}



long PickSemaphore(TMRD *tmrd, long open, SEM_TYPE mask) {
    SEMD *drop = (open)? &tmrd->osem : &tmrd->isem,
         *pick = (open)? &tmrd->isem : &tmrd->osem;
    long iter;

    for (iter = 0; iter < tmrd->ncpu; iter++)
        if (mask & (1 << iter))
            ResetEvent(drop->list[iter]);

    for (iter = 0; iter < tmrd->ncpu; iter++)
        if (mask & (1 << iter))
            SetEvent(pick->list[iter]);

    return TRUE;
}



SEM_TYPE WaitSemaphore(TMRD *tmrd, long open, SEM_TYPE mask) {
    SEMD *wait = (open)? &tmrd->osem : &tmrd->isem;
    SEM_TYPE retn;
    HANDLE *list;

    if (mask != SEM_NULL) {
        mask &= (1 << tmrd->ncpu) - 1;
        retn = mask;
        open = 0;
        while (retn) {
            retn &= ~(1 << FindBit(retn));
            open++;
        }
        list = LocalAlloc(LMEM_FIXED, open * sizeof(*list));
        list = &list[open];
        retn = mask;
        while (retn) {
            *--list = wait->list[FindBit(retn)];
            retn &= ~(1 << FindBit(retn));
        }
        WaitForMultipleObjects(open, list, TRUE, INFINITE);
        LocalFree(list);
        retn = mask;
    }
    else {
        retn = WaitForMultipleObjects(tmrd->ncpu, wait->list, FALSE, INFINITE);
        retn = ((retn - WAIT_OBJECT_0) < MAXIMUM_WAIT_OBJECTS)? 1 << retn : 0;
    }
    return retn;
}



void FreeSemaphore(SEMD *retn, long nthr) {
    long iter;

    for (iter = 0; iter < nthr; iter++)
        CloseHandle(retn->list[iter]);
    LocalFree(retn->list);
}



void MakeSemaphore(SEMD *retn, long nthr, SEM_TYPE mask) {
    long iter;

    retn->list = LocalAlloc(LMEM_FIXED, nthr * sizeof(*retn->list));
    for (iter = 0; iter < nthr; iter++)
        retn->list[iter] = CreateEvent(NULL, TRUE, (mask >> iter) & 1, NULL);
}



uint64_t TimeFunc() {
    return GetTickCount();
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
            static char fout[64];
            sprintf(fout, TXT_FFPS, fram);
            SetWindowText(hWnd, fout);
            printf("%s\n", fout);
            fram = 0;
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
    wglDeleteContext(mwrc);
    ReleaseDC(hogl, mwdc);
    DeleteDC(mwdc);
    DestroyWindow(hogl);
    DestroyWindow(hwnd);
}



long EngineInitialize(uint32_t rndr,
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

    tmrd.flgs = flgs;
    tmrd.rndr = rndr;
    tmrd.draw = FALSE;
    tmrd.pict.xdim =
        *xdim = (*xdim)? *xdim : GetSystemMetrics(SM_CXVIRTUALSCREEN);
    tmrd.pict.ydim =
        *ydim = (*ydim)? *ydim : GetSystemMetrics(SM_CYVIRTUALSCREEN);

    RegisterClassEx(&wndc);
    hwnd = CreateWindowEx(WS_EX_LAYERED | WS_EX_TOPMOST,
                          wndc.lpszClassName, NULL, WS_POPUP | WS_VISIBLE,
                          0, 0, 100, 100,//tmrd.pict.xdim, tmrd.pict.ydim,
                          NULL, NULL, wndc.hInstance, NULL);
    switch (tmrd.rndr) {
        case BRT_RSTD:
            break;

        case BRT_ROGL:
            wndc.lpszClassName = "+";
            wndc.hbrBackground = GetStockObject(BLACK_BRUSH);
            wndc.lpfnWndProc = DefWindowProc;
            RegisterClassEx(&wndc);
            hogl = CreateWindowEx(WS_EX_APPWINDOW, wndc.lpszClassName,
                                  NULL, WS_POPUP, 0, 0, 0, 0,
                                  NULL, NULL, wndc.hInstance, NULL);
            mwdc = GetDC(hogl);
            ppfd.iLayerType = PFD_MAIN_PLANE;
            SetPixelFormat(mwdc, ChoosePixelFormat(mwdc, &ppfd), &ppfd);
            wglMakeCurrent(mwdc, mwrc = wglCreateContext(mwdc));

            if ((wglMakeCtx = (typeof(wglMakeCtx))
                 wglGetProcAddress("wglCreateContextAttribsARB"))
            &&  (rtmp = wglMakeCtx(mwdc, 0, attr))) {
                wglMakeCurrent(NULL, NULL);
                wglDeleteContext(mwrc);
                wglMakeCurrent(mwdc, mwrc = rtmp);
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

    devc = CreateCompatibleDC(NULL);
    bmpi.bmiHeader.biWidth = tmrd.pict.xdim;
    bmpi.bmiHeader.biHeight =
        (tmrd.rndr == BRT_RSTD)? -tmrd.pict.ydim : tmrd.pict.ydim;
    hdib = CreateDIBSection(devc, &bmpi, DIB_RGB_COLORS,
                           (void*)&tmrd.pict.bptr, 0, 0);
    SelectObject(devc, hdib);

    GetSystemInfo(&syin);
    tmrd.ncpu = min(MAXIMUM_WAIT_OBJECTS, max(1, syin.dwNumberOfProcessors));
    tmrd.thrd = LocalAlloc(LMEM_FIXED, tmrd.ncpu * sizeof(*tmrd.thrd));
    MakeSemaphore(&tmrd.isem, tmrd.ncpu, SEM_NULL);
    MakeSemaphore(&tmrd.osem, tmrd.ncpu, SEM_FULL);
    SwitchThreads(&tmrd, 0);
    tmrd.time = TimeFunc();
    return ~0;
}



void EngineRunMainLoop(UFRM func, uint32_t msec, uint32_t size) {
    BLENDFUNCTION bfun = {AC_SRC_OVER, 0, 255, AC_SRC_ALPHA};
    RECT scrr = {0, 0, tmrd.pict.xdim, tmrd.pict.ydim};
    SIZE dims = {tmrd.pict.xdim, tmrd.pict.ydim};
    POINT cpos, zpos = {};
    MSG pmsg = {};

    LONG iter;
    FRBO *surf = NULL;

    if (tmrd.uarr) {
        UpdateFrame = func;
        tmrd.data = calloc(((size >> 12) + 2) * 4096, sizeof(*tmrd.data));
        tmrd.size = size;

        switch (tmrd.rndr) {
            case BRT_RSTD:
                break;

            case BRT_ROGL:
                surf = MakeRBO(tmrd.pict.xdim, tmrd.pict.ydim);
                glBindFramebuffer(GL_DRAW_FRAMEBUFFER, surf->fbuf);
                glViewport(0, 0, surf->xdim, surf->ydim);
                glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
                break;
        }
        InitRenderer(&tmrd);
        while (TRUE) {
            tmrd.time = TimeFunc();
            if (PeekMessage(&pmsg, 0, 0, 0, PM_REMOVE)) {
                if (pmsg.message == WM_QUIT)
                    break;
                TranslateMessage(&pmsg);
                DispatchMessage(&pmsg);
                continue;
            }
            if (!tmrd.draw)
                continue;
            GetCursorPos(&cpos);
            ScreenToClient(hwnd, &cpos);
            iter = ((GetAsyncKeyState(VK_LBUTTON))? 1 : 0)
                 | ((GetAsyncKeyState(VK_MBUTTON))? 2 : 0)
                 | ((GetAsyncKeyState(VK_RBUTTON))? 4 : 0);
            UpdateFrame(tmrd.data, &tmrd.time, iter, cpos.x, cpos.y,
                        SelectUnit(tmrd.uarr, tmrd.data,
                                   tmrd.size, cpos.x, cpos.y));
            switch (tmrd.rndr) {
                case BRT_RSTD:
                    FillRect(devc, &scrr, GetStockObject(BLACK_BRUSH));
                    PickSemaphore(&tmrd, 1, SEM_FULL);
                    WaitSemaphore(&tmrd, 1, SEM_FULL);
                    break;

                case BRT_ROGL:
                    ReadRBO(surf, &tmrd.pict, tmrd.flgs);
                    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, surf->fbuf);
                    DrawRendererOGL(tmrd.data, tmrd.size);
                    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
                    break;
            }
            UpdateLayeredWindow(hwnd, NULL, &zpos, &dims,
                                devc, &zpos, 0, &bfun, ULW_ALPHA);
            fram++;
            while (TimeFunc() - tmrd.time < msec)
                Sleep(1);
        }
        switch (tmrd.rndr) {
            case BRT_RSTD:
                StopThreads(&tmrd);
                break;

            case BRT_ROGL: {
                FreeRendererOGL();
                FreeRBO(&surf);
                DeinitGL();
                break;
            }
        }
        FreeUnitArray(&tmrd.uarr);
        FreeHashTrees();
        free(tmrd.data);
    }
    else
        printf("No animation base found! Exiting...\n");

    DeleteDC(devc);
    DeleteObject(hdib);

    FreeSemaphore(&tmrd.isem, tmrd.ncpu);
    FreeSemaphore(&tmrd.osem, tmrd.ncpu);
    LocalFree(tmrd.thrd);
}
