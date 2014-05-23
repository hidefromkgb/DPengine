#define _WIN32_WINNT 0x0501
#define _WIN32_IE 0x0300
#define WINVER 0x0410

#include <windows.h>
#include <mmsystem.h>
#include <commctrl.h>

#include "rsrc/rsrc.h"
#include "../core/core.h"
#include "../core/ogl/core.h"



#define FLG_IBGR 0x10000
#define FLG_IPBO 0x20000
#define FLG_CONS 0x40000
#define FLG_EOGL 0x80000

#define BRT_RSTD 0
#define BRT_ROGL 1



typedef struct _TMRD {
    HANDLE quit;
    ulong time;
} TMRD;

typedef struct _THRD {
    union {
        FILL fill;
        DRAW draw;
    } fprm;
    long loop;
    HANDLE evti, *evto;
    void (*func)(void*);
} THRD;



/// renderbuffer-based framebuffer object
typedef struct _FRBO {
    GLuint fbuf,    /// framebuffer
           rbuf[2], /// renderbuffers for pixel and depth data
           pbuf[2]; /// pixel-transfer buffer array
    GLint  xdim,    /// width
           ydim,    /// height
           swiz;    /// pixel buffer switcher
} FRBO;



UNIT *pick = NULL;
VEC2 cptr = {};
UINT fcnt = 0;

ulong rndr = BRT_ROGL;
HGLRC mwrc;
HDC mwdc;



FRBO *MakeRBO(VEC2 size) {
    FRBO *retn = malloc(sizeof(FRBO));
    GLint data;

    retn->xdim = size.x;
    retn->ydim = size.y;
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

    if (flgs & FLG_IPBO)
        glBindBufferARB(GL_PIXEL_PACK_BUFFER_ARB, robj->pbuf[robj->swiz]);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, robj->fbuf);
    glReadPixels(0, 0, robj->xdim, robj->ydim,
                (flgs & FLG_IBGR)? GL_BGRA : GL_RGBA,
                 GL_UNSIGNED_BYTE, (flgs & FLG_IPBO)? NULL : pict->bptr);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);

    if (flgs & FLG_IPBO) {
        robj->swiz ^= 1;
        glBindBufferARB(GL_PIXEL_PACK_BUFFER_ARB, robj->pbuf[robj->swiz]);
        if ((bptr = glMapBufferARB(GL_PIXEL_PACK_BUFFER_ARB,
                                   GL_READ_ONLY_ARB))) {
            memcpy(pict->bptr, bptr,
                   pict->size.x * pict->size.y * sizeof(BGRA));
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



DWORD APIENTRY TimeFunc(LPVOID data) {
    #define data ((TMRD*)data)
    while (!data->quit) {
        data->time = timeGetTime();
        Sleep(MIN_WAIT);
    }
    SetEvent(data->quit);
    return TRUE;
    #undef data
}



DWORD APIENTRY ThrdFunc(LPVOID data) {
    #define data ((THRD*)data)
    do {
        WaitForSingleObject(data->evti, INFINITE);
        ResetEvent(data->evti);
        if (data->loop)
            data->func(&data->fprm);
        else
            printf("Thread exited\n");
        SetEvent(*data->evto);
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
            pick = EMP_PICK;

        case WM_MOUSEMOVE:
            if (pick) {
                cptr.x = (short)(lPrm);
                cptr.y = (short)(lPrm >> 16);
            }
            return 0;


        case WM_LBUTTONUP:
            ReleaseCapture();
            pick = NULL;
            return 0;


        case WM_TIMER: {
            static char fout[24];
            sprintf(fout, "%u FPS", fcnt);
            SetWindowText(hWnd, fout);
            printf("%s\n", fout);
            fcnt = 0;
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



BOOL APIENTRY EnterProc(HWND hWnd, UINT uMsg, WPARAM wPrm, LPARAM lPrm) {
    switch (uMsg) {
        case WM_INITDIALOG: {
            HWND temp = GetDlgItem(hWnd, MSC_INST);
            SendMessage(temp, UDM_SETRANGE, 0, MAKELPARAM(4096, 1));
            SendMessage(temp, UDM_SETPOS, 0, 128);
            SendMessage(GetDlgItem(hWnd, MCB_EOGL),
                        BM_SETCHECK, BST_CHECKED, 0);
            return TRUE;
        }

        case WM_COMMAND:
            switch (LOWORD(wPrm)) {
                case IDOK:
                    uMsg = (SendMessage(GetDlgItem(hWnd, MSC_INST),
                                        UDM_GETPOS,  0, 0) & 0xFFFF)
                         | (SendMessage(GetDlgItem(hWnd, MCB_IBGR),
                                        BM_GETCHECK, 0, 0)? FLG_IBGR : 0)
                         | (SendMessage(GetDlgItem(hWnd, MCB_IPBO),
                                        BM_GETCHECK, 0, 0)? FLG_IPBO : 0)
                         | (SendMessage(GetDlgItem(hWnd, MCB_CONS),
                                        BM_GETCHECK, 0, 0)? FLG_CONS : 0)
                         | (SendMessage(GetDlgItem(hWnd, MCB_EOGL),
                                        BM_GETCHECK, 0, 0)? FLG_EOGL : 0);
                    EndDialog(hWnd, uMsg);
                    break;

                case IDCANCEL:
                    EndDialog(hWnd, 0);
                    break;

                case MCB_EOGL:
                    lPrm = SendMessage(GetDlgItem(hWnd, MCB_EOGL),
                                       BM_GETCHECK, 0, 0);
                    EnableWindow(GetDlgItem(hWnd, MCB_IBGR), lPrm);
                    EnableWindow(GetDlgItem(hWnd, MCB_IPBO), lPrm);
                    break;
            }
            return FALSE;


        case WM_CLOSE:
            EndDialog(hWnd, 0);
            return FALSE;


        default:
            return FALSE;
    }
}



int APIENTRY WinMain(HINSTANCE inst, HINSTANCE prev, LPSTR cmdl, int show) {
    WNDCLASSEX wndc = {sizeof(wndc), CS_HREDRAW | CS_VREDRAW, WindowProc,
                       0, 0, inst, LoadIcon(inst, MAKEINTRESOURCE(ICN_MAIN)),
                       LoadCursor(NULL, IDC_HAND), NULL, NULL, "-", NULL};
    INITCOMMONCONTROLSEX icct = {sizeof(icct), ICC_STANDARD_CLASSES};
    BLENDFUNCTION bfun = {AC_SRC_OVER, 0, 255, AC_SRC_ALPHA};
    PIXELFORMATDESCRIPTOR ppfd = {sizeof(ppfd), 1,
                                  PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL |
                                  PFD_DOUBLEBUFFER, PFD_TYPE_RGBA, 32};
    GLint attr[] = {WGL_CONTEXT_MAJOR_VERSION_ARB, 3,
                    WGL_CONTEXT_MINOR_VERSION_ARB, 0,
                    WGL_CONTEXT_FLAGS_ARB,         0,
                    0};
    HGLRC APIENTRY (*wglMakeCtx)(HDC, HGLRC, GLint*);

    BITMAPINFO bmpi = {{sizeof(bmpi.bmiHeader), 0, 0,
                        1, 8 * sizeof(BGRA), BI_RGB}};
    SYSTEM_INFO syin = {};
    SYSTEMTIME init = {};
    POINT zpos = {};
    MSG pmsg = {};
    HGLRC rtmp;

    TMRD  tmrd = {};
    THRD *thrd;
    ULIB *ulib;
    UNIT *tail, *iter;
    FRBO *fram;

    char *temp;
    WIN32_FIND_DATAW fdir;
    HANDLE hdir, hwnd, hogl, *evto;
    UINT time, indx, nlim, ncpu;
    RECT scrr;

    HBITMAP hdib;
    PICT draw;
    HDC devc;

    T2UV *data = NULL;
    ulong flgs, uniq, size;

    hwnd = hogl = NULL;
    InitCommonControlsEx(&icct);
    flgs = DialogBoxParam(inst, MAKEINTRESOURCE(DLG_MAIN), NULL, EnterProc, 0);
    if (flgs == 0)
        return 0;

    rndr = (flgs & FLG_EOGL)? BRT_ROGL : BRT_RSTD;
    if (flgs & FLG_CONS) {
        AllocConsole();
        freopen("CONOUT$", "wb", stdout);
    }
    GetLocalTime(&init);
    seed = (init.wMilliseconds ^ init.wHour)
         + (init.wSecond ^ init.wMinute) * 0x10000;
    draw.size.x = GetSystemMetrics(SM_CXVIRTUALSCREEN);
    draw.size.y = GetSystemMetrics(SM_CYVIRTUALSCREEN);
    scrr = (RECT){0, 0, draw.size.x, draw.size.y};

    time = timeGetTime();
    GetSystemInfo(&syin);
    ncpu = min(MAXIMUM_WAIT_OBJECTS, max(1, syin.dwNumberOfProcessors));
    evto = LocalAlloc(LMEM_FIXED, ncpu * sizeof(*evto));
    thrd = LocalAlloc(LMEM_FIXED, ncpu * sizeof(*thrd));
    for (indx = 0; indx < ncpu; indx++) {
        evto[indx] = CreateEvent(NULL, FALSE, TRUE, NULL);
        thrd[indx] = (THRD){{(FILL){NULL, 0}}, TRUE,
                            CreateEvent(NULL, FALSE, FALSE, NULL),
                            &evto[indx], (void (*)(void*))FillLibStdThrd};
        CreateThread(NULL, 0, ThrdFunc, &thrd[indx], 0, NULL);
    }
    tail = NULL;
    ulib = NULL;

    hdir = FindFirstFileW(L"anim\\*", &fdir);
    while (hdir != INVALID_HANDLE_VALUE &&
           GetLastError() != ERROR_NO_MORE_FILES) {
        indx = WideCharToMultiByte(CP_UTF8, 0, fdir.cFileName, -1,
                                   NULL, 0, NULL, NULL);
        WideCharToMultiByte(CP_UTF8, 0, fdir.cFileName, -1,
                            temp = malloc(indx * 2), indx, NULL, NULL);
        MakeEmptyLib(&ulib, "anim", temp);
        free(temp);
        indx = WaitForMultipleObjects(ncpu, evto, FALSE, INFINITE);
        if ((indx -= WAIT_OBJECT_0) < MAXIMUM_WAIT_OBJECTS) {
            thrd[indx].fprm.fill.ulib = ulib;
            ResetEvent(*thrd[indx].evto);
            SetEvent(thrd[indx].evti);
        }
        else
            break;
        FindNextFileW(hdir, &fdir);
    }
    FindClose(hdir);

    WaitForMultipleObjects(ncpu, evto, TRUE, INFINITE);
    printf("\n");
    for (indx = 0; indx < ncpu; indx++) {
        thrd[indx].loop = FALSE;
        ResetEvent(*thrd[indx].evto);
        SetEvent(thrd[indx].evti);
    }
    WaitForMultipleObjects(ncpu, evto, TRUE, INFINITE);
    for (nlim = indx = 0; indx < ncpu; indx++) {
        nlim += thrd[indx].fprm.fill.load;
    }
    if (nlim) {
        time = timeGetTime() - time;
        printf("\nLoading complete: %u objects, %u ms [%0.3f ms/obj]\n\n",
               nlim, time, (float)time / (float)nlim);

        UnitListFromLib(ulib, &tail, flgs & 0xFFFF, draw.size, &uniq, &size);

        CreateThread(NULL, 0, TimeFunc, &tmrd, 0, NULL);
        devc = CreateCompatibleDC(NULL);
        bmpi.bmiHeader.biWidth  = draw.size.x;
        bmpi.bmiHeader.biHeight = draw.size.y * ((rndr == BRT_RSTD)? -1 : 1);
        hdib = CreateDIBSection(devc, &bmpi, DIB_RGB_COLORS,
                               (void*)&draw.bptr, 0, 0);
        SelectObject(devc, hdib);

        show = TRUE;
        RegisterClassEx(&wndc);
        hwnd = CreateWindowEx(WS_EX_LAYERED | WS_EX_TOPMOST,
                              wndc.lpszClassName, NULL, WS_POPUP | WS_VISIBLE,
                              0, 0, draw.size.x, draw.size.y,
                              NULL, NULL, wndc.hInstance, wndc.hIcon);
        switch (rndr) {
            case BRT_RSTD:
                for (indx = 0; indx < ncpu; indx++) {
                    thrd[indx].loop = TRUE;
                    thrd[indx].func = (void (*)(void*))DrawPixStdThrd;
                    thrd[indx].fprm.draw =
                        (DRAW){NULL, &draw, ((draw.size.y + 1) / ncpu)* indx,
                              ((draw.size.y + 1) / ncpu)*(indx + 1)};
                    CreateThread(NULL, 0, ThrdFunc, &thrd[indx], 0, NULL);
                }
                thrd[ncpu - 1].fprm.draw.ymax = draw.size.y;
                break;

            case BRT_ROGL:
                wndc.lpfnWndProc = DefWindowProc;
                wndc.lpszClassName = "+";
                wndc.hbrBackground = GetStockObject(BLACK_BRUSH);
                RegisterClassEx(&wndc);
                hogl = CreateWindowEx(WS_EX_APPWINDOW, wndc.lpszClassName,
                                      NULL, WS_POPUP, 0, 0, 0, 0,
                                      NULL, NULL, wndc.hInstance, wndc.hIcon);
                mwdc = GetDC(hogl);
                ppfd.iLayerType = PFD_MAIN_PLANE;
                SetPixelFormat(mwdc, ChoosePixelFormat(mwdc, &ppfd), &ppfd);
                wglMakeCurrent(mwdc, mwrc = wglCreateContext(mwdc));

                if ((wglMakeCtx = (typeof(wglMakeCtx))
                        wglGetProcAddress("wglCreateContextAttribsARB"))) {
                    if ((rtmp = wglMakeCtx(mwdc, 0, attr))) {
                        wglMakeCurrent(NULL, NULL);
                        wglDeleteContext(mwrc);
                        wglMakeCurrent(mwdc, mwrc = rtmp);
                    }
                }
                if (!InitRendererOGL()) {
                    char errm[48];
                    sprintf(errm, "Can`t init OpenGL v%d.%d!",
                            attr[1], attr[3]);
                    MessageBox(hwnd, errm, NULL, MB_OK | MB_ICONEXCLAMATION);
                    show = FALSE;
                }
                else {
                    size *= 4;
                    data = calloc(size, sizeof(*data));
                    MakeRendererOGL(ulib, uniq, data, size,
                                   (flgs & FLG_IBGR)? FALSE : TRUE);
                    SizeRendererOGL(draw.size.x, draw.size.y);
                    fram = MakeRBO(draw.size);
                    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fram->fbuf);
                    glViewport(0, 0, fram->xdim, fram->ydim);
                    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
                }
                break;
        }
        while (show) {
            time = timeGetTime();
            if (PeekMessage(&pmsg, 0, 0, 0, PM_REMOVE)) {
                if (pmsg.message == WM_QUIT)
                    break;
                TranslateMessage(&pmsg);
                DispatchMessage(&pmsg);
            }
            else {
                if ((iter = UpdateFrameStd(&tail, &pick, &tmrd.time, cptr))) {
                    switch (rndr) {
                        /// global vars used: PICK, CPTR, FCNT
                        case BRT_RSTD:
                            FillRect(devc, &scrr, GetStockObject(BLACK_BRUSH));
                            for (indx = 0; indx < ncpu; indx++) {
                                thrd[indx].fprm.draw.tail = iter;
                                ResetEvent(*thrd[indx].evto);
                                SetEvent(thrd[indx].evti);
                            }
                            WaitForMultipleObjects(ncpu, evto, TRUE, INFINITE);
                            break;

                        case BRT_ROGL:
                            indx = 0;
                            while (iter) {
                                data[indx + 0] = data[indx + 1] =
                                data[indx + 2] = data[indx + 3] =
                                    (T2UV){(iter->cpos.y << 16) |
                                           (iter->cpos.x & 0xFFFF),
                                          ((iter->flgs & UCF_REVY)?
                                            0x80000000 : 0) |
                                          ((iter->flgs & UCF_REVX)?
                                            0x40000000 : 0) |
                                          ((iter->fcur & 0x3FF) << 20) |
                                          ((iter->uuid - 1) & 0xFFFFF)};
                                iter = iter->prev;
                                indx += 4;
                            }
                            ReadRBO(fram, &draw, flgs);
                            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fram->fbuf);
                            DrawRendererOGL(data, size);
                            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
                            break;
                    }
                    UpdateLayeredWindow(hwnd, NULL, &zpos, (SIZE*)&draw.size,
                                        devc, &zpos, 0, &bfun, ULW_ALPHA);
                }
                fcnt++;
                while (timeGetTime() - time < FRM_WAIT)
                    Sleep(MIN_WAIT);
            }
        }
        DeleteDC(devc);
        DeleteObject(hdib);
        tmrd.quit = CreateEvent(NULL, FALSE, FALSE, NULL);
        WaitForSingleObject(tmrd.quit, INFINITE);
        CloseHandle(tmrd.quit);

        switch (rndr) {
            case BRT_RSTD:
                for (indx = 0; indx < ncpu; indx++) {
                    thrd[indx].loop = FALSE;
                    SetEvent(thrd[indx].evti);
                }
                WaitForMultipleObjects(ncpu, evto, TRUE, INFINITE);
                break;

            case BRT_ROGL:
                FreeRBO(&fram);
                FreeRendererOGL();
                free(data);
                wglMakeCurrent(NULL, NULL);
                wglDeleteContext(mwrc);
                ReleaseDC(hogl, mwdc);
                DestroyWindow(hogl);
                break;
        }
        FreeLibList(&ulib, (void (*)(void**))FreeAnimStd);
        FreeUnitList(&tail, NULL);
    }
    else
        printf("No animation base found! Exiting...\n");
    for (indx = 0; indx < ncpu; indx++) {
        CloseHandle(*thrd[indx].evto);
        CloseHandle(thrd[indx].evti);
    }
    LocalFree(evto);
    LocalFree(thrd);
    fclose(stdout);
    FreeConsole();
    return 0;
}
