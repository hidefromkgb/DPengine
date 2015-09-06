#define _WIN32_IE 0x0300
#define _WIN32_WINNT 0x0501
#define WINVER _WIN32_WINNT

#include <time.h>
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



char *UTF8(LPWSTR wide) {
    long size = WideCharToMultiByte(CP_UTF8, 0, wide, -1, 0, 0, 0, 0);
    char *retn = calloc(size * 2, sizeof(*retn));
    WideCharToMultiByte(CP_UTF8, 0, wide, -1, retn, size * 2, 0, 0);
    return retn;
}



char *LoadFileZ(char *name, long *size) {
    DWORD temp, flen;
    char *retn = 0;

    temp = strlen(name) * 4 + 2;
    LPWSTR wide = calloc(temp, sizeof(*wide));
    MultiByteToWideChar(CP_UTF8, 0, name, -1, wide, temp);

    HANDLE file = CreateFileW(wide, GENERIC_READ, FILE_SHARE_READ, 0,
                              OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    free(wide);
    if (GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)
        file = CreateFileA(name, GENERIC_READ, FILE_SHARE_READ, 0,
                           OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
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



long OldWin32() {
    return ((GetVersion() & 0xFF) < 5)? 1 : 0;
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
/** TODO **/
//            ENGD *engc = (ENGD*)item->data;

//          if (item->flgs & MFL_VCHK)
//              engc->flgs |= WIN_IRGN;
//          else
//              engc->flgs &= ~WIN_IRGN;
//          if (engc->flgs & COM_IOPQ)
//              Message((HWND)engc->user[0], (char*)engc->tran[TXT_UOFO],
//                       0, MB_OK | MB_ICONEXCLAMATION);
//            else
//                RestartEngine(engc, engc->rscm);
            break;
        }
        case MMI_IBGR:
        case MMI_IPBO: {
/** TODO **/
//            ENGD *engc = (ENGD*)item->data;

//            if (item->flgs & MFL_VCHK)
//                engc->flgs |= ((item->uuid == MMI_IBGR)? WIN_IBGR : WIN_IPBO);
//            else
//                engc->flgs &= ~((item->uuid == MMI_IBGR)? WIN_IBGR : WIN_IPBO);
//            if (engc->rscm != SCM_ROGL)
//                Message((HWND)engc->user[0], (char*)engc->tran[TXT_UWGL],
//                         0, MB_OK | MB_ICONEXCLAMATION);
//            else
//                RestartEngine(engc, engc->rscm);
            break;
        }
    }
}



MENU *OSSpecificMenu(ENGC *engc) {
    char buff[1024];
    buff[countof(buff) - 1] = 0;
/** TODO **/
//    MENU tmpl[] =
//   {{.text = engc->tran[TXT_CONS], .uuid = MMI_CONS, .func = OSSpecific,
//     .flgs = MFL_CCHK | ((GetConsoleTitle(buff, countof(buff) - 1))?
//                          MFL_VCHK : 0)},
//    {.text = engc->tran[TXT_IRGN], .uuid = MMI_IRGN, .func = OSSpecific,
//     .flgs = MFL_CCHK | ((engc->flgs & WIN_IRGN)? MFL_VCHK : 0)
//                      | ((OldWin32())? MFL_GRAY : 0),
//     .data = (uintptr_t)engc},
//    {.text = engc->tran[TXT_IBGR], .uuid = MMI_IBGR, .func = OSSpecific,
//     .flgs = MFL_CCHK | ((engc->flgs & WIN_IBGR)? MFL_VCHK : 0),
//     .data = (uintptr_t)engc},
//    {.text = engc->tran[TXT_IPBO], .uuid = MMI_IPBO, .func = OSSpecific,
//     .flgs = MFL_CCHK | ((engc->flgs & WIN_IPBO)? MFL_VCHK : 0),
//     .data = (uintptr_t)engc},
//    {}};
//    return MenuFromTemplate(tmpl);
    return 0;
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
    HWND  iwnd = CreateWindowEx(0, "STATIC", 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
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



int APIENTRY WinMain(HINSTANCE inst, HINSTANCE prev, LPSTR cmdl, int show) {
/** TODO **/
//    INCBIN("../core/icon.gif", MainIcon);

    INITCOMMONCONTROLSEX icct = {sizeof(icct), ICC_STANDARD_CLASSES};
    NOTIFYICONDATAW nicd = {sizeof(nicd), 0, 1,
                            NIF_MESSAGE | NIF_ICON | NIF_TIP, WM_TRAY};
    HANDLE hdir;
    long flgs;

    RECT temp = {MAXLONG, MAXLONG, MINLONG, MINLONG};
    ENGC engc = {};

    InitCommonControlsEx(&icct);
    flgs = DialogBoxParam(inst, MAKEINTRESOURCE(DLG_MAIN), 0, EnterProc, 0);
    if (!flgs)
        return 0;

    if (flgs & FLG_CONS) {
        AllocConsole();
        freopen("CONOUT$", "wb", stdout);
    }
    if ((engc.engh = EngineInitialize())) {
        WIN32_FIND_DATAW dirw;
        WIN32_FIND_DATAA dira;
        hdir = FindFirstFileW(L""DEF_FLDR""DEF_DSEP"*", &dirw);
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
        EngineFinishLoading(engc.engh);

        /// [TODO] substitute this by GUI selection
        LINF *libs = engc.libs;
        while (libs) {
            libs->icnt = flgs & 0xFFFF;
            libs = (LINF*)libs->prev;
        }

        EnumDisplayMonitors(0, 0, CalcScreen, (LPARAM)&temp);


/** TODO **/
//        {
//            HBITMAP hdib, hmsk;
//            LONG x, y, xdim, ydim, xoff;
//            xdim = GetSystemMetrics(SM_CXSMICON);
//            ydim = GetSystemMetrics(SM_CYSMICON);
//            BYTE tran[ydim * (xoff = (xdim >> 3) + ((xdim & 7)? 1 : 0))];
//
//            /// the size is wrong, but let it be: MainIcon does have a GIF ending
//            ASTD *igif = MakeDataAnimStd(MainIcon, 1024 * 1024);
//            BGRA *clrs = ExtractRescaleSwizzleAlign(igif, 0xE4, 0, xdim, ydim);
//            FreeAnimStd(&igif);
//
//            for (y = 0; y < ydim; y++)
//                for (x = 0; x < xdim; x++)
//                    if (clrs[xdim * y + x].A)
//                        tran[xoff * y + (x >> 3)] &= ~(0x80 >> (x & 7));
//                    else
//                        tran[xoff * y + (x >> 3)] |=  (0x80 >> (x & 7));
//
//            hdib = CreateBitmap(xdim, ydim, 1, 32, clrs);
//            hmsk = CreateBitmap(xdim, ydim, 1, 1, tran);
//
//            ICONINFO icon = {FALSE, 0, 0, hmsk, hdib};
//            nicd.hIcon = CreateIconIndirect(&icon);
//            DeleteObject(hdib);
//            DeleteObject(hmsk);
//            free(clrs);
//        }
//        nicd.hWnd = hwnd;
//        char *hint = ConvertUTF8((char*)engc->tran[TXT_HEAD]);
        long oldw = OldWin32();
        if (oldw) {
//            strcpy((char*)nicd.szTip, hint);
            Shell_NotifyIconA(NIM_ADD, (NOTIFYICONDATAA*)&nicd);
        }
        else {
//            wcscpy(nicd.szTip, (LPWSTR)hint);
            Shell_NotifyIconW(NIM_ADD, &nicd);
        }
//        free(hint);


        ExecuteEngine(&engc, temp.left, temp.top,
                       temp.right - temp.left, temp.bottom - temp.top,
                      (flgs & FLG_EOGL)? SCM_ROGL : SCM_RSTD,
                     ((flgs & FLG_IBGR)? WIN_IBGR : 0) |
                     ((flgs & FLG_IPBO)? WIN_IPBO : 0) |
                     ((flgs & FLG_IRGN)? WIN_IRGN : 0) |
                     ((flgs & FLG_IOPQ)? COM_IOPQ : 0), 0);
        if (oldw)
            Shell_NotifyIconA(NIM_DELETE, (NOTIFYICONDATAA*)&nicd);
        else
            Shell_NotifyIconW(NIM_DELETE, &nicd);
        DestroyIcon(nicd.hIcon);
    }
    fclose(stdout);
    FreeConsole();
    return 0;
}
