#define _WIN32_IE 0x0500
#define _WIN32_WINNT 0x0501
#define WINVER _WIN32_WINNT

#include <windows.h>
#include <commctrl.h>
#include <shlobj.h>

#include "../exec/exec.h"



#define WM_TRAY (WM_USER + 1000)

#define FLG_CONS (1 << 16)
#define FLG_EOGL (1 << 17)
#define FLG_IBGR (1 << 18)
#define FLG_IPBO (1 << 19)
#define FLG_IOPQ (1 << 20)
#define FLG_IRGN (1 << 21)



static inline long OldWin32() {
    static long retn = 2;

    if (retn == 2)
        retn = ((GetVersion() & 0xFF) < 5)? 1 : 0;
    return retn;
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



long rMessage(char *text, char *head, uint32_t flgs) {
    char *tttt = rConvertUTF8(text),
         *hhhh = rConvertUTF8(head);

    union {
        MSGBOXPARAMSA a;
        MSGBOXPARAMSW w;
    } msgp = {{sizeof(msgp), 0, GetModuleHandle(0), tttt, hhhh,
               flgs | MB_TASKMODAL | MB_USERICON, (LPSTR)1}};

    long retn = (OldWin32())? MessageBoxIndirectA(&msgp.a)
                            : MessageBoxIndirectW(&msgp.w);
    free(tttt);
    free(hhhh);
    return retn;
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



MENU *rOSSpecificMenu(ENGC *engc) {
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
        MENUITEMINFOW w;
        MENUITEMINFOA a;
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



DWORD APIENTRY MenuThread(MENU *menu) {
    MENUITEMINFOA pmii = {sizeof(pmii), MIIM_DATA};
    HWND  iwnd = CreateWindowEx(0, WC_STATIC, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
    HMENU mwnd = Submenu(menu, 0);
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

    CreateThread(0, 0, (LPTHREAD_START_ROUTINE)MenuThread, menu, 0, &retn);
}



LRESULT APIENTRY TrayProc(HWND hWnd, UINT uMsg, WPARAM wPrm, LPARAM lPrm) {
    if (uMsg == WM_TRAY) {
        if (lPrm == WM_RBUTTONDOWN)
            rOpenContextMenu((MENU*)GetWindowLongPtr(hWnd, GWLP_USERDATA));
        return 0;
    }
    return DefWindowProc(hWnd, uMsg, wPrm, lPrm);
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
    free(data);
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



int APIENTRY WinMain(HINSTANCE inst, HINSTANCE prev, LPSTR cmdl, int show) {
    INITCOMMONCONTROLSEX icct = {sizeof(icct), ICC_STANDARD_CLASSES};
    RECT area = {MAXLONG, MAXLONG, MINLONG, MINLONG};
    CHAR *conf, path[4 * (MAX_PATH + 1)] = {};
    LPWSTR wide = 0;
    HANDLE hdir;
    ENGC *engc;
    BOOL retn;

//    if (flgs & FLG_CONS) {
        AllocConsole();
        freopen("CONOUT$", "wb", stdout);
//    }

    retn = 0;
    if (OldWin32()) {
        if (SHGetFolderPathA(NULL, CSIDL_APPDATA | CSIDL_FLAG_CREATE, NULL,
                             SHGFP_TYPE_CURRENT, (LPSTR)path) == S_OK) {
            strcat(path, DEF_OPTS);
            wide = UTF16(path);
            retn = CreateDirectoryA((LPSTR)path, 0);
            if (!retn && (GetLastError() == ERROR_ALREADY_EXISTS))
                retn = 1;
        }
    }
    else {
        if (SHGetFolderPathW(NULL, CSIDL_APPDATA | CSIDL_FLAG_CREATE, NULL,
                             SHGFP_TYPE_CURRENT, (LPWSTR)path) == S_OK) {
            wcscat((LPWSTR)path, L""DEF_OPTS);
            wide = wcsdup((LPWSTR)path);
            retn = CreateDirectoryW((LPWSTR)path, 0);
            if (!retn && (GetLastError() == ERROR_ALREADY_EXISTS))
                retn = 1;
        }
    }
    if (!(conf = (retn && wide)? UTF8(wide) : 0))
        printf("WARNING: cannot create '%s'!", conf);
    free(wide);

    engc = eInitializeEngine(conf);
    free(conf);

    if (OldWin32()) {
        WIN32_FIND_DATAA dirf;

        hdir = FindFirstFileA(DEF_FLDR"/*", &dirf);
        while ((hdir != INVALID_HANDLE_VALUE) &&
               (GetLastError() != ERROR_NO_MORE_FILES)) {
            eAppendLib(engc, DEF_CONF, DEF_FLDR, dirf.cFileName);
            FindNextFileA(hdir, &dirf);
        }
    }
    else {
        WIN32_FIND_DATAW dirf;

        hdir = FindFirstFileW(L""DEF_FLDR"/*", &dirf);
        while ((hdir != INVALID_HANDLE_VALUE) &&
               (GetLastError() != ERROR_NO_MORE_FILES)) {
            char *temp = UTF8(dirf.cFileName);
            eAppendLib(engc, DEF_CONF, DEF_FLDR, temp);
            free(temp);
            FindNextFileW(hdir, &dirf);
        }
    }
    FindClose(hdir);


    /// [TODO:] substitute this by GUI selection
    __DEL_ME__SetLibUses(engc, 1);


    InitCommonControlsEx(&icct);
    EnumDisplayMonitors(0, 0, CalcScreen, (LPARAM)&area);
    eExecuteEngine(engc, GetSystemMetrics(SM_CXSMICON),
                   GetSystemMetrics(SM_CYSMICON),
                   area.left, area.top, area.right, area.bottom);
    fclose(stdout);
    FreeConsole();
    return 0;
}
