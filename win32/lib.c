#define _WIN32_IE 0x0300
#define _WIN32_WINNT 0x0501
#define WINVER _WIN32_WINNT

#include <windows.h>

#include <core.h>
#include <ogl/oglstd.h>



void RestartEngine(ENGD *engd, ulong anew) {
    engd->anew = anew;
    PostMessage((HWND)engd->user[0], WM_QUIT, 0, 0);
}



void ShowMainWindow(ENGD *engd, ulong show) {
    ShowWindow((HWND)engd->user[0], (show)? SW_SHOW : SW_HIDE);
}



void ReadRBO(FRBO *robj, PICT *pict, ulong flgs) {
    void *bptr;

    if (flgs & WIN_IPBO)
        glBindBufferARB(GL_PIXEL_PACK_BUFFER, robj->pbuf[robj->swiz]);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, robj->fbuf);
    glReadPixels(0, 0, robj->xdim, robj->ydim,
                (flgs & WIN_IBGR)? GL_BGRA : GL_RGBA,
                 GL_UNSIGNED_BYTE, (flgs & WIN_IPBO)? 0 : pict->bptr);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);

    if (flgs & WIN_IPBO) {
        robj->swiz ^= 1;
        glBindBufferARB(GL_PIXEL_PACK_BUFFER, robj->pbuf[robj->swiz]);
        bptr = glMapBufferARB(GL_PIXEL_PACK_BUFFER, GL_READ_ONLY);
        if (bptr) {
            memcpy(pict->bptr, bptr, pict->xdim * pict->ydim * sizeof(BGRA));
            glUnmapBufferARB(GL_PIXEL_PACK_BUFFER);
        }
        glBindBufferARB(GL_PIXEL_PACK_BUFFER, 0);
    }
}



char *LoadFile(char *name, long *size) {
    char *retn = 0;
    DWORD temp, flen;
    HANDLE file;

    if ((GetVersion() & 0xFF) < 5)
        file = CreateFileA(name, GENERIC_READ, FILE_SHARE_READ, 0,
                           OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    else {
        LPWSTR wide = calloc((flen = (strlen(name) + 1) * 4), sizeof(*wide));
        MultiByteToWideChar(CP_UTF8, 0, name, -1, wide, flen);
        file = CreateFileW(wide, GENERIC_READ, FILE_SHARE_READ, 0,
                           OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
        free(wide);
    }
    if (file != INVALID_HANDLE_VALUE) {
        flen = GetFileSize(file, 0);
        retn = malloc(flen + 1);
        ReadFile(file, retn, flen, &temp, 0);
        CloseHandle(file);
        retn[flen] = '\0';
        if (size)
            *size = flen;
    }
    return retn;
}



void MakeThread(THRD *thrd) {
    DWORD retn;

    CreateThread(0, 0, (LPTHREAD_START_ROUTINE)ThrdFunc, thrd, 0, &retn);
}



long PickSemaphore(ENGD *engd, long open, SEM_TYPE mask) {
    SEMD *drop = (open)? &engd->osem : &engd->isem,
         *pick = (open)? &engd->isem : &engd->osem;
    long iter;

    for (iter = 0; iter < engd->ncpu; iter++)
        if (mask & (1 << iter))
            ResetEvent(drop->list[iter]);

    for (iter = 0; iter < engd->ncpu; iter++)
        if (mask & (1 << iter))
            SetEvent(pick->list[iter]);

    return TRUE;
}



SEM_TYPE WaitSemaphore(ENGD *engd, long open, SEM_TYPE mask) {
    SEMD *wait = (open)? &engd->osem : &engd->isem;
    HANDLE *iter, list[engd->ncpu];
    SEM_TYPE retn, temp;

    if (mask != SEM_NULL) {
        open = 0;
        iter = list;
        retn = mask &= (1 << engd->ncpu) - 1;
        while (retn) {
            *iter++ = wait->list[temp = FindBit(retn)];
            retn &= ~(1 << temp);
            open++;
        }
        WaitForMultipleObjects(open, list, TRUE, INFINITE);
        retn = mask;
    }
    else {
        retn = WaitForMultipleObjects(engd->ncpu, wait->list, FALSE, INFINITE);
        retn = (retn < MAXIMUM_WAIT_OBJECTS)? 1 << retn : 0;
    }
    return retn;
}



void FreeSemaphore(SEMD *retn, long nthr) {
    long iter;

    for (iter = 0; iter < nthr; iter++)
        CloseHandle(retn->list[iter]);
    free(retn->list);
}



void MakeSemaphore(SEMD *retn, long nthr, SEM_TYPE mask) {
    long iter;

    retn->list = malloc(nthr * sizeof(*retn->list));
    for (iter = 0; iter < nthr; iter++)
        retn->list[iter] = CreateEvent(0, TRUE, (mask >> iter) & 1, 0);
}



long CountCPUs() {
    SYSTEM_INFO syin = {};

    GetSystemInfo(&syin);
    return min(MAXIMUM_WAIT_OBJECTS, max(1, syin.dwNumberOfProcessors));
}



uint64_t TimeFunc() {
    return GetTickCount();
}



void APIENTRY TimeFuncWrapper(UINT uTmr, UINT uMsg, DWORD_PTR dUsr,
                              DWORD_PTR Res1, DWORD_PTR Res2) {
    *(uint64_t*)dUsr = TimeFunc();
}



LRESULT APIENTRY WindowProc(HWND hWnd, UINT uMsg, WPARAM wPrm, LPARAM lPrm) {
    switch (uMsg) {
        case WM_CREATE:
            SetWindowLongPtr(hWnd, GWLP_USERDATA,
                            (LONG_PTR)((CREATESTRUCT*)lPrm)->lpCreateParams);
            SetTimer(hWnd, 1, 1000, 0);
            return 0;

/** TODO **/
//        case WM_TRAY: {
//            ENGD *engd = (ENGD*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
//
//            if (lPrm == WM_RBUTTONDOWN)
//                EngineOpenContextMenu(engd->menu);
//            return 0;
//        }

        case WM_KEYDOWN:
            if (wPrm == VK_ESCAPE)
        case WM_CLOSE:
                EngineCallback(GetWindowLongPtr(hWnd, GWLP_USERDATA),
                               ECB_QUIT, ~0);
            return 0;


        case WM_LBUTTONDOWN:
            SetCapture(hWnd);
            return 0;


        case WM_LBUTTONUP:
            ReleaseCapture();
            return 0;


        case WM_TIMER: {
            char fout[64];

            OutputFPS((ENGD*)GetWindowLongPtr(hWnd, GWLP_USERDATA), fout);
            SetWindowText(hWnd, fout);
            printf("%s\n", fout);
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



#define MAX_RECT 2000
BOOL APIENTRY ULWstub(HWND hwnd, HDC hdst, POINT *pdst, SIZE *size, HDC hsrc,
                      POINT *psrc, COLORREF ckey, BGRA *bptr, DWORD flgs) {
    struct {
        RGNDATAHEADER head;
        RECT rect[MAX_RECT];
    } rgns;
    long x, y, xpos, ypos;
    HRGN retn, temp;

    if (flgs != ULW_OPAQUE) {
        retn = CreateRectRgn(0, 0, 0, 0);
        rgns.head = (RGNDATAHEADER){sizeof(rgns.head), RDH_RECTANGLES,
                                    0, 0, {0, 0, size->cx, size->cy}};
        for (y = size->cy - 1; y >= 0; y--) {
            ypos = (psrc->y < 0)? y : size->cy - 1 - y;
            for (xpos = 0, x = size->cx - 1; x >= 0; x--) {
                if (bptr[size->cx * y + x].A && !xpos)
                    xpos = x + 1;
                else if ((!bptr[size->cx * y + x].A || !x) && xpos) {
                    rgns.rect[rgns.head.nCount++] =
                        (RECT){(!x && bptr[size->cx * y].A)? 0 : x + 1,
                                 ypos, xpos, ypos + 1};
                    xpos = 0;
                }
                if (!(x | y) || (rgns.head.nCount >= MAX_RECT)) {
                    rgns.head.nRgnSize = rgns.head.nCount * sizeof(RECT);
                    temp = ExtCreateRegion(0, rgns.head.dwSize +
                                              rgns.head.nRgnSize,
                                          (RGNDATA*)&rgns);
                    CombineRgn(temp, temp, retn, RGN_OR);
                    DeleteObject(retn);
                    retn = temp;
                    rgns.head.nCount = 0;
                }
            }
        }
        SetWindowRgn(hwnd, retn, TRUE);
    }
    BitBlt(hdst, 0, 0, size->cx, size->cy, hsrc, 0, 0, SRCCOPY);
    return TRUE;
}
#undef MAX_RECT



void RunMainLoop(ENGD *engd) {
    BLENDFUNCTION bfun = {AC_SRC_OVER, 0, 255, AC_SRC_ALPHA};
    WNDCLASSEX wndc = {sizeof(wndc), CS_HREDRAW | CS_VREDRAW, WindowProc, 0,
                       0, 0, 0, LoadCursor(0, IDC_HAND), 0, 0, " ", 0};
    PIXELFORMATDESCRIPTOR ppfd = {sizeof(ppfd), 1, PFD_SUPPORT_OPENGL |
                                  PFD_DRAW_TO_WINDOW | PFD_DOUBLEBUFFER,
                                  PFD_TYPE_RGBA, 32};
    BITMAPINFO bmpi = {{sizeof(bmpi.bmiHeader), 0, 0,
                        1, 8 * sizeof(BGRA), BI_RGB}};
    SIZE dims = {engd->pict.xdim, engd->pict.ydim};
    RECT scrr = {0, 0, dims.cx, dims.cy};
    POINT cpos, mpos, zpos = {};
    MSG pmsg = {};

    BLENDFUNCTION *bptr;
    BOOL APIENTRY (*ULW)(HWND, HDC, POINT*, SIZE*, HDC, POINT*,
                         COLORREF, BLENDFUNCTION*, DWORD);
    UINT time, attr, flgs;
    HINSTANCE hlib;
    HDC devc, mwdc;
    HBITMAP hdib;
    HGLRC mwrc;
    HWND hwnd;
    FRBO *surf;

    mwrc = 0;
    devc = CreateCompatibleDC(0);
    bmpi.bmiHeader.biWidth = dims.cx;
    bmpi.bmiHeader.biHeight = (engd->rscm == SCM_RSTD)? -dims.cy : dims.cy;
    hdib = CreateDIBSection(devc, &bmpi, DIB_RGB_COLORS,
                           (void*)&engd->pict.bptr, 0, 0);
    SelectObject(devc, hdib);

    bptr = &bfun;
    flgs = ULW_ALPHA;
    attr = WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TOOLWINDOW;
    mpos = (POINT){engd->mpos.x, engd->mpos.y};

    hlib = LoadLibrary("user32");
    if ((engd->flgs & COM_OPAQ) || (engd->flgs & WIN_IRGN)
    || !(ULW = GetProcAddress(hlib, "UpdateLayeredWindow"))) {
        bptr = (typeof(bptr))engd->pict.bptr;
        zpos.y = bmpi.bmiHeader.biHeight;
        ULW = (typeof(ULW))ULWstub;
        attr &= ~WS_EX_LAYERED;
        if (engd->flgs & COM_OPAQ)
            flgs = ULW_OPAQUE;
    }
    RegisterClassEx(&wndc);
    hwnd = CreateWindowEx(attr, wndc.lpszClassName, 0, WS_POPUP | WS_VISIBLE,
                          0, 0, 0, 0, 0, 0, wndc.hInstance, engd);
    engd->user[0] = (uintptr_t)hwnd;

    mwdc = GetDC(hwnd);
    switch (engd->rscm) {
        case SCM_RSTD:
            break;

        case SCM_ROGL: {
            GLchar *retn;

            ppfd.iLayerType = PFD_MAIN_PLANE;
            SetPixelFormat(mwdc, ChoosePixelFormat(mwdc, &ppfd), &ppfd);
            wglMakeCurrent(mwdc, mwrc = wglCreateContext(mwdc));
            if ((retn = LoadOpenGLFunctions(NV_vertex_program3 |
                                            ARB_framebuffer_object))) {
                MessageBox(hwnd, retn, 0, MB_OK | MB_ICONEXCLAMATION);
                free(retn);
                RestartEngine(engd, SCM_RSTD);
                goto _nogl;
            }
            surf = MakeRBO(dims.cx, dims.cy);
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, surf->fbuf);
            glViewport(0, 0, surf->xdim, surf->ydim);
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
            break;
        }
    }
    time = timeSetEvent(1, 0, TimeFuncWrapper,
                       (DWORD_PTR)&engd->time, TIME_PERIODIC);
    SetWindowPos(hwnd, 0, mpos.x, mpos.y, dims.cx, dims.cy,
                 SWP_NOZORDER | SWP_NOACTIVATE);
    while (TRUE) {
        if (PeekMessage(&pmsg, 0, 0, 0, PM_REMOVE)) {
            if (!IsWindow(hwnd) || (pmsg.message == WM_QUIT))
                break;
            TranslateMessage(&pmsg);
            DispatchMessage(&pmsg);
            continue;
        }
        if (engd->time - engd->tfrm < engd->msec) {
            Sleep(1);
            continue;
        }
        engd->tfrm = engd->time;
        if (!(engd->flgs & COM_DRAW))
            continue;
        GetCursorPos(&cpos);
        ScreenToClient(hwnd, &cpos);
        attr = ((GetAsyncKeyState(VK_LBUTTON))? UFR_LBTN : 0)
             | ((GetAsyncKeyState(VK_MBUTTON))? UFR_MBTN : 0)
             | ((GetAsyncKeyState(VK_RBUTTON))? UFR_RBTN : 0)
             | ((GetActiveWindow() == hwnd)?    UFR_MOUS : 0);
        engd->size = engd->ufrm((uintptr_t)engd, engd->udat, &engd->data,
                                &engd->time, attr, cpos.x, cpos.y,
                                 SelectUnit(engd->uarr, engd->data,
                                            engd->size, cpos.x, cpos.y));
        if (!engd->size) {
            EngineCallback((uintptr_t)engd, ECB_QUIT, ~0);
            break;
        }
        switch (engd->rscm) {
            case SCM_RSTD:
                SwitchThreads(engd, 1);
                FillRect(devc, &scrr, GetStockObject(BLACK_BRUSH));
                PickSemaphore(engd, 1, SEM_FULL);
                WaitSemaphore(engd, 1, SEM_FULL);
                break;

            case SCM_ROGL:
                MakeRendererOGL((ROGL**)&engd->rndr, engd->uarr,
                               ~engd->flgs & WIN_IBGR, engd->uniq,
                                engd->size, engd->pict.xdim, engd->pict.ydim);
                ReadRBO(surf, &engd->pict, engd->flgs);
                BindRBO(surf, GL_TRUE);
                DrawRendererOGL(engd->rndr, engd->uarr, engd->data,
                                engd->size, engd->flgs & COM_OPAQ);
                BindRBO(surf, GL_FALSE);
                break;
        }
        ULW(hwnd, mwdc, &mpos, &dims, devc, &zpos, 0, bptr, flgs);
        engd->fram++;
    }
    timeKillEvent(time);
    switch (engd->rscm) {
        case SCM_RSTD:
            StopThreads(engd);
            break;

        case SCM_ROGL: {
            FreeRendererOGL((ROGL**)&engd->rndr);
            FreeRBO(&surf);
        _nogl:
            wglMakeCurrent(0, 0);
            wglDeleteContext(mwrc);
            break;
        }
    }
    KillTimer(hwnd, 1);
    ReleaseDC(hwnd, mwdc);
    DestroyWindow(hwnd);
    /// mandatory: we are a library, not an application
    UnregisterClass(wndc.lpszClassName, wndc.hInstance);
    DeleteDC(mwdc);
    DeleteDC(devc);
    DeleteObject(hdib);
    FreeLibrary(hlib);
}
