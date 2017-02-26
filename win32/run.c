#define _WIN32_IE 0x0500
#define _WIN32_WINNT 0x0501
#define WINVER _WIN32_WINNT

#include <windows.h>
#include <wininet.h>
#include <commdlg.h>
#include <commctrl.h>
#include <shellapi.h>
#include <shlobj.h>
#include <ole2.h>

#include "../exec/exec.h"



#define WM_TRAY (WM_USER + 1000)



typedef struct {
    HANDLE isem;
    UPRE func;
    long size;
} DTHR;

typedef struct {
    HANDLE hdir;
    LPSTR base;
} FIND;

typedef struct {
    HINTERNET hnet, hcon;
} NETC;

/// the necessary evil:
/// SH* functions, loaded from different DLLs on different Windows versions
HRESULT APIENTRY (*SHGF)(HWND, int, HANDLE, DWORD, LPSTR) = 0;
LPITEMIDLIST APIENTRY (*SHBF)(PBROWSEINFOA) = 0;
BOOL APIENTRY (*SHGP)(LPCITEMIDLIST, LPSTR) = 0;
int APIENTRY (*SHFO)(LPSHFILEOPSTRUCTA) = 0;



static inline long OldWin32() {
    static long retn = 2;

    if (retn == 2)
        retn = ((GetVersion() & 0xFF) < 5)? 1 : 0;
    return retn;
}



char *BackReslash(char *conv) {
    long iter;

    if (conv)
        for (iter = 0; conv[iter]; iter++)
            if (conv[iter] == '/')
                conv[iter] = '\\';
    return conv;
}



char *UTF8(LPWSTR wide) {
    long size = 2 * WideCharToMultiByte(CP_UTF8, 0, wide, -1, 0, 0, 0, 0);
    char *retn = calloc(size, sizeof(*retn));
    WideCharToMultiByte(CP_UTF8, 0, wide, -1, retn, size, 0, 0);
    return retn;
}



LPWSTR UTF16(char *utf8) {
    long size = (strlen(utf8) + 1) * 4;
    LPWSTR retn = calloc(size, sizeof(*retn));
    MultiByteToWideChar(CP_UTF8, 0, utf8, -1, retn, size);
    return retn;
}



char *rConvertUTF8(char *utf8) {
    if (!utf8)
        return utf8;

    LPWSTR wide = UTF16(utf8);
    long size;

    if (!OldWin32())
        return (char*)wide;
    else {
        utf8 = calloc((size = (wcslen(wide) + 1) * 4), sizeof(*utf8));
        WideCharToMultiByte(CP_ACP, WC_NO_BEST_FIT_CHARS, wide, -1,
                            utf8, size - 1, "#", 0);
        free(wide);
        return utf8;
    }
}



void AssignTextToControl(CTRL *ctrl, char *text) {
    char *name;

    if (ctrl->priv[0] && text) {
        name = rConvertUTF8(text);
        if (OldWin32())
            SendMessageA((HWND)ctrl->priv[0], WM_SETTEXT, 0, (LPARAM)name);
        else
            SendMessageW((HWND)ctrl->priv[0], WM_SETTEXT, 0, (LPARAM)name);
        free(name);
    }
}



LRESULT APIENTRY MessageBtnRename(int code, WPARAM wPrm, LPARAM lPrm) {
    CWPRETSTRUCT *retn = (CWPRETSTRUCT*)lPrm;
    CTRL ctrl;
    intptr_t addr;
    char **data, clas[32];

    /// well... if THIS isn`t ugly, then I don`t know what is
    if ((code >= 0) && (retn->message == WM_INITDIALOG)) {
        clas[0] = clas[sizeof(clas) - 1] = 0;
        GetClassName(retn->hwnd, clas, sizeof(clas) - 1);
        if (!strcmp(clas, "#32770")) {
            addr = 0;
            clas[0] = clas[sizeof(clas) - 1] = 0;
            GetWindowText(retn->hwnd, clas, sizeof(clas) - 1);
            for (code = 0; clas[code]; code++)
                addr |= (clas[code] - '0') << (code << 2);
            data = (char**)addr;
            ctrl.priv[0] = (intptr_t)retn->hwnd;
            AssignTextToControl(&ctrl, (data[0])? data[0] : "");
            if (!data[2])
                ctrl.priv[0] = (intptr_t)FindWindowEx(retn->hwnd, 0,
                                                      WC_BUTTON, 0);
            else {
                ctrl.priv[0] = (intptr_t)GetDlgItem(retn->hwnd, IDCANCEL);
                AssignTextToControl(&ctrl, data[2]);
                ctrl.priv[0] = (intptr_t)GetDlgItem(retn->hwnd, IDOK);
            }
            AssignTextToControl(&ctrl, data[1]);
        }
    }
    return CallNextHookEx(0, code, wPrm, lPrm);
}

long rMessage(char *text, char *head, char *byes, char *bnay) {
    char hptr[32] = {}, *data[] = {head, byes, bnay};
    union {
        MSGBOXPARAMSA a;
        MSGBOXPARAMSW w;
    } msgp = {{sizeof(msgp), 0, GetModuleHandle(0), rConvertUTF8(text), 0,
             ((bnay)? MB_OKCANCEL : MB_OK) | MB_TASKMODAL | MB_USERICON,
             (LPSTR)1}};
    intptr_t retn, addr;
    HHOOK hook;

    addr = (intptr_t)data;
    for (retn = 0; retn < 16; retn++) {
        hptr[retn] = '0' + (addr & 0xF);
        addr >>= 4;
    }
    msgp.a.lpszCaption = rConvertUTF8(hptr);
    hook = SetWindowsHookEx(WH_CALLWNDPROCRET, MessageBtnRename,
                            0, GetCurrentThreadId());
    retn = (OldWin32())? MessageBoxIndirectA(&msgp.a)
                       : MessageBoxIndirectW(&msgp.w);
    free((LPSTR)msgp.a.lpszCaption);
    free((LPSTR)msgp.a.lpszText);
    UnhookWindowsHookEx(hook);
    return !!(retn == IDOK);
}



#define MMI_CONS 1
#define MMI_IRGN 2
#define MMI_IBGR 3
#define MMI_IPBO 4

void OSSpecific(MENU *item) {
/*
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
            ENGC *engc = (ENGC*)item->data;

            if (item->flgs & MFL_VCHK)
                engc->flgs |= WIN_IRGN;
            else
                engc->flgs &= ~WIN_IRGN;
            if (engc->flgs & COM_OPAQ)
                Message(0, (char*)engc->tran[TXT_UOFO],
                        0, MB_OK | MB_ICONEXCLAMATION);
            else
                EngineCallback(engc->engh, ECB_QUIT, ~0);
            break;
        }
        case MMI_IBGR:
        case MMI_IPBO: {
            ENGC *engc = (ENGC*)item->data;

            if (item->flgs & MFL_VCHK)
                engc->flgs |= ((item->uuid == MMI_IBGR)? WIN_IBGR : WIN_IPBO);
            else
                engc->flgs &= ~((item->uuid == MMI_IBGR)? WIN_IBGR : WIN_IPBO);
/// TODO
//            if (engc->rscm != SCM_ROGL)
//                Message(0, (char*)engc->tran[TXT_UWGL],
//                        0, MB_OK | MB_ICONEXCLAMATION);
//            else
                EngineCallback(engc->engh, ECB_QUIT, ~0);
            break;
        }
    }
//*/
}



MENU *rOSSpecificMenu(void *engc) {
/*
    char buff[1024];
    buff[countof(buff) - 1] = 0;

    MENU tmpl[] =
   {{.text = engc->tran[TXT_CONS], .uuid = MMI_CONS, .func = OSSpecific,
     .flgs = MFL_CCHK | ((GetConsoleTitle(buff, countof(buff) - 1))?
                          MFL_VCHK : 0)},
    {.text = engc->tran[TXT_IRGN], .uuid = MMI_IRGN, .func = OSSpecific,
     .flgs = MFL_CCHK | ((engc->flgs & WIN_IRGN)? MFL_VCHK : 0)
                      | ((OldWin32())? MFL_GRAY : 0),
     .data = (uintptr_t)engc},
    {.text = engc->tran[TXT_IBGR], .uuid = MMI_IBGR, .func = OSSpecific,
     .flgs = MFL_CCHK | ((engc->flgs & WIN_IBGR)? MFL_VCHK : 0),
     .data = (uintptr_t)engc},
    {.text = engc->tran[TXT_IPBO], .uuid = MMI_IPBO, .func = OSSpecific,
     .flgs = MFL_CCHK | ((engc->flgs & WIN_IPBO)? MFL_VCHK : 0),
     .data = (uintptr_t)engc},
    {}};

    return MenuFromTemplate(tmpl);
//*/
    return 0;
}



HMENU Submenu(MENU *menu, long *chld) {
    if (!menu)
        return 0;

    union {
        MENUITEMINFOA a;
        MENUITEMINFOW w;
    } pmii;
    long indx, oldw;
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



DWORD APIENTRY MenuThread(LPVOID menu) {
    MENUITEMINFOA pmii = {sizeof(pmii), MIIM_DATA};
    HWND  iwnd = CreateWindowEx(0, WC_STATIC, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
    HMENU mwnd = Submenu((MENU*)menu, 0);
    POINT ppos;
    DWORD retn;

    GetCursorPos(&ppos);
    SetForegroundWindow(iwnd);
    if ((retn = TrackPopupMenu(mwnd, TPM_NONOTIFY | TPM_RETURNCMD,
                               ppos.x, ppos.y, 0, iwnd, 0))) {
        PostMessage(iwnd, WM_NULL, 0, 0);
        GetMenuItemInfoA(mwnd, retn, FALSE, &pmii);
        eProcessMenuItem((MENU*)pmii.dwItemData);
    }
    DestroyWindow(iwnd);
    DestroyMenu(mwnd);
    return TRUE;
}



void rOpenContextMenu(MENU *menu) {
    DWORD retn;

    CreateThread(0, 0, MenuThread, menu, 0, &retn);
}



DWORD APIENTRY ParallelFunc(LPVOID data) {
    DTHR *dthr = (DTHR*)((intptr_t*)data)[0];
    intptr_t user = ((intptr_t*)data)[1];

    free(data);
    dthr->func(user, 0);
    ReleaseSemaphore(dthr->isem, 1, 0);
    return TRUE;
}



intptr_t rMakeParallel(UPRE func, long size) {
    SYSTEM_INFO syin = {};
    DTHR *dthr;

    GetSystemInfo(&syin);
    size = (syin.dwNumberOfProcessors > 1)?
            syin.dwNumberOfProcessors * size : size;
    *(dthr = calloc(1, sizeof(*dthr))) =
        (DTHR){CreateSemaphore(0, size, size, 0), func, size};
    return (intptr_t)dthr;
}



void rLoadParallel(intptr_t user, intptr_t data) {
    DTHR *dthr = (DTHR*)user;
    DWORD retn;
    intptr_t *pass;

    pass = calloc(2, sizeof(intptr_t));
    pass[0] = user;
    pass[1] = data;
    WaitForSingleObject(dthr->isem, INFINITE);
    CreateThread(0, 0, ParallelFunc, pass, 0, &retn);
}



void rFreeParallel(intptr_t user) {
    DTHR *dthr = (DTHR*)user;

    for (; dthr->size; dthr->size--)
        WaitForSingleObject(dthr->isem, INFINITE);
    CloseHandle(dthr->isem);
    free(dthr);
}



LRESULT APIENTRY DWP(HWND hWnd, UINT uMsg, WPARAM wPrm, LPARAM lPrm) {
    return (OldWin32())? DefWindowProcA(hWnd, uMsg, wPrm, lPrm)
                       : DefWindowProcW(hWnd, uMsg, wPrm, lPrm);
}



LRESULT APIENTRY TrayProc(HWND hWnd, UINT uMsg, WPARAM wPrm, LPARAM lPrm) {
    if (uMsg == WM_TRAY) {
        if (lPrm == WM_RBUTTONDOWN)
            rOpenContextMenu((MENU*)GetWindowLongPtr(hWnd, GWLP_USERDATA));
        return 0;
    }
    return DWP(hWnd, uMsg, wPrm, lPrm);
}



BOOL APIENTRY CalcScreen(HMONITOR hmon, HDC hdcm, LPRECT rect, LPARAM data) {
    LPRECT retn = (LPRECT)data;

    retn->left   = min(retn->left,   rect->left  );
    retn->right  = max(retn->right,  rect->right );
    retn->top    = min(retn->top,    rect->top   );
    retn->bottom = max(retn->bottom, rect->bottom);

    return TRUE;
}



char *rLoadFile(char *name, long *size) {
    char *retn = 0;
    HANDLE file;
    DWORD flen;

    name = rConvertUTF8(name);
    if (OldWin32())
        file = CreateFileA(name, GENERIC_READ, FILE_SHARE_READ, 0,
                           OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    else
        file = CreateFileW((LPWSTR)name, GENERIC_READ, FILE_SHARE_READ, 0,
                           OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    free(name);
    if (file != INVALID_HANDLE_VALUE) {
        flen = GetFileSize(file, 0);
        retn = malloc(flen + 1);
        ReadFile(file, retn, flen, &flen, 0);
        CloseHandle(file);
        retn[flen] = '\0';
        if (size)
            *size = flen;
    }
    return retn;
}



long rSaveFile(char *name, char *data, long size) {
    HANDLE file;
    DWORD flen;

    name = rConvertUTF8(name);
    if (OldWin32())
        file = CreateFileA(name, GENERIC_WRITE, FILE_SHARE_READ, 0,
                           CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
    else
        file = CreateFileW((LPWSTR)name, GENERIC_WRITE, FILE_SHARE_READ, 0,
                           CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
    free(name);
    if (file != INVALID_HANDLE_VALUE) {
        WriteFile(file, data, size, &flen, 0);
        SetEndOfFile(file);
        CloseHandle(file);
        return flen;
    }
    return 0;
}



long rMoveDir(char *dsrc, char *ddst) {
    union {
        SHFILEOPSTRUCTA a;
        SHFILEOPSTRUCTW w;
    } fops = {{.fFlags = FOF_NOCONFIRMATION | FOF_NOERRORUI
                       | FOF_NOCONFIRMMKDIR | FOF_SILENT}};
    char *temp;
    long size;

    if (!ddst)
        fops.a.wFunc = FO_DELETE;
    else {
        fops.a.wFunc = FO_MOVE;
        ddst = rConvertUTF8(temp = BackReslash(strdup(ddst)));
        free(temp);
    }
    dsrc = rConvertUTF8(temp = BackReslash(strdup(dsrc)));
    free(temp);
    if (OldWin32()) {
        dsrc = realloc(dsrc, (size = strlen(dsrc)) + 2);
        dsrc[size + 1] = 0;
        fops.a.pFrom = dsrc;
        if (ddst) {
            ddst = realloc(ddst, (size = strlen(ddst)) + 2);
            ddst[size + 1] = 0;
            fops.a.pTo = ddst;
        }
    }
    else {
        dsrc = realloc(dsrc, (size = wcslen((LPWSTR)dsrc) * 2) + 4);
        dsrc[size + 2] = dsrc[size + 3] = 0;
        fops.w.pFrom = (LPWSTR)dsrc;
        if (ddst) {
            ddst = realloc(ddst, (size = wcslen((LPWSTR)ddst) * 2) + 4);
            ddst[size + 2] = ddst[size + 3] = 0;
            fops.w.pTo = (LPWSTR)ddst;
        }
    }
    SHFO(&fops.a);
    free(ddst);
    free(dsrc);
    return !fops.a.fAnyOperationsAborted;
}



long rMakeDir(char *name) {
    BOOL retn;

    name = rConvertUTF8(name);
    if (OldWin32())
        retn = CreateDirectoryA(name, 0);
    else
        retn = CreateDirectoryW((LPWSTR)name, 0);
    if (!retn && (GetLastError() == ERROR_ALREADY_EXISTS))
        retn = TRUE;
    free(name);
    return !!retn;
}



void rFreeHTTPS(intptr_t user) {
    NETC *netc = (NETC*)user;

    if (netc) {
        if (netc->hcon)
            InternetCloseHandle(netc->hcon);
        if (netc->hnet)
            InternetCloseHandle(netc->hnet);
        free(netc);
    }
}



intptr_t rMakeHTTPS(char *user, char *serv) {
    NETC *netc;

    if (!user || !serv)
        return 0;
    netc = calloc(1, sizeof(*netc));
    netc->hnet = InternetOpen(user, INTERNET_OPEN_TYPE_PRECONFIG, 0, 0, 0);
    netc->hcon = InternetConnect(netc->hnet, serv, INTERNET_DEFAULT_HTTPS_PORT,
                                 0, 0, INTERNET_SERVICE_HTTP, 0, 1);
    if (!netc->hnet || !netc->hcon) {
        rFreeHTTPS((intptr_t)netc);
        netc = 0;
    }
    return (intptr_t)netc;
}



long rLoadHTTPS(intptr_t user, char *page, char **dest) {
    NETC *netc = (NETC*)user;
    char *retn = 0;
    long  size = 0;
    DWORD read = 0;
    HINTERNET hreq;

    if (!user || !page || !dest)
        return 0;
    *dest = 0;
    user = INTERNET_FLAG_KEEP_CONNECTION | INTERNET_FLAG_SECURE;
    if ((hreq = HttpOpenRequest(netc->hcon, "GET", page, 0, 0, 0, user, 1))) {
        HttpSendRequest(hreq, 0, 0, 0, 0);
        do {
            retn = realloc(retn, (size += read) + 0x1000 + 1);
        } while (InternetReadFile(hreq, retn + size, 0x1000, &read) && read);
        InternetCloseHandle(hreq);
        retn = realloc(retn, size + 1);
        if (size) {
            retn[size] = 0;
            *dest = retn;
        }
    }
    return size;
}



intptr_t rMakeTrayIcon(MENU *mctx, char *text,
                       uint32_t *data, long xdim, long ydim) {
    NOTIFYICONDATAW *nicd = calloc(1, sizeof(*nicd));
    char *hint = rConvertUTF8(text);

    LONG x, y, xoff = (xdim >> 3) + ((xdim & 7)? 1 : 0);
    ICONINFO icon = {};
    LPBYTE tran;

    *nicd = (NOTIFYICONDATAW){sizeof(*nicd), 0, 1,
                              NIF_MESSAGE | NIF_ICON | NIF_TIP, WM_TRAY};
    tran = malloc(ydim * xoff);
    for (y = 0; y < ydim; y++)
        for (x = 0; x < xdim; x++)
            if (data[xdim * y + x] & 0xFF000000)
                tran[xoff * y + (x >> 3)] &= ~(0x80 >> (x & 7));
            else
                tran[xoff * y + (x >> 3)] |=  (0x80 >> (x & 7));

    icon.hbmMask  = CreateBitmap(xdim, ydim, 1,  1, tran);
    icon.hbmColor = CreateBitmap(xdim, ydim, 1, 32, data);
    nicd->hIcon = CreateIconIndirect(&icon);
    /// [TODO:] this is not quite right, replace with options window
    nicd->hWnd  = CreateWindowEx(0, WC_STATIC, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
    SetWindowLongPtr(nicd->hWnd, GWLP_USERDATA, (LONG_PTR)mctx);
    SetWindowLongPtr(nicd->hWnd, GWLP_WNDPROC, (LONG_PTR)TrayProc);
    DeleteObject(icon.hbmColor);
    DeleteObject(icon.hbmMask);
    free(tran);

    if (OldWin32()) {
        strcpy(((NOTIFYICONDATAA*)nicd)->szTip, hint);
        Shell_NotifyIconA(NIM_ADD, (NOTIFYICONDATAA*)nicd);
    }
    else {
        wcscpy(nicd->szTip, (LPWSTR)hint);
        Shell_NotifyIconW(NIM_ADD, nicd);
    }
    free(hint);
    return (intptr_t)nicd;
}



void rFreeTrayIcon(intptr_t icon) {
    NOTIFYICONDATAW *nicd = (typeof(nicd))icon;

    DestroyWindow(nicd->hWnd);
    DestroyIcon(nicd->hIcon);
    if (OldWin32())
        Shell_NotifyIconA(NIM_DELETE, (NOTIFYICONDATAA*)nicd);
    else
        Shell_NotifyIconW(NIM_DELETE, nicd);
    free(nicd);
}



void MoveControl(CTRL *ctrl, intptr_t data) {
    long xspc, yspc, xpos = (int16_t)data, ypos = (int32_t)data >> 16;
    CTRL *root = ctrl;

    while (root->prev)
        root = root->prev;
    xspc = (uint16_t)(root->priv[2]      );
    yspc = (uint16_t)(root->priv[2] >> 16);
    xpos = (xpos < 0)? -xpos : xpos * xspc;
    ypos = (ypos < 0)? -ypos : ypos * yspc;
    xspc = (ctrl->xdim < 0)? -ctrl->xdim : ctrl->xdim * xspc;
    yspc = (ctrl->ydim < 0)? -ctrl->ydim : ctrl->ydim * yspc;
    SetWindowPos((HWND)ctrl->priv[0], HWND_TOP,
                  xpos, ypos, xspc, yspc, SWP_SHOWWINDOW);
}



void ResizeSpinControl(CTRL *ctrl) {
    HWND edit, spin;
    RECT dims;

    edit = (HWND)ctrl->priv[0];
    spin = (HWND)ctrl->priv[1];

    ShowWindow(spin, SW_SHOW);
    SendMessage(spin, UDM_SETBUDDY, (WPARAM)edit, 0);
    /// this is slow! somehow speed it up (e.g. restore from the saved value)
    SendMessage(spin, UDM_SETPOS32, 0, SendMessage(spin, UDM_GETPOS32, 0, 0));
    /// also slow! same as above
    GetClientRect(edit, &dims);
    dims.top = (dims.bottom - ctrl->priv[4]) >> 1;
    SendMessage(edit, EM_SETRECT, 0, (LPARAM)&dims);
}



void ProcessSpin(LPARAM lPrm) {
    NMUPDOWN *nmud = (NMUPDOWN*)lPrm;
    CTRL *ctrl = (CTRL*)GetWindowLongPtr(nmud->hdr.hwndFrom, GWLP_USERDATA);

    lPrm = nmud->iPos + nmud->iDelta;
    lPrm = (lPrm >= ctrl->priv[2])? lPrm : ctrl->priv[2];
    lPrm = (lPrm <= ctrl->priv[3])? lPrm : ctrl->priv[3];
    ctrl->fc2e(ctrl, MSG_NSET, lPrm);
}



void TextInRect(HDC devc, char *text, RECT *rect) {
    if (text) {
        if (OldWin32())
            DrawTextA(devc, text, -1, rect,
                      DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        else
            DrawTextW(devc, (LPWSTR)text, -1, rect,
                      DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    }
}



/// PRIV:
///  0: HWND
///  1: HDC
///  2: HBITMAP
///  3: data array
///  4: (xdim) | (ydim << 16)
///  5:
///  6:
///  7: (animation ID << 10) | (current frame)
LRESULT APIENTRY IBoxProc(HWND hWnd, UINT uMsg, WPARAM wPrm, LPARAM lPrm) {
    switch (uMsg) {
        case WM_ERASEBKGND:
            return ~0;

        case WM_PAINT: {
            CTRL *ctrl = (CTRL*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
            if (!ctrl || !ctrl->priv[7])
                break;

            AINF anim = {(ctrl->priv[7] >> 10) & 0x3FFFFF,
                         (int16_t)ctrl->priv[4], (int32_t)ctrl->priv[4] >> 16,
                          ctrl->priv[7] & 0x3FF, (uint32_t*)ctrl->priv[3]};
            PAINTSTRUCT pstr;
            HBRUSH btnf;
            RECT rect;
            HDC devc;

            btnf = GetSysColorBrush(COLOR_BTNFACE);
            devc = BeginPaint(hWnd, &pstr);
            rect = (RECT){0, 0, anim.xdim, anim.ydim};
            FillRect((HDC)ctrl->priv[1], &rect, btnf);
            ctrl->fc2e(ctrl, MSG_IFRM, (intptr_t)&anim);
            BitBlt(devc, 0, 0, anim.xdim, anim.ydim,
                  (HDC)ctrl->priv[1], 0, 0, SRCCOPY);
            EndPaint(hWnd, &pstr);
            return 0;
        }
    }
    return DWP(hWnd, uMsg, wPrm, lPrm);
}



/// PRIV:
///  0: HWND
///  1: HWND-parent
///  2:
///  3:
///  4:
///  5:
///  6:
///  7:
LRESULT APIENTRY ListProc(HWND hWnd, UINT uMsg, WPARAM wPrm, LPARAM lPrm) {
    switch (uMsg) {
        case WM_CREATE:
            SetWindowLongPtr(hWnd, GWLP_USERDATA,
                            (LONG_PTR)((CREATESTRUCT*)lPrm)->lpCreateParams);
            return 0;

        case WM_NOTIFY:
            switch (((LPNMHDR)lPrm)->code) {
                case LVN_GETDISPINFO: {
                    CTRL *ctrl = (CTRL*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
                    NMLVDISPINFO *lvdi = (NMLVDISPINFO*)lPrm;

                    lvdi->item.state =
                        ((ctrl->fc2e(ctrl, MSG_LGST,
                                     lvdi->item.iItem))? 2 : 1) << 12;
                    lvdi->item.stateMask = LVIS_STATEIMAGEMASK;
                    break;
                }
                case LVN_ITEMCHANGING: {
                    CTRL *ctrl = (CTRL*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
                    NMLISTVIEW *nmlv = (NMLISTVIEW*)lPrm;
                    UINT flgs;

                    if ((flgs = (nmlv->uNewState & LVIS_STATEIMAGEMASK) >> 12))
                        ctrl->fc2e(ctrl, MSG_LSST,
                                  (nmlv->iItem << 1) | ((flgs == 2)? 1 : 0));
                    InvalidateRect(hWnd, 0, FALSE);
                    return FALSE;
                }
                case HDN_BEGINTRACKA:
                case HDN_BEGINTRACKW:
                    return TRUE;
            }
            break;
    }
    return DWP(hWnd, uMsg, wPrm, lPrm);
}



/// PRIV:
///  0: HWND
///  1: scroll position
///  2: total height of the preview list
///  3: total width of the preview list
///  4: height of the visible area
///  5:
///  6:
///  7: HWND-scrolled
LRESULT APIENTRY SBoxProc(HWND hWnd, UINT uMsg, WPARAM wPrm, LPARAM lPrm) {
    #define SCR_PLUS 10
    switch (uMsg) {
        case WM_CREATE:
            SetWindowLongPtr(hWnd, GWLP_USERDATA,
                            (LONG_PTR)((CREATESTRUCT*)lPrm)->lpCreateParams);
            return 0;

        case WM_MOUSEACTIVATE:
            if (!GetWindowLongPtr(hWnd, GWLP_USERDATA))
                SetFocus(GetParent(hWnd));
            return 0;

        case WM_NOTIFY:
            if (((NMHDR*)lPrm)->code == UDN_DELTAPOS)
                ProcessSpin(lPrm);
            break;

        case WM_SIZE:
        case WM_VSCROLL:
        case WM_MOUSEWHEEL: {
            CTRL *ctrl = (CTRL*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
            SCROLLINFO sinf = {sizeof(SCROLLINFO),
                               SIF_PAGE | SIF_POS | SIF_RANGE | SIF_TRACKPOS};
            if (!ctrl || !ctrl->fc2e)
                break;
            GetScrollInfo(hWnd, SB_VERT, &sinf);
            sinf.fMask = SIF_PAGE | SIF_POS | SIF_RANGE | SIF_TRACKPOS;
            sinf.nMin = 0;
            if (uMsg == WM_SIZE) {
                double temp;

                temp = (ctrl->priv[2])?
                       (double)ctrl->priv[1] / (double)ctrl->priv[2] : 0.0;
                ctrl->priv[4] = (uint16_t)(lPrm >> 16);
                ctrl->priv[3] = (uint16_t)(lPrm      );
                LockWindowUpdate((HWND)ctrl->priv[7]);
                ctrl->priv[2] = ctrl->fc2e(ctrl, MSG_SMAX, lPrm);
                LockWindowUpdate(0);
                ctrl->priv[1] = temp * ctrl->priv[2];
            }
            else if (uMsg == WM_MOUSEWHEEL)
                ctrl->priv[1] -= (int16_t)HIWORD(wPrm)
                               * ((wPrm & MK_SHIFT)? 5 : 1);
            else {
                switch (LOWORD(wPrm)) {
                    case SB_LINEUP:
                        ctrl->priv[1] -= SCR_PLUS;
                        break;

                    case SB_LINEDOWN:
                        ctrl->priv[1] += SCR_PLUS;
                        break;

                    case SB_PAGEUP:
                        ctrl->priv[1] -= ctrl->priv[4];
                        break;

                    case SB_PAGEDOWN:
                        ctrl->priv[1] += ctrl->priv[4];
                        break;

                    case SB_THUMBTRACK:
                        ctrl->priv[1] = sinf.nTrackPos;
                        break;

                    case SB_TOP:
                        ctrl->priv[1] = 0;
                        break;

                    case SB_BOTTOM:
                        ctrl->priv[1] = ctrl->priv[2];
                        break;

                    case SB_ENDSCROLL:
                    case SB_THUMBPOSITION:
                    default:
                        return 0;
                }
            }
            ctrl->priv[1] = max(0, min(ctrl->priv[2], ctrl->priv[1]));
            sinf.nMax = ctrl->priv[2] + ctrl->priv[4];
            sinf.nPage = ctrl->priv[4];
            sinf.nPos = ctrl->priv[1];
            SetWindowPos((HWND)ctrl->priv[7], HWND_TOP, 0, -sinf.nPos,
                          ctrl->priv[3], sinf.nMax, SWP_SHOWWINDOW);
            SetScrollInfo(hWnd, SB_VERT, &sinf, TRUE);
            return 0;
        }
    }
    return DWP(hWnd, uMsg, wPrm, lPrm);
    #undef SCR_PLUS
}



/// PRIV:
///  0: HWND
///  1: FONT
///  2: (fontmul.x) | (fontmul.y << 16)
///  3: (wndsize.x) | (wndsize.y << 16)
///  4: spin control text field alignment coeff
///  5:
///  6:
///  7:
LRESULT APIENTRY OptProc(HWND hWnd, UINT uMsg, WPARAM wPrm, LPARAM lPrm) {
    CTRL *ctrl;

    switch (uMsg) {
        case WM_COMMAND:
            if (lPrm) {
                ctrl = (CTRL*)GetWindowLongPtr((HWND)lPrm, GWLP_USERDATA);
                if (ctrl->fc2e) /// not everything that comes here is buttons
                    ctrl->fc2e(ctrl, MSG_BCLK, BST_CHECKED ==
                               SendMessage((HWND)lPrm, BM_GETCHECK, 0, 0));
            }
            return 0;

        case WM_GETMINMAXINFO:
            if ((ctrl = (CTRL*)GetWindowLongPtr(hWnd, GWLP_USERDATA))) {
                ((MINMAXINFO*)lPrm)->ptMinTrackSize.x =
                    (uint16_t)(ctrl->priv[3]);
                ((MINMAXINFO*)lPrm)->ptMinTrackSize.y =
                    (uint16_t)(ctrl->priv[3] >> 16);
            }
            return 0;

        case WM_CLOSE:
            ctrl = (CTRL*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
            if (ctrl->fc2e(ctrl, MSG_WEND, 0))
                PostQuitMessage(0);
            return 0;

        case WM_SIZE:
            ctrl = (CTRL*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
            ctrl->fc2e(ctrl, MSG_WSZC, lPrm);
            return 0;

        case WM_NOTIFY:
            if (((NMHDR*)lPrm)->code == UDN_DELTAPOS)
                ProcessSpin(lPrm);
            break;

        case WM_DESTROY:
            PostQuitMessage(0);
            break;
    }
    return DWP(hWnd, uMsg, wPrm, lPrm);
}



/// PRIV:
///  0: HWND
///  1: HWND-spin
///  2: minimum
///  3: maximum
///  4: text field alignment coeff
///  5:
///  6:
///  7: original procedure
LRESULT APIENTRY SpinProc(HWND hWnd, UINT uMsg, WPARAM wPrm, LPARAM lPrm) {
    static char keys[] =
        {VK_BACK, VK_END, VK_HOME, VK_LEFT, VK_UP, VK_RIGHT, VK_DOWN,
         VK_DELETE, '0', '1', '2', '3', '4', '5', '6', '7', '8', '9'};

    CTRL *ctrl = (CTRL*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
    DWORD bcur, ecur;
    char text[128];

    switch (uMsg) {
        case WM_KILLFOCUS:
            wPrm = VK_RETURN;
            /// falling through
        case WM_GETDLGCODE:
            if (wPrm == VK_RETURN) {
                SendMessage(hWnd, WM_GETTEXT, 128, (LPARAM)text);
                sprintf(text, "%d", wPrm = strtol(text, 0, 10));
                SendMessage(hWnd, WM_SETTEXT, 128, (LPARAM)text);
                ctrl->fe2c(ctrl, MSG_NSET, wPrm);
                ctrl->fc2e(ctrl, MSG_NSET, wPrm);
                if (uMsg == WM_KILLFOCUS)
                    break;
                return 0;
            }
            lPrm = DLGC_WANTARROWS | DLGC_WANTCHARS;
            for (uMsg = 0; uMsg < sizeof(keys) / sizeof(*keys); uMsg++)
                if (wPrm == keys[uMsg])
                    return lPrm;
            if ((ctrl->priv[2] < 0) && (wPrm == '-')) {
                SendMessage(hWnd, WM_GETTEXT, 128, (LPARAM)text);
                SendMessage(hWnd, EM_GETSEL, (WPARAM)&bcur, (LPARAM)&ecur);
                if (!bcur && (ecur || !strchr(text, wPrm)))
                    return lPrm;
            }
            return 0;
    }
    return CallWindowProc((WNDPROC)ctrl->priv[7], hWnd, uMsg, wPrm, lPrm);
}



/// PRIV:
///  0: HWND
///  1: FONT
///  2: text to draw
///  3: progress position
///  4: progress maximum
///  5:
///  6:
///  7: original procedure
LRESULT APIENTRY PBarProc(HWND hWnd, UINT uMsg, WPARAM wPrm, LPARAM lPrm) {
    CTRL *ctrl = (CTRL*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
    LRESULT retn, lcur;
    HANDLE devc, clip;
    PAINTSTRUCT pstr;
    RECT rect;

    switch (uMsg) {
        case WM_PAINT:
            GetClientRect(hWnd, &rect);
            lcur = (OldWin32())? rect.right * ctrl->priv[3]
                               / ((ctrl->priv[4] > 0)? ctrl->priv[4] : 1) : 0;
            devc = BeginPaint(hWnd, &pstr);
            retn = CallWindowProc((WNDPROC)ctrl->priv[7],
                                   hWnd, uMsg, (WPARAM)devc, lPrm);
            SetBkMode(devc, TRANSPARENT);
            SelectObject(devc, (HFONT)ctrl->priv[1]);
            for (uMsg = (OldWin32())? 0 : 1; uMsg <= 1; uMsg++) {
                clip = CreateRectRgn((uMsg)? lcur : 0, 0,
                                    (!uMsg)? lcur : rect.right, rect.bottom);
                SetTextColor(devc,  (!uMsg)? GetSysColor(COLOR_HIGHLIGHTTEXT)
                                           : GetSysColor(COLOR_MENUTEXT));
                SelectClipRgn(devc, clip);
                DeleteObject(clip);
                TextInRect(devc, (char*)ctrl->priv[2], &rect);
            }
            SelectClipRgn(devc, 0);
            EndPaint(hWnd, &pstr);
            return retn;
    }
    return CallWindowProc((WNDPROC)ctrl->priv[7], hWnd, uMsg, wPrm, lPrm);
}



intptr_t FE2CW(CTRL *ctrl, uint32_t cmsg, intptr_t data) {
    switch (cmsg) {
        case MSG__SHW:
            ShowWindow((HWND)ctrl->priv[0], (data)? SW_SHOW : SW_HIDE);
            if (data)
                SetForegroundWindow((HWND)ctrl->priv[0]);
            break;

        case MSG__TXT:
            AssignTextToControl(ctrl, (char*)data);
            break;

        case MSG_WSZC: {
            LONG xfrm, yfrm;
            RECT rect;

            SystemParametersInfo(SPI_GETWORKAREA, 0, &rect, 0);

            rect.left += rect.right >> 1;
            rect.top += rect.bottom >> 1;
            xfrm = (ctrl->flgs & FSW_SIZE)? SM_CXFRAME : SM_CXFIXEDFRAME;
            yfrm = (ctrl->flgs & FSW_SIZE)? SM_CYFRAME : SM_CYFIXEDFRAME;
            rect.right  = (((uint16_t)(data)) + ctrl->xdim)
                        *   (uint16_t)(ctrl->priv[2])
                        +   (GetSystemMetrics(xfrm) << 1);
            rect.bottom = (((uint16_t)(data >> 16)) + ctrl->ydim)
                        *   (uint16_t)(ctrl->priv[2] >> 16)
                        +   (GetSystemMetrics(yfrm) << 1)
                        +    GetSystemMetrics(SM_CYCAPTION);

            rect.left -= rect.right >> 1;
            rect.top -= rect.bottom >> 1;

            ctrl->priv[3] =  (uint16_t)rect.right
                          | ((uint32_t)rect.bottom << 16);
            SetWindowPos((HWND)ctrl->priv[0], HWND_TOP,
                          rect.left, rect.top, rect.right, rect.bottom,
                          IsWindowVisible((HWND)ctrl->priv[0])?
                          SWP_SHOWWINDOW : SWP_HIDEWINDOW);
            break;
        }
    }
    return 0;
}



intptr_t FE2CP(CTRL *ctrl, uint32_t cmsg, intptr_t data) {
    MSG pmsg;

    switch (cmsg) {
        case MSG_PLIM:
            ctrl->priv[4] = data;
            SendMessage((HWND)ctrl->priv[0], PBM_SETRANGE32, 0, ctrl->priv[4]);
            InvalidateRect((HWND)ctrl->priv[0], 0, FALSE);
            break;

        case MSG_PGET:
            return (data)? ctrl->priv[4] : ctrl->priv[3];

        case MSG_PPOS:
            ctrl->priv[3] = data;
            /// necessary to avoid the annoying time lag on Vista+
            if (ctrl->priv[3] < ctrl->priv[4])
                SendMessage((HWND)ctrl->priv[0], PBM_SETPOS,
                             ctrl->priv[3] + 1, 0);
            SendMessage((HWND)ctrl->priv[0], PBM_SETPOS, ctrl->priv[3], 0);
            while (PeekMessage(&pmsg, 0, 0, 0, PM_REMOVE)) {
                TranslateMessage(&pmsg);
                DispatchMessage(&pmsg);
            }
            break;

        case MSG__TXT:
            free((void*)ctrl->priv[2]);
            ctrl->priv[2] = (intptr_t)rConvertUTF8((char*)data);
            break;
    }
    return 0;
}



intptr_t FE2CX(CTRL *ctrl, uint32_t cmsg, intptr_t data) {
    switch (cmsg) {
        case MSG__ENB:
            EnableWindow((HWND)ctrl->priv[0], !!data);
            break;

        case MSG__TXT:
            AssignTextToControl(ctrl, (char*)data);
            break;

        case MSG_BGST:
            data  = (IsWindowEnabled((HWND)ctrl->priv[0]))? FCS_ENBL : 0;
            if ((ctrl->flgs & FCT_TTTT) == FCT_CBOX)
                data |= (SendMessage((HWND)ctrl->priv[0],
                                      BM_GETCHECK, 0, 0) == BST_CHECKED)?
                         FCS_MARK : 0;
            return data;

        case MSG_BCLK:
            cmsg = ~BST_CHECKED;
            if ((ctrl->flgs & FCT_TTTT) == FCT_CBOX) {
                cmsg = SendMessage((HWND)ctrl->priv[0], BM_GETCHECK, 0, 0);
                SendMessage((HWND)ctrl->priv[0], BM_SETCHECK,
                            (data)? BST_CHECKED : BST_UNCHECKED, 0);
            }
            return cmsg == BST_CHECKED;
    }
    return 0;
}



intptr_t FE2CL(CTRL *ctrl, uint32_t cmsg, intptr_t data) {
    switch (cmsg) {
        case MSG__ENB:
            EnableWindow((HWND)ctrl->priv[0], !!data);
            break;

        case MSG__TXT: {
            LVCOLUMN vcol = {LVCF_TEXT, 0, 0,
                            .pszText = rConvertUTF8((char*)data)};

            if (OldWin32())
                SendMessageA((HWND)ctrl->priv[0],
                              LVM_SETCOLUMNA, 0, (LPARAM)&vcol);
            else
                SendMessageW((HWND)ctrl->priv[0],
                              LVM_SETCOLUMNW, 0, (LPARAM)&vcol);
            SendMessageA((HWND)ctrl->priv[0], LVM_REDRAWITEMS, 0, MAXWORD);
            InvalidateRect((HWND)ctrl->priv[0], 0, FALSE);
            free(vcol.pszText);
            break;
        }
        case MSG_LADD: {
            LVITEM item = {LVIF_TEXT, MAXWORD,
                          .pszText = rConvertUTF8((char*)data)};

            if (OldWin32())
                SendMessageA((HWND)ctrl->priv[0],
                              LVM_INSERTITEMA, 0, (LPARAM)&item);
            else
                SendMessageW((HWND)ctrl->priv[0],
                              LVM_INSERTITEMW, 0, (LPARAM)&item);
            free(item.pszText);
            break;
        }
    }
    return 0;
}



/// note that MSG_NSET shall multiply the lower value by -1
intptr_t FE2CN(CTRL *ctrl, uint32_t cmsg, intptr_t data) {
    switch (cmsg) {
        case MSG__GSZ: {
            RECT dims;

            GetWindowRect((HWND)ctrl->priv[0], &dims);
            return (uint16_t)((dims.right - dims.left)      )
                 | (uint32_t)((dims.bottom - dims.top) << 16);
        }
        case MSG__POS:
            MoveControl(ctrl, data);
            ResizeSpinControl(ctrl);
            break;

        case MSG__SHW:
            ShowWindow((HWND)ctrl->priv[0], (data)? SW_SHOW : SW_HIDE);
            ShowWindow((HWND)ctrl->priv[1], (data)? SW_SHOW : SW_HIDE);
            break;

        case MSG__ENB:
            EnableWindow((HWND)ctrl->priv[0], !!data);
            EnableWindow((HWND)ctrl->priv[1], !!data);
            break;

        case MSG_NGET:
            return SendMessage((HWND)ctrl->priv[1], UDM_GETPOS32, 0, 0);

        case MSG_NSET:
            data = (data > ctrl->priv[2])? data : ctrl->priv[2];
            data = (data < ctrl->priv[3])? data : ctrl->priv[3];
            SendMessage((HWND)ctrl->priv[1], UDM_SETPOS32, 0, data);
            break;

        case MSG_NDIM:
            ctrl->priv[2] = (int16_t)data;
            ctrl->priv[3] = (uint16_t)(data >> 16);
            SendMessage((HWND)ctrl->priv[1], UDM_SETRANGE32,
                        (WPARAM)ctrl->priv[2], (LPARAM)ctrl->priv[3]);
            break;
    }
    return 0;
}



intptr_t FE2CT(CTRL *ctrl, uint32_t cmsg, intptr_t data) {
    switch (cmsg) {
        case MSG__GSZ: {
            RECT dims;

            GetWindowRect((HWND)ctrl->priv[0], &dims);
            return (uint16_t)((dims.right - dims.left)      )
                 | (uint32_t)((dims.bottom - dims.top) << 16);
        }
        case MSG__POS:
            MoveControl(ctrl, data);
            break;

        case MSG__TXT:
            AssignTextToControl(ctrl, (char*)data);
            break;

        case MSG__ENB:
            EnableWindow((HWND)ctrl->priv[0], !!data);
            break;

        case MSG__SHW:
            ShowWindow((HWND)ctrl->priv[0], (data)? SW_SHOW : SW_HIDE);
            break;
    }
    return 0;
}



intptr_t FE2CS(CTRL *ctrl, uint32_t cmsg, intptr_t data) {
    switch (cmsg) {
        case MSG_WSZC: {
            POINT size;
            RECT dims;

            if ((ctrl->prev->flgs & FCT_TTTT) != FCT_WNDW)
                return -1;
            GetWindowRect((HWND)ctrl->priv[0], &dims);
            if (!data) {
                dims.bottom -= dims.top;
                dims.right -= dims.left + GetSystemMetrics(SM_CXVSCROLL);
                SendMessage((HWND)ctrl->priv[0], WM_SIZE, 0,
                            (uint16_t)dims.right
                          | (uint32_t)(dims.bottom << 16));
            }
            else {
                ScreenToClient((HWND)ctrl->prev->priv[0],  (POINT*)&dims);
                size.x = (uint16_t)(data      ) - dims.left - ctrl->prev->xdim
                       * (uint16_t)(ctrl->prev->priv[2]      );
                size.y = (uint16_t)(data >> 16) - dims.top  - ctrl->prev->ydim
                       * (uint16_t)(ctrl->prev->priv[2] >> 16);
                SetWindowPos((HWND)ctrl->priv[0], HWND_TOP, dims.left,
                              dims.top, size.x, size.y, SWP_SHOWWINDOW);
            }
            break;
        }
        case MSG__SHW:
            ShowWindow((HWND)ctrl->priv[0], (data)? SW_SHOW : SW_HIDE);
            break;
    }
    return 0;
}



intptr_t FE2CI(CTRL *ctrl, uint32_t cmsg, intptr_t data) {
    switch (cmsg) {
        case MSG__POS:
            MoveControl(ctrl, data);
            break;

        case MSG__SHW:
            ShowWindow((HWND)ctrl->priv[0], (data)? SW_SHOW : SW_HIDE);
            break;

        case MSG_IFRM:
            ctrl->priv[7] = data;
            InvalidateRect((HWND)ctrl->priv[0], 0, FALSE);
            break;
    }
    return 0;
}



void rFreeControl(CTRL *ctrl) {
    switch (ctrl->flgs & FCT_TTTT) {
        case FCT_WNDW:
            /// every window has its own font instance
            DeleteObject((HFONT)ctrl->priv[1]);
            break;

        case FCT_TEXT:
            break;

        case FCT_BUTN:
            break;

        case FCT_CBOX:
            break;

        case FCT_SPIN:
            DestroyWindow((HWND)ctrl->priv[1]); /// the very spin control
            break;

        case FCT_LIST:
            DestroyWindow((HWND)ctrl->priv[0]); /// freeing the listbox...
            ctrl->priv[0] = ctrl->priv[1];      /// ...and marking the parent
            break;

        case FCT_SBOX:
            DestroyWindow((HWND)ctrl->priv[7]); /// the scrolled area
            break;

        case FCT_IBOX:
            DeleteDC((HDC)ctrl->priv[1]);         /// the DIB context
            DeleteObject((HGDIOBJ)ctrl->priv[2]); /// the associated DIB
            break;

        case FCT_PBAR:
            /// font (priv[1]) is inherited, so it doesn`t need to be freed
            free((void*)ctrl->priv[2]); /// freeing the text line, if any
            break;
    }
    DestroyWindow((HWND)ctrl->priv[0]);
}



void RegClass(WNDCLASSEXA *wndc) {
    WNDCLASSEXA test;
    LPWSTR clas;

    if (OldWin32()) {
        if (!GetClassInfoExA(wndc->hInstance, wndc->lpszClassName, &test))
            RegisterClassExA(wndc);
    }
    else {
        clas = UTF16((LPSTR)wndc->lpszClassName);
        if (!GetClassInfoExW(wndc->hInstance, clas, (WNDCLASSEXW*)&test))
            RegisterClassExW((WNDCLASSEXW*)wndc);
        free(clas);
    }
}

void rMakeControl(CTRL *ctrl, long *xoff, long *yoff) {
    #define WC_MAINWND L"W"
    #define WC_LISTROOT L"L"
    #define WC_SIZEBOX L"S"
    #define WC_IMGBOX L"I"
    static struct {
        LPSTR name;
        DWORD wsty;
    } base[] = {
        {(LPSTR)WC_STATIC,      SS_CENTERIMAGE},
        {(LPSTR)WC_BUTTON,      BS_FLAT | BS_MULTILINE},
        {(LPSTR)WC_BUTTON,      BS_FLAT | BS_AUTOCHECKBOX},
        {(LPSTR)WC_EDIT,        ES_MULTILINE | ES_WANTRETURN | ES_AUTOHSCROLL},
        {(LPSTR)WC_LISTVIEW,    LVS_REPORT},
        {(LPSTR)PROGRESS_CLASS, PBS_SMOOTH},
        {(LPSTR)WC_SIZEBOX,     WS_CLIPCHILDREN | WS_VSCROLL},
        {(LPSTR)WC_IMGBOX,      WS_CLIPCHILDREN},
    };
    WNDCLASSEX wndc = {sizeof(wndc), CS_HREDRAW | CS_VREDRAW,
                       0, 0, 0, GetModuleHandle(0)};
    LONG xsty, wsty, xpos, ypos, xdim, ydim, xspc, yspc;
    HANDLE cwnd;
    LPSTR name;
    CTRL *root;

    cwnd = 0;
    root = ctrl->prev;
    if ((ctrl->flgs & FCT_TTTT) == FCT_WNDW) {
        TEXTMETRIC tmet;

        ctrl->priv[1] = (typeof(*ctrl->priv))
            CreateFont(14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                       DEFAULT_CHARSET, 0, 0, 0, 0, "Verdana");
        cwnd = CreateCompatibleDC(0);
        SelectObject(cwnd, (HFONT)ctrl->priv[1]);
        GetTextMetrics(cwnd, &tmet);
        DeleteDC(cwnd);
        ctrl->priv[4] = tmet.tmHeight + tmet.tmDescent; /// for spin controls
        ctrl->priv[2] =  (uint16_t)(tmet.tmAveCharWidth * 1.7)
                      | ((uint32_t)(tmet.tmHeight * 0.7) << 16);
        wndc.hIcon = LoadIcon(wndc.hInstance, MAKEINTRESOURCE(1));
        wndc.hCursor = LoadCursor(0, IDC_ARROW);
        wndc.lpfnWndProc = OptProc;
        wndc.lpszClassName = (LPSTR)WC_MAINWND;
        wndc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
        RegClass(&wndc);
        xsty = (ctrl->flgs & FSW_SIZE)? 0 : WS_EX_TOOLWINDOW;
        wsty = WS_CLIPCHILDREN
             | ((ctrl->flgs & FSW_SIZE)? WS_OVERLAPPEDWINDOW : WS_SYSMENU);
        cwnd = CreateWindowEx(xsty, (LPSTR)WC_MAINWND, 0, wsty,
                              0, 0, 0, 0, 0, 0, wndc.hInstance, 0);
        SetWindowLongPtr(cwnd, GWLP_USERDATA, (LONG_PTR)ctrl);
        ctrl->fe2c = FE2CW;
    }
    else if (root) {
        xsty = 0;
        wsty = ctrl->flgs & FCT_TTTT;
        name = base[wsty - 1].name;
        wsty = base[wsty - 1].wsty
             | WS_CHILD |  ((wsty != FCT_WNDW)? WS_VISIBLE : 0)
                        | (((wsty == FCT_TEXT) ||
                            (wsty == FCT_PBAR) ||
                            (wsty == FCT_SBOX))? 0 : WS_TABSTOP);
        xdim = ((ctrl->prev->flgs & FCT_TTTT) != FCT_SBOX)? 0 : 7;
        cwnd = (HWND)ctrl->prev->priv[xdim];

        while (root->prev)
            root = root->prev;
        xspc = (uint16_t)(root->priv[2]);
        yspc = (uint16_t)(root->priv[2] >> 16);
        xpos =  ctrl->xpos + ((xoff && (ctrl->flgs & FCP_HORZ))?
                              *xoff : root->xpos);
        ypos =  ctrl->ypos + ((yoff && (ctrl->flgs & FCP_VERT))?
                              *yoff : root->ypos);
        xdim = (ctrl->xdim < 0)? -ctrl->xdim : ctrl->xdim * xspc;
        ydim = (ctrl->ydim < 0)? -ctrl->ydim : ctrl->ydim * yspc;
        if (xoff)
            *xoff = xpos
                  + ((ctrl->xdim < 0)? 1 - ctrl->xdim / xspc : ctrl->xdim);
        if (yoff)
            *yoff = ypos
                  + ((ctrl->ydim < 0)? 1 - ctrl->ydim / yspc : ctrl->ydim);

        switch (ctrl->flgs & FCT_TTTT) {
            case FCT_TEXT:
                wsty |= (ctrl->flgs & FST_SUNK)? SS_SUNKEN : 0;
                wsty |= (ctrl->flgs & FST_CNTR)? SS_CENTER : 0;
                ctrl->fe2c = FE2CT;
                break;

            case FCT_BUTN:
                wsty |= (ctrl->flgs & FSB_DFLT)? BS_DEFPUSHBUTTON : 0;
                ctrl->fe2c = FE2CX;
                break;

            case FCT_CBOX:
                wsty |= (ctrl->flgs & FSX_LEFT)? BS_LEFTTEXT : 0;
                ctrl->fe2c = FE2CX;
                break;

            case FCT_SPIN:
                xsty = WS_EX_CLIENTEDGE;
                ctrl->priv[4] = root->priv[4]; /// copying alignment
                ctrl->fe2c = FE2CN;
                break;

            case FCT_LIST:
                wndc.hCursor = LoadCursor(0, IDC_ARROW);
                wndc.lpfnWndProc = ListProc;
                wndc.lpszClassName = (LPSTR)WC_LISTROOT;
                wndc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
                RegClass(&wndc);
                cwnd = CreateWindowEx(0, (LPSTR)WC_LISTROOT,
                                      0, WS_CHILD | WS_VISIBLE,
                                      xpos * xspc, ypos * yspc, xdim, ydim,
                                      cwnd, (HMENU)ctrl->uuid, 0, ctrl);
                xpos = ypos = 0;
                ctrl->priv[1] = (intptr_t)cwnd;
                xsty = WS_EX_CLIENTEDGE;
                ctrl->fe2c = FE2CL;
                break;

            case FCT_SBOX:
                wndc.hCursor = LoadCursor(0, IDC_ARROW);
                wndc.lpfnWndProc = SBoxProc;
                wndc.lpszClassName = (LPSTR)WC_SIZEBOX;
                wndc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
                RegClass(&wndc);
                ctrl->fe2c = FE2CS;
                break;

            case FCT_IBOX:
                wndc.hCursor = LoadCursor(0, IDC_ARROW);
                wndc.lpfnWndProc = IBoxProc;
                wndc.lpszClassName = (LPSTR)WC_IMGBOX;
                wndc.hbrBackground = (HBRUSH)COLOR_MENU;
                RegClass(&wndc);
                ctrl->fe2c = FE2CI;
                break;

            case FCT_PBAR:
                ctrl->fe2c = FE2CP;
                ctrl->priv[1] = root->priv[1]; /// copying font
                ctrl->priv[4] = 1;
                break;
        }
        cwnd = CreateWindowEx(xsty, name, 0, wsty, xpos * xspc, ypos * yspc,
                              xdim, ydim, cwnd, (HMENU)ctrl->uuid, 0, ctrl);
        SendMessage(cwnd, WM_SETFONT, (WPARAM)root->priv[1], FALSE);
        SetWindowLongPtr(cwnd, GWLP_USERDATA, (LONG_PTR)ctrl);

        switch (ctrl->flgs & FCT_TTTT) {
            case FCT_LIST: {
                LVCOLUMN vcol = {LVCF_WIDTH | LVCF_TEXT, 0, 0, ""};
                RECT rect;

                GetClientRect(cwnd, &rect);
                vcol.cx = rect.right - GetSystemMetrics(SM_CXVSCROLL);
                if (OldWin32()) {
                    SendMessageA(cwnd, LVM_INSERTCOLUMNA, 0, (LPARAM)&vcol);
                }
                else
                    SendMessageW(cwnd, LVM_INSERTCOLUMNW, 0, (LPARAM)&vcol);
                SendMessage(cwnd, LVM_SETCALLBACKMASK,
                            LVIS_STATEIMAGEMASK, 0);
                SendMessage(cwnd, LVM_SETEXTENDEDLISTVIEWSTYLE, 0,
                            LVS_EX_CHECKBOXES | LVS_EX_GRIDLINES |
                            LVS_EX_FULLROWSELECT);
                break;
            }
            case FCT_SPIN: {
                HWND spin;

                ctrl->priv[0] = (typeof(*ctrl->priv))cwnd;
                SendMessage(cwnd, EM_LIMITTEXT, 9, 0);
                xdim = ((ctrl->prev->flgs & FCT_TTTT) != FCT_SBOX)? 0 : 7;
                spin = CreateWindowEx(0, UPDOWN_CLASS, 0, UDS_HOTTRACK
                                    | UDS_NOTHOUSANDS | UDS_ALIGNRIGHT
                                    | UDS_SETBUDDYINT | UDS_ARROWKEYS
                                    | WS_CHILD | WS_VISIBLE, 0, 0, 0, 0,
                                     (HWND)ctrl->prev->priv[xdim], 0, 0, 0);
                SetWindowLongPtr(spin, GWLP_USERDATA, (LONG_PTR)ctrl);
                ctrl->priv[1] = (typeof(*ctrl->priv))spin;
                ctrl->priv[7] = (typeof(*ctrl->priv))
                    SetWindowLongPtr(cwnd, GWLP_WNDPROC, (LONG_PTR)SpinProc);
                ResizeSpinControl(ctrl);
                break;
            }
            case FCT_PBAR:
                ctrl->priv[7] = (typeof(*ctrl->priv))
                    SetWindowLongPtr(cwnd, GWLP_WNDPROC, (LONG_PTR)PBarProc);
                break;

            case FCT_SBOX:
                ctrl->priv[7] =
                    (intptr_t)CreateWindowEx(0, name, 0, WS_CLIPCHILDREN |
                                             WS_CHILD | WS_VISIBLE, 0, 0,
                                             0, 0, cwnd, 0, 0, 0);
                break;

            case FCT_IBOX: {
                BITMAPINFO bmpi = {{sizeof(bmpi.bmiHeader),
                                    0, 0, 1, 32, BI_RGB}};
                bmpi.bmiHeader.biWidth =   xdim;
                bmpi.bmiHeader.biHeight = -ydim;
                ctrl->priv[4] = (uint16_t)(xdim      )
                              | (uint32_t)(ydim << 16);
                ctrl->priv[1] = (intptr_t)CreateCompatibleDC(0);
                ctrl->priv[2] =
                    (intptr_t)CreateDIBSection((HDC)ctrl->priv[1],
                                               &bmpi, DIB_RGB_COLORS,
                                               (void*)&ctrl->priv[3], 0, 0);
                SelectObject((HDC)ctrl->priv[1], (HGDIOBJ)ctrl->priv[2]);
                break;
            }
        }
    }
    ctrl->priv[0] = (typeof(*ctrl->priv))cwnd;
    #undef WC_IMGBOX
    #undef WC_SIZEBOX
    #undef WC_LISTROOT
    #undef WC_MAINWND
}



void rInternalMainLoop(CTRL *root, uint32_t fram, UPRE upre, intptr_t data) {
    uint64_t time, tcur;
    uint32_t temp;
    MSG pmsg;

    time = tcur = 0;
    while (pmsg.message != WM_QUIT) {
        if (PeekMessage(&pmsg, 0, 0, 0, PM_REMOVE)) {
            if (!IsDialogMessage((HWND)root->priv[0], &pmsg)) {
                TranslateMessage(&pmsg);
                DispatchMessage(&pmsg);
            }
            continue;
        }
        if ((temp = GetTickCount()) < (uint32_t)tcur)
            tcur += 0x100000000ULL;
        if ((tcur = temp | (tcur & 0xFFFFFFFF00000000ULL)) - time < fram) {
            Sleep(1);
            continue;
        }
        time = tcur;
        upre(data, time);
    }
}



intptr_t rFindMake(char *base) {
    FIND *find;
    char *temp;
    long  size;

    if (!base)
        return 0;
    size = strlen(base);
    memcpy(temp = calloc(1, 32 + size), base, size);
    memcpy(temp + size, "/*", sizeof("/*"));
    find = calloc(1, sizeof(*find));
    find->base = rConvertUTF8(temp);
    find->hdir = INVALID_HANDLE_VALUE;
    free(temp);
    return (intptr_t)find;
}



char *rFindFile(intptr_t data) {
    FIND *find = (FIND*)data;
    union {
        WIN32_FIND_DATAA a;
        WIN32_FIND_DATAW w;
    } fdir;
    char *retn;

    if (!find)
        return 0;
    if (!OldWin32()) {
        if (find->hdir == INVALID_HANDLE_VALUE)
            find->hdir = FindFirstFileW((LPWSTR)find->base, &fdir.w);
        else if (!FindNextFileW(find->hdir, &fdir.w)) {
            FindClose(find->hdir);
            find->hdir = INVALID_HANDLE_VALUE;
        }
        retn = (find->hdir != INVALID_HANDLE_VALUE)?
                UTF8(fdir.w.cFileName) : 0;
    }
    else {
        if (find->hdir == INVALID_HANDLE_VALUE)
            find->hdir = FindFirstFileA(find->base, &fdir.a);
        else if (!FindNextFileA(find->hdir, &fdir.a)) {
            FindClose(find->hdir);
            find->hdir = INVALID_HANDLE_VALUE;
        }
        retn = (find->hdir != INVALID_HANDLE_VALUE)?
                strdup(fdir.a.cFileName) : 0;
    }
    if (!retn) {
        free(find->base);
        free(find);
    }
    return retn;
}



int APIENTRY bCBP(HWND hWnd, UINT uMsg, LPARAM lPrm, LPARAM wPrm) {
    if (uMsg == BFFM_INITIALIZED) {
        if (OldWin32())
            SendMessageA(hWnd, BFFM_SETSELECTIONA, TRUE, wPrm);
        else
            SendMessageW(hWnd, BFFM_SETSELECTIONW, TRUE, wPrm);
    }
    return 0;
}

char *rChooseDir(CTRL *root, char *base) {
    char buff[(MAX_PATH + 2) * 2];
    union {
        BROWSEINFOA a;
        BROWSEINFOW w;
    } binf = {{(HWND)root->priv[0], 0, buff, 0,
                BIF_DONTGOBELOWDOMAIN | BIF_EDITBOX
              | BIF_RETURNONLYFSDIRS  | BIF_NEWDIALOGSTYLE, bCBP}};
    LPITEMIDLIST retn;

    binf.a.lParam = (LPARAM)rConvertUTF8(base = BackReslash(strdup(base)));
    free(base);
    base = 0;
    buff[0] = buff[1] = 0;
    if ((retn = SHBF(&binf.a))) {
        SHGP(retn, buff);
        CoTaskMemFree(retn);
        base = (OldWin32())? strdup(buff) : UTF8((LPWSTR)buff);
    }
    free((PVOID)binf.a.lParam);
    return base;
}

char *rChooseFile(CTRL *root, char *fext, char *file) {
    union {
        OPENFILENAMEA a;
        OPENFILENAMEW w;
    } hofn = {{sizeof(hofn), (HWND)root->priv[0],
              .nMaxFile = MAX_PATH, .nFilterIndex = 1,
              .Flags = OFN_HIDEREADONLY | OFN_FILEMUSTEXIST
                     | OFN_ENABLESIZING | OFN_NONETWORKBUTTON}};
    char *retn, *exts, extt[64];
    long quit = 0;

    retn = BackReslash(strdup(file));
    sprintf(extt, "*.%s\n*.%s\n", fext, fext);
    exts = rConvertUTF8(extt);
    file = realloc(rConvertUTF8(retn), (MAX_PATH + 1) * 4);
    free(retn);
    retn = file;
    if (OldWin32()) {
        hofn.a.lpstrFile = retn;
        hofn.a.lpstrFilter = exts;
        for (quit = strlen(exts); quit > 0; quit--)
            if (exts[quit] == '\n')
                exts[quit] = 0;
        quit = GetOpenFileNameA(&hofn.a);
    }
    else {
        hofn.w.lpstrFile = (LPWSTR)retn;
        hofn.w.lpstrFilter = (LPWSTR)exts;
        for (quit = wcslen((LPWSTR)exts); quit > 0; quit--)
            if (((LPWSTR)exts)[quit] == '\n')
                ((LPWSTR)exts)[quit] = 0;
        quit = GetOpenFileNameW(&hofn.w);
        retn = UTF8((LPWSTR)(file = retn));
        free(file);
    }
    if (!quit) {
        free(retn);
        retn = 0;
    }
    free(exts);
    return retn;
}



int APIENTRY WinMain(HINSTANCE inst, HINSTANCE prev, LPSTR cmdl, int show) {
    INITCOMMONCONTROLSEX icct = {sizeof(icct), ICC_STANDARD_CLASSES};
    RECT area = {MAXLONG, MAXLONG, MINLONG, MINLONG};
    CHAR *name, *conf = 0, path[(MAX_PATH + 2) * 2] = {};
    HMODULE hlib;

//    if (flgs & FLG_CONS) {
//        AllocConsole();
//        freopen("CONOUT$", "wb", stdout);
//    }

    OleInitialize(0);
    InitCommonControlsEx(&icct);
    EnumDisplayMonitors(0, 0, CalcScreen, (LPARAM)&area);

    hlib = LoadLibrary((OldWin32())? "shfolder" : "shell32");
    SHGF = (PVOID)GetProcAddress(hlib, (OldWin32())? "SHGetFolderPathA"
                                                   : "SHGetFolderPathW");
    SHFO = (PVOID)GetProcAddress(hlib, (OldWin32())? "SHFileOperationA"
                                                   : "SHFileOperationW");
    SHBF = (PVOID)GetProcAddress(hlib, (OldWin32())? "SHBrowseForFolderA"
                                                   : "SHBrowseForFolderW");
    SHGP = (PVOID)GetProcAddress(hlib, (OldWin32())? "SHGetPathFromIDListA"
                                                   : "SHGetPathFromIDListW");
    if (SHGF(NULL, CSIDL_APPDATA | CSIDL_FLAG_CREATE, NULL,
             SHGFP_TYPE_CURRENT, path) == S_OK) {
        if (!OldWin32()) {
            strcpy(path, conf = UTF8((LPWSTR)path));
            free(conf);
        }
        if (rMakeDir(strcat(path, DEF_OPTS)))
            conf = strdup(path);
        else {
            printf("WARNING: cannot create '%s'!", path);
            conf = 0;
        }
    }
    if (OldWin32()) {
        GetModuleFileNameA(0, path, MAX_PATH);
        name = BackReslash(strdup(path));
    }
    else {
        GetModuleFileNameW(0, (LPWSTR)path, MAX_PATH);
        name = BackReslash(UTF8((LPWSTR)path));
    }
    for (show = strlen(name) - 1; show >= 0; show--)
        if (name[show] == '\\') {
            name[show] = 0;
            break;
        }
    eExecuteEngine(conf, name, GetSystemMetrics(SM_CXSMICON),
                   GetSystemMetrics(SM_CYSMICON),
                   area.left, area.top, area.right, area.bottom);
    FreeLibrary(hlib);
    OleUninitialize();
    fclose(stdout);
    FreeConsole();
    free(name);
    free(conf);
    exit(0);
    return 0;
}
