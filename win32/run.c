#define _WIN32_IE 0x0300
#define _WIN32_WINNT 0x0501
#define WINVER _WIN32_WINNT

#include <windows.h>
#include <commctrl.h>

#include "rsrc/run.h"
#include "../exec/exec.h"



#define WM_TRAY (WM_USER + 1000)

#define FLG_CONS (1 << 16)
#define FLG_EOGL (1 << 17)
#define FLG_IBGR (1 << 18)
#define FLG_IPBO (1 << 19)
#define FLG_IOPQ (1 << 20)
#define FLG_IRGN (1 << 21)



long OldWin32() {
    static long retn = 2;

    if (retn == 2)
        retn = ((GetVersion() & 0xFF) < 5)? 1 : 0;
    return retn;
}



char *UTF8(LPWSTR wide) {
    long size = WideCharToMultiByte(CP_UTF8, 0, wide, -1, 0, 0, 0, 0);
    char *retn = calloc(size * 2, sizeof(*retn));
    WideCharToMultiByte(CP_UTF8, 0, wide, -1, retn, size * 2, 0, 0);
    return retn;
}



LPWSTR UTF16(char *utf8) {
    long size = (strlen(utf8) + 1) * 4;
    LPWSTR retn = calloc(size, sizeof(*retn));
    MultiByteToWideChar(CP_UTF8, 0, utf8, -1, retn, size);
    return retn;
}



char *ConvertUTF8(char *utf8) {
    if (!utf8)
        return utf8;

    LPWSTR wide = UTF16(utf8);
    long size;

    if (OldWin32()) {
        utf8 = calloc((size = (wcslen(wide) + 1) * 4), sizeof(*utf8));
        WideCharToMultiByte(CP_ACP, WC_NO_BEST_FIT_CHARS, wide, -1,
                            utf8, size - 1, "#", 0);
        free(wide);
        return utf8;
    }
    else
        return (char*)wide;
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



void SetTrayIconText(uintptr_t icon, char *text) {
    char *hint = ConvertUTF8(text);

    if (OldWin32()) {
        strcpy((char*)((NOTIFYICONDATAA*)icon)->szTip, hint);
        Shell_NotifyIconA(NIM_MODIFY, (NOTIFYICONDATAA*)icon);
    }
    else {
        wcscpy(((NOTIFYICONDATAW*)icon)->szTip, (LPWSTR)hint);
        Shell_NotifyIconW(NIM_MODIFY, (NOTIFYICONDATAW*)icon);
    }
    free(hint);
}



#define MMI_CONS 1
#define MMI_IRGN 2
#define MMI_IBGR 3
#define MMI_IPBO 4

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
/** TODO **/
//            if (engc->rscm != SCM_ROGL)
//                Message(0, (char*)engc->tran[TXT_UWGL],
//                        0, MB_OK | MB_ICONEXCLAMATION);
//            else
                EngineCallback(engc->engh, ECB_QUIT, ~0);
            break;
        }
    }
}



MENU *OSSpecificMenu(ENGC *engc) {
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
}



HMENU Submenu(MENU *menu, ulong *chld) {
    if (!menu)
        return 0;

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
    HWND  iwnd = CreateWindowEx(0, WC_STATIC, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
    HMENU mwnd = Submenu(menu, 0);
    POINT ppos;
    DWORD retn;

    GetCursorPos(&ppos);
    SetForegroundWindow(iwnd);
    if ((retn = TrackPopupMenu(mwnd, TPM_NONOTIFY | TPM_RETURNCMD,
                               ppos.x, ppos.y, 0, iwnd, 0))) {
        GetMenuItemInfoA(mwnd, retn, FALSE, &pmii);
        ProcessMenuItem((MENU*)pmii.dwItemData);
    }
    DestroyWindow(iwnd);
    DestroyMenu(mwnd);
    return TRUE;
}



void OpenContextMenu(MENU *menu) {
    DWORD retn;

    CreateThread(0, 0, (LPTHREAD_START_ROUTINE)MenuThread, menu, 0, &retn);
}



BOOL APIENTRY EnterProc(HWND hWnd, UINT uMsg, WPARAM wPrm, LPARAM lPrm) {
    switch (uMsg) {
        case WM_INITDIALOG: {
            HWND temp = GetDlgItem(hWnd, MSC_INST);
            SendMessage(temp, UDM_SETRANGE, 0, MAKELPARAM(4096, 1));
            SendMessage(temp, UDM_SETPOS, 0, 128);
            SendMessage(GetDlgItem(hWnd, MCB_EOGL),
                        BM_SETCHECK, BST_CHECKED, 0);
            if (OldWin32()) {
                EnableWindow(temp = GetDlgItem(hWnd, MCB_IRGN), FALSE);
                SendMessage(temp, BM_SETCHECK, BST_CHECKED, 0);
            }
            return TRUE;
        }

        case WM_COMMAND:
            switch (LOWORD(wPrm)) {
                case IDOK:
                    uMsg = (SendMessage(GetDlgItem(hWnd, MSC_INST),
                                        UDM_GETPOS,  0, 0) & 0xFFFF)
                         | (SendMessage(GetDlgItem(hWnd, MCB_IOPQ),
                                        BM_GETCHECK, 0, 0)? FLG_IOPQ : 0)
                         | (SendMessage(GetDlgItem(hWnd, MCB_IRGN),
                                        BM_GETCHECK, 0, 0)? FLG_IRGN : 0)
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

                case MCB_IOPQ:
                    lPrm = !SendMessage(GetDlgItem(hWnd, MCB_IOPQ),
                                        BM_GETCHECK, 0, 0);
                    lPrm = (OldWin32())? FALSE : lPrm;
                    EnableWindow(GetDlgItem(hWnd, MCB_IRGN), lPrm);
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



BOOL APIENTRY CalcScreen(HMONITOR hmon, HDC hdcm, LPRECT rect, LPARAM data) {
    LPRECT retn = (LPRECT)data;

    retn->left   = min(retn->left,   rect->left  );
    retn->right  = max(retn->right,  rect->right );
    retn->top    = min(retn->top,    rect->top   );
    retn->bottom = max(retn->bottom, rect->bottom);

    return TRUE;
}



char *LoadFileZ(char *name, long *size) {
    char *retn = 0;
    HANDLE file;
    DWORD flen;

    name = ConvertUTF8(name);
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



long SaveFile(char *name, char *data, long size) {
    HANDLE file;
    DWORD flen;

    name = ConvertUTF8(name);
    if (OldWin32())
        file = CreateFileA(name, GENERIC_WRITE, FILE_SHARE_READ, 0,
                           CREATE_ALWAYS, FILE_ATTRIBUTE_TEMPORARY, 0);
    else
        file = CreateFileW((LPWSTR)name, GENERIC_WRITE, FILE_SHARE_READ, 0,
                           CREATE_ALWAYS, FILE_ATTRIBUTE_TEMPORARY, 0);
    free(name);
    if (file != INVALID_HANDLE_VALUE) {
        WriteFile(file, data, size, &flen, 0);
        CloseHandle(file);
        return flen;
    }
    return 0;
}



int APIENTRY WinMain(HINSTANCE inst, HINSTANCE prev, LPSTR cmdl, int show) {
    INCBIN("../core/icon.gif", MainIcon);

    INITCOMMONCONTROLSEX icct = {sizeof(icct), ICC_STANDARD_CLASSES};
    NOTIFYICONDATAW nicd = {sizeof(nicd), 0, 1,
                            NIF_MESSAGE | NIF_ICON | NIF_TIP, WM_TRAY};
    RECT temp = {MAXLONG, MAXLONG, MINLONG, MINLONG};
    AINF igif = {};
    ENGC engc = {};

    uint32_t flgs;
    char path[MAX_PATH], file[MAX_PATH];

    InitCommonControlsEx(&icct);
    GetTempPath(MAX_PATH, path);
    flgs = GetTickCount();
    sprintf(file, "%s%08X.gif", path, PRNG(&flgs));
    if (!(flgs = DialogBoxParam(inst, MAKEINTRESOURCE(DLG_MAIN),
                                0, EnterProc, 0)))
        return 0;

    if (flgs & FLG_CONS) {
        AllocConsole();
        freopen("CONOUT$", "wb", stdout);
    }
    EngineCallback(0, ECB_INIT, (uintptr_t)&engc.engh);

    WIN32_FIND_DATAW dirw;
    WIN32_FIND_DATAA dira;
    HANDLE hdir = FindFirstFileW(L""DEF_FLDR""DEF_DSEP"*", &dirw);
    if (GetLastError() != ERROR_CALL_NOT_IMPLEMENTED)
        while ((hdir != INVALID_HANDLE_VALUE) &&
               (GetLastError() != ERROR_NO_MORE_FILES)) {
            char *temp = UTF8(dirw.cFileName);
            AppendLib(&engc, DEF_CONF, DEF_FLDR, temp);
            free(temp);
            FindNextFileW(hdir, &dirw);
        }
    else {
        hdir = FindFirstFileA(DEF_FLDR""DEF_DSEP"*", &dira);
        while ((hdir != INVALID_HANDLE_VALUE) &&
               (GetLastError() != ERROR_NO_MORE_FILES)) {
            AppendLib(&engc, DEF_CONF, DEF_FLDR, dira.cFileName);
            FindNextFileA(hdir, &dira);
        }
    }
    FindClose(hdir);
    SaveFile(file, MainIcon, MainIcon_end - MainIcon);
    EngineLoadAnimAsync(engc.engh, (uint8_t*)file, &igif);
    EngineCallback(engc.engh, ECB_LOAD, 0);
    DeleteFile(file);

    /// [TODO:] substitute this by GUI selection
    LINF *libs = engc.libs;
    while (libs) {
        libs->icnt = flgs & 0xFFFF;
        libs = (LINF*)libs->prev;
    }

    igif.fcnt = 0;
    igif.xdim = GetSystemMetrics(SM_CXSMICON);
    igif.ydim = GetSystemMetrics(SM_CYSMICON);
    igif.time = calloc(sizeof(*igif.time), igif.xdim * igif.ydim);
    EngineCallback(engc.engh, ECB_DRAW, (uintptr_t)&igif);

    LONG x, y, xoff = (igif.xdim >> 3) + ((igif.xdim & 7)? 1 : 0);
    ICONINFO icon = {};
    LPBYTE tran;

    tran = malloc(igif.ydim * xoff);
    for (y = 0; y < igif.ydim; y++)
        for (x = 0; x < igif.xdim; x++)
            if (igif.time[igif.xdim * y + x] & 0xFF000000)
                tran[xoff * y + (x >> 3)] &= ~(0x80 >> (x & 7));
            else
                tran[xoff * y + (x >> 3)] |=  (0x80 >> (x & 7));

    icon.hbmMask  = CreateBitmap(igif.xdim, igif.ydim, 1,  1, tran);
    icon.hbmColor = CreateBitmap(igif.xdim, igif.ydim, 1, 32, igif.time);
    nicd.hWnd  = CreateWindowEx(0, WC_STATIC, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
    nicd.hIcon = CreateIconIndirect(&icon);
    DeleteObject(icon.hbmColor);
    DeleteObject(icon.hbmMask);
    free(igif.time);
    free(tran);

    if (OldWin32())
        Shell_NotifyIconA(NIM_ADD, (NOTIFYICONDATAA*)&nicd);
    else
        Shell_NotifyIconW(NIM_ADD, &nicd);

    EnumDisplayMonitors(0, 0, CalcScreen, (LPARAM)&temp);
    ExecuteEngine(&engc, temp.left, temp.top, temp.right - temp.left,
                   temp.bottom - temp.top, (uintptr_t)&nicd,
                  (flgs & FLG_EOGL)? SCM_ROGL : SCM_RSTD,
                 ((flgs & FLG_IBGR)? WIN_IBGR : 0) |
                 ((flgs & FLG_IPBO)? WIN_IPBO : 0) | COM_SHOW |
                 ((flgs & FLG_IRGN)? WIN_IRGN : 0) | COM_DRAW |
                 ((flgs & FLG_IOPQ)? COM_OPAQ : 0), 0);
    if (OldWin32())
        Shell_NotifyIconA(NIM_DELETE, (NOTIFYICONDATAA*)&nicd);
    else
        Shell_NotifyIconW(NIM_DELETE, &nicd);
    DestroyIcon(nicd.hIcon);

    fclose(stdout);
    FreeConsole();
    return 0;
}
