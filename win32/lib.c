#define _WIN32_IE 0x0300
#define _WIN32_WINNT 0x0501
#define WINVER _WIN32_WINNT

#include <windows.h>
#include <commctrl.h>

#include <core.h>
#include <ogl/oglstd.h>



#define WM_TRAY (WM_USER + 1000)
#define WM_STOP (WM_USER + 1001)

#define MMI_CONS 1
#define MMI_IRGN 2
#define MMI_IBGR 3
#define MMI_IPBO 4



void RestartEngine(ENGD *engd, ulong rscm) {
    engd->draw = rscm;
    PostMessage((HWND)engd->user[0], WM_STOP, 0, 0);
}



void ShowMainWindow(ENGD *engd, ulong show) {
    ShowWindow((HWND)engd->user[0], (show)? SW_SHOW : SW_HIDE);
}



long OldWin32() {
    return ((GetVersion() & 0xFF) < 5)? 1 : 0;
}



LPWSTR UTF16(char *utf8) {
    long size = strlen(utf8) * 4 + 2;
    LPWSTR retn = calloc(size, sizeof(*retn));
    MultiByteToWideChar(CP_UTF8, 0, utf8, -1, retn, size);
    return retn;
}



char *ASCII(LPWSTR wide) {
    long size = (wcslen(wide) + 1) * 2;
    char *retn = calloc(size, sizeof(*retn));
    WideCharToMultiByte(CP_ACP, WC_NO_BEST_FIT_CHARS, wide, -1,
                        retn, size - 1, "?", NULL);
    return retn;
}



char *UTF8toASCII(char *utf8) {
    LPWSTR temp = UTF16(utf8);
    char *retn = ASCII(temp);
    free(temp);
    return retn;
}



char *ConvertUTF8(char *utf8) {
    if (!utf8)
        return utf8;
    return (!OldWin32())? (char*)UTF16(utf8) : UTF8toASCII(utf8);
}



long Message(HWND hwnd, char *text, char *head, UINT flgs) {
    char *tttt = ConvertUTF8(text),
         *hhhh = ConvertUTF8(head);
    long retn = (OldWin32())? MessageBoxA(hwnd, tttt, hhhh, flgs)
                            : MessageBoxW(hwnd, (LPWSTR)tttt,
                                         (LPWSTR)hhhh, flgs);
    free(tttt);
    free(hhhh);
    return retn;
}



void OSSpecific(MENU *item) {
    switch (item->uuid) {
        case MMI_CONS:
            if (item->flgs & MFL_VCHK) {
                AllocConsole();
                freopen("CONOUT$", "wb", stdout);
            }
            else {
                fclose(stdout);
                FreeConsole();
            }
            break;

        case MMI_IRGN: {
            ENGD *engd = (ENGD*)item->data;

            if (item->flgs & MFL_VCHK)
                engd->flgs |= WIN_IRGN;
            else
                engd->flgs &= ~WIN_IRGN;
            if (engd->flgs & COM_IOPQ)
                Message((HWND)engd->user[0], (char*)engd->tran[TXT_UOFO],
                         NULL, MB_OK | MB_ICONEXCLAMATION);
            else
                RestartEngine(engd, engd->rscm);
            break;
        }
        case MMI_IBGR:
        case MMI_IPBO: {
            ENGD *engd = (ENGD*)item->data;

            if (item->flgs & MFL_VCHK)
                engd->flgs |= ((item->uuid == MMI_IBGR)? WIN_IBGR : WIN_IPBO);
            else
                engd->flgs &= ~((item->uuid == MMI_IBGR)? WIN_IBGR : WIN_IPBO);
            if (engd->rscm != SCM_ROGL)
                Message((HWND)engd->user[0], (char*)engd->tran[TXT_UWGL],
                         NULL, MB_OK | MB_ICONEXCLAMATION);
            else
                RestartEngine(engd, engd->rscm);
            break;
        }
    }
}



MENU *OSSpecificMenu(ENGD *engd) {
    char buff[1024];
    MENU tmpl[] =
        {{.text = engd->tran[TXT_CONS], .uuid = MMI_CONS, .func = OSSpecific,
          .flgs = MFL_CCHK | ((GetConsoleTitle(buff, 1023))? MFL_VCHK : 0)},
         {.text = engd->tran[TXT_IRGN], .uuid = MMI_IRGN, .func = OSSpecific,
          .flgs = MFL_CCHK | ((engd->flgs & WIN_IRGN)? MFL_VCHK : 0) |
                             (((GetVersion() & 0xFF) < 5)? MFL_GRAY : 0),
          .data = (uintptr_t)engd},
         {.text = engd->tran[TXT_IBGR], .uuid = MMI_IBGR, .func = OSSpecific,
          .flgs = MFL_CCHK | ((engd->flgs & WIN_IBGR)? MFL_VCHK : 0),
          .data = (uintptr_t)engd},
         {.text = engd->tran[TXT_IPBO], .uuid = MMI_IPBO, .func = OSSpecific,
          .flgs = MFL_CCHK | ((engd->flgs & WIN_IPBO)? MFL_VCHK : 0),
          .data = (uintptr_t)engd},
         {}};
    return EngineMenuFromTemplate(tmpl);
}



HMENU Submenu(MENU *menu, ulong *chld) {
    if (!menu)
        return NULL;

    union {
        MENUITEMINFOW w;
        MENUITEMINFOA a;
    } pmii;
    ulong indx, oldw;
    HMENU retn;

    oldw = OldWin32();
    indx = (chld)? *chld : 1;
    retn = CreatePopupMenu();
    while (menu->text) {
        pmii.w = (MENUITEMINFOW){sizeof(pmii.w),
                                 MIIM_DATA | MIIM_FTYPE | MIIM_ID | MIIM_STATE,
                                 MFT_STRING, MFS_UNHILITE};
        pmii.w.dwItemData = (ULONG_PTR)menu;
        if (!*menu->text)
            pmii.w.fType |= MFT_SEPARATOR;
        else {
            pmii.w.dwTypeData = (LPWSTR)menu->text;
            pmii.w.cch = (!oldw)? wcslen((LPWSTR)menu->text)
                                : strlen((char*)menu->text);
            pmii.w.fState |= (menu->flgs & MFL_GRAY)? MFS_GRAYED : 0;
            pmii.w.fMask |= MIIM_STRING;
            if (menu->flgs & MFL_CCHK) {
                if (menu->flgs & MFL_RCHK & ~MFL_CCHK)
                    pmii.w.fType |= MFT_RADIOCHECK;
                pmii.w.fState |= (menu->flgs & MFL_VCHK)? MFS_CHECKED : 0;
            }
            if (menu->chld) {
                pmii.w.fMask |= MIIM_SUBMENU;
                pmii.w.hSubMenu = Submenu(menu->chld, &indx);
            }
        }
        pmii.w.wID = indx++;
        if (oldw)
            InsertMenuItemA(retn, pmii.a.wID, FALSE, &pmii.a);
        else
            InsertMenuItemW(retn, pmii.w.wID, FALSE, &pmii.w);
        menu++;
    }
    if (chld)
        *chld = indx;
    return retn;
}



DWORD APIENTRY MenuThread(MENU *menu) {
    MENUITEMINFOA pmii = {sizeof(pmii), MIIM_DATA};
    HWND  iwnd = CreateWindowEx(0, "STATIC", 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
    HMENU mwnd = Submenu(menu, NULL);
    POINT ppos;
    DWORD retn;

    GetCursorPos(&ppos);
    SetForegroundWindow(iwnd);
    if ((retn = TrackPopupMenu(mwnd, TPM_NONOTIFY | TPM_RETURNCMD,
                               ppos.x, ppos.y, 0, iwnd, NULL))) {
        GetMenuItemInfoA(mwnd, retn, FALSE, &pmii);
        ProcessMenuItem((MENU*)pmii.dwItemData);
    }
    DestroyWindow(iwnd);
    DestroyMenu(mwnd);
    return TRUE;
}



void EngineOpenContextMenu(MENU *menu) {
    DWORD retn;

    CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)MenuThread, menu, 0, &retn);
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



char *LoadFile(char *name, long *size) {
    char *retn = 0;
    DWORD temp, flen;
    HANDLE file;

    if (OldWin32())
        file = CreateFileA(name, GENERIC_READ, FILE_SHARE_READ, 0,
                           OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    else {
        LPWSTR wide = UTF16(name);
        file = CreateFileW(wide, GENERIC_READ, FILE_SHARE_READ, 0,
                           OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
        free(wide);
    }
    if (file != INVALID_HANDLE_VALUE) {
        flen = GetFileSize(file, NULL);
        retn = malloc(flen + 1);
        ReadFile(file, retn, flen, &temp, NULL);
        CloseHandle(file);
        retn[flen] = '\0';
        if (size)
            *size = flen;
    }
    return retn;
}



void MakeThread(THRD *thrd) {
    DWORD retn;

    CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)ThrdFunc, thrd, 0, &retn);
}



void InitRenderer(ENGD *engd) {
    switch (engd->rscm) {
        case SCM_RSTD:
            SwitchThreads(engd, 1);
            break;

        case SCM_ROGL: {
            engd->rndr = MakeRendererOGL(engd->uarr, engd->uniq, engd->size,
                                       !(engd->flgs & WIN_IBGR));
            SizeRendererOGL(engd->rndr, engd->pict.xdim, engd->pict.ydim);
            break;
        }
    }
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
        retn->list[iter] = CreateEvent(NULL, TRUE, (mask >> iter) & 1, NULL);
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

        case WM_TRAY: {
            ENGD *engd = (ENGD*)GetWindowLongPtr(hWnd, GWLP_USERDATA);

            if (lPrm == WM_RBUTTONDOWN)
                EngineOpenContextMenu(engd->menu);
            return 0;
        }
        case WM_KEYDOWN:
            if (wPrm == VK_ESCAPE)
        case WM_CLOSE:
                RestartEngine((ENGD*)GetWindowLongPtr(hWnd, GWLP_USERDATA),
                               SCM_QUIT);
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
                    temp = ExtCreateRegion(NULL, rgns.head.dwSize +
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
    INCBIN("../core/icon.gif", MainIcon);

    INITCOMMONCONTROLSEX icct = {sizeof(icct), ICC_STANDARD_CLASSES};
    BLENDFUNCTION bfun = {AC_SRC_OVER, 0, 255, AC_SRC_ALPHA};
    NOTIFYICONDATAW nicd = {sizeof(nicd), 0, 1,
                            NIF_MESSAGE | NIF_ICON | NIF_TIP, WM_TRAY};
    WNDCLASSEX wndc = {sizeof(wndc), CS_HREDRAW | CS_VREDRAW, WindowProc, 0,
                       0, 0, 0, LoadCursor(0, IDC_HAND), 0, 0, " ", 0};
    PIXELFORMATDESCRIPTOR ppfd = {sizeof(ppfd), 1, PFD_SUPPORT_OPENGL |
                                  PFD_DRAW_TO_WINDOW | PFD_DOUBLEBUFFER,
                                  PFD_TYPE_RGBA, 32};
    BITMAPINFO bmpi = {{sizeof(bmpi.bmiHeader), 0, 0,
                        1, 8 * sizeof(BGRA), BI_RGB}};
    SIZE dims = {engd->pict.xdim, engd->pict.ydim};
    RECT scrr = {0, 0, dims.cx, dims.cy};
    POINT cpos, zpos = {};
    MSG pmsg = {};

    BOOL APIENTRY (*ULW)(HWND, HDC, POINT*, SIZE*, HDC, POINT*,
                         COLORREF, BLENDFUNCTION*, DWORD);
    BLENDFUNCTION *bptr;
    HBITMAP hdib, hmsk;
    HINSTANCE hlib;
    HDC devc, mwdc;
    HGLRC mwrc;
    HWND hwnd;
    UINT time;

    LONG x, y, xdim, ydim, xoff, oldw;
    FRBO *surf;
    ASTD *igif;
    BGRA *clrs;
    {
        xdim = GetSystemMetrics(SM_CXSMICON);
        ydim = GetSystemMetrics(SM_CYSMICON);
        BYTE tran[ydim * (xoff = (xdim >> 3) + ((xdim & 7)? 1 : 0))];

        /// the size is wrong, but let it be: MainIcon does have a GIF ending
        igif = MakeDataAnimStd(MainIcon, 1024 * 1024);
        clrs = ExtractRescaleSwizzleAlign(igif, 0xE4, 0, xdim, ydim);
        FreeAnimStd(&igif);

        for (y = 0; y < ydim; y++)
            for (x = 0; x < xdim; x++)
                if (clrs[xdim * y + x].A)
                    tran[xoff * y + (x >> 3)] &= ~(0x80 >> (x & 7));
                else
                    tran[xoff * y + (x >> 3)] |=  (0x80 >> (x & 7));

        hdib = CreateBitmap(xdim, ydim, 1, 32, clrs);
        hmsk = CreateBitmap(xdim, ydim, 1, 1, tran);

        ICONINFO icon = {FALSE, 0, 0, hmsk, hdib};
        wndc.hIcon = CreateIconIndirect(&icon);
        CloseHandle(hdib);
        CloseHandle(hmsk);
        free(clrs);
    }
    mwrc = NULL;
    devc = CreateCompatibleDC(NULL);
    bmpi.bmiHeader.biWidth = dims.cx;
    bmpi.bmiHeader.biHeight = (engd->rscm == SCM_RSTD)? -dims.cy : dims.cy;
    hdib = CreateDIBSection(devc, &bmpi, DIB_RGB_COLORS,
                           (void*)&engd->pict.bptr, 0, 0);
    SelectObject(devc, hdib);

    bptr = &bfun;
    xdim = ULW_ALPHA;
    x = WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TOOLWINDOW;

    InitCommonControlsEx(&icct);
    hlib = LoadLibrary("user32");
    if ((engd->flgs & COM_IOPQ) || (engd->flgs & WIN_IRGN)
    || !(ULW = GetProcAddress(hlib, "UpdateLayeredWindow"))) {
        bptr = (typeof(bptr))engd->pict.bptr;
        zpos.y = bmpi.bmiHeader.biHeight;
        ULW = (typeof(ULW))ULWstub;
        x &= ~WS_EX_LAYERED;
        if (engd->flgs & COM_IOPQ)
            xdim = ULW_OPAQUE;
    }
    RegisterClassEx(&wndc);
    hwnd = CreateWindowEx(x, wndc.lpszClassName, NULL, WS_POPUP | WS_VISIBLE,
                          0, 0, 0, 0, NULL, NULL, wndc.hInstance, engd);
    engd->user[0] = (uintptr_t)hwnd;
    oldw = OldWin32();

    nicd.hWnd = hwnd;
    nicd.hIcon = wndc.hIcon;

    char *temp = ConvertUTF8((char*)engd->tran[TXT_HEAD]);
    if (oldw) {
        strcpy((char*)nicd.szTip, temp);
        Shell_NotifyIconA(NIM_ADD, (NOTIFYICONDATAA*)&nicd);
    }
    else {
        wcscpy(nicd.szTip, (LPWSTR)temp);
        Shell_NotifyIconW(NIM_ADD, &nicd);
    }
    free(temp);

    mwdc = GetDC(hwnd);
    switch (engd->rscm) {
        case SCM_RSTD:
            break;

        case SCM_ROGL:
            ppfd.iLayerType = PFD_MAIN_PLANE;
            SetPixelFormat(mwdc, ChoosePixelFormat(mwdc, &ppfd), &ppfd);
            wglMakeCurrent(mwdc, mwrc = wglCreateContext(mwdc));
            if (!LoadOpenGLFunctions()) {
                Message(NULL, (char*)engd->tran[TXT_NOGL],
                        NULL, MB_OK | MB_ICONEXCLAMATION);
                RestartEngine(engd, SCM_RSTD);
                goto _nogl;
            }
            surf = MakeRBO(dims.cx, dims.cy);
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, surf->fbuf);
            glViewport(0, 0, surf->xdim, surf->ydim);
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
            break;
    }
    InitRenderer(engd);

    time = timeSetEvent(1, 0, TimeFuncWrapper,
                       (DWORD_PTR)&engd->time, TIME_PERIODIC);
    SetWindowPos(hwnd, NULL, 0, 0, dims.cx, dims.cy,
                 SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
    while (TRUE) {
        if (PeekMessage(&pmsg, 0, 0, 0, PM_REMOVE)) {
            if (!IsWindow(hwnd) || (pmsg.message == WM_STOP))
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
        if (!engd->draw)
            continue;
        GetCursorPos(&cpos);
        ScreenToClient(hwnd, &cpos);
        xoff = ((GetAsyncKeyState(VK_LBUTTON))? UFR_LBTN : 0)
             | ((GetAsyncKeyState(VK_MBUTTON))? UFR_MBTN : 0)
             | ((GetAsyncKeyState(VK_RBUTTON))? UFR_RBTN : 0)
             | ((GetActiveWindow() == hwnd)?    UFR_MOUS : 0);
        engd->size = engd->ufrm((uintptr_t)engd, engd->udat, engd->data,
                                &engd->time, xoff, cpos.x, cpos.y,
                                 SelectUnit(engd->uarr, engd->data,
                                            engd->size, cpos.x, cpos.y));
        if (!engd->size) {
            RestartEngine(engd, SCM_QUIT);
            break;
        }
        switch (engd->rscm) {
            case SCM_RSTD:
                FillRect(devc, &scrr, GetStockObject(BLACK_BRUSH));
                PickSemaphore(engd, 1, SEM_FULL);
                WaitSemaphore(engd, 1, SEM_FULL);
                break;

            case SCM_ROGL:
                ReadRBO(surf, &engd->pict, engd->flgs);
                BindRBO(surf, GL_TRUE);
                DrawRendererOGL(engd->rndr, engd->uarr, engd->data,
                                engd->size, engd->flgs & COM_IOPQ);
                BindRBO(surf, GL_FALSE);
                break;
        }
        ULW(hwnd, mwdc, &zpos, &dims, devc, &zpos, 0, bptr, xdim);
        engd->fram++;
    }
    timeKillEvent(time);
    switch (engd->rscm) {
        case SCM_RSTD:
            StopThreads(engd);
            break;

        case SCM_ROGL: {
            FreeRendererOGL(engd->rndr);
            FreeRBO(&surf);
        _nogl:
            wglMakeCurrent(NULL, NULL);
            wglDeleteContext(mwrc);
            break;
        }
    }
    KillTimer(hwnd, 1);
    if (oldw)
        Shell_NotifyIconA(NIM_DELETE, (NOTIFYICONDATAA*)&nicd);
    else
        Shell_NotifyIconW(NIM_DELETE, &nicd);
    DestroyIcon(wndc.hIcon);
    ReleaseDC(hwnd, mwdc);
    DestroyWindow(hwnd);
    /// mandatory: we are a library, not an application
    UnregisterClass(wndc.lpszClassName, wndc.hInstance);
    DeleteDC(mwdc);
    DeleteDC(devc);
    DeleteObject(hdib);
    FreeLibrary(hlib);
}
