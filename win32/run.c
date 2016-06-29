#define _WIN32_IE 0x0500
#define _WIN32_WINNT 0x0501
#define WINVER _WIN32_WINNT

#include <windows.h>
#include <commctrl.h>
#include <shlobj.h>

#include "../exec/exec.h"



#define WM_TRAY (WM_USER + 1000)



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
        CloseHandle(file);
        return flen;
    }
    return 0;
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
    return DefWindowProc(hWnd, uMsg, wPrm, lPrm);
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
    return DefWindowProc(hWnd, uMsg, wPrm, lPrm);
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
                ctrl->priv[2] = ctrl->fc2e(ctrl, MSG_SMAX, lPrm);
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
    return DefWindowProc(hWnd, uMsg, wPrm, lPrm);
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
    CTRL *ctrl = (CTRL*)GetWindowLongPtr(hWnd, GWLP_USERDATA);

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
            if (ctrl) {
                ((MINMAXINFO*)lPrm)->ptMinTrackSize.x =
                    (uint16_t)(ctrl->priv[3]);
                ((MINMAXINFO*)lPrm)->ptMinTrackSize.y =
                    (uint16_t)(ctrl->priv[3] >> 16);
            }
            return 0;

        case WM_CLOSE:
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;

        case WM_SIZE:
            ctrl->fc2e(ctrl, MSG_WSZC, lPrm);
            return 0;

        case WM_NOTIFY:
            if (((NMHDR*)lPrm)->code == UDN_DELTAPOS)
                ProcessSpin(lPrm);
            break;
    }
    return DefWindowProc(hWnd, uMsg, wPrm, lPrm);
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
//        case WM_ERASEBKGND:
//            return ~0;

        case WM_GETDLGCODE:
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
            lcur = rect.right * ctrl->priv[3]
                 / ((ctrl->priv[4] > 0)? ctrl->priv[4] : 1);
            devc = BeginPaint(hWnd, &pstr);
            retn = CallWindowProc((WNDPROC)ctrl->priv[7],
                                   hWnd, uMsg, (WPARAM)devc, lPrm);
            SetBkMode(devc, TRANSPARENT);
            SelectObject(devc, (HFONT)ctrl->priv[1]);
            for (uMsg = 0; uMsg <= 1; uMsg++) {
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

        case MSG_WSZC: {
            RECT rect;

            SystemParametersInfo(SPI_GETWORKAREA, 0, &rect, 0);

            rect.left += rect.right >> 1;
            rect.top += rect.bottom >> 1;

            rect.right  = (((uint16_t)(data)) + ctrl->xdim)
                        *   (uint16_t)(ctrl->priv[2])
                        +   (GetSystemMetrics(SM_CXFRAME) << 1);
            rect.bottom = (((uint16_t)(data >> 16)) + ctrl->ydim)
                        *   (uint16_t)(ctrl->priv[2] >> 16)
                        +   (GetSystemMetrics(SM_CYFRAME) << 1)
                        +    GetSystemMetrics(SM_CYCAPTION);

            rect.left -= rect.right >> 1;
            rect.top -= rect.bottom >> 1;

            ctrl->priv[3] =  (uint16_t)rect.right
                          | ((uint32_t)rect.bottom << 16);
            SetWindowPos((HWND)ctrl->priv[0], HWND_TOP, rect.left, rect.top,
                         rect.right, rect.bottom, SWP_SHOWWINDOW);
            break;
        }
    }
    return 0;
}



intptr_t FE2CP(CTRL *ctrl, uint32_t cmsg, intptr_t data) {
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
            break;

        case MSG_PTXT:
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

        case MSG_BGST:
            return ((IsWindowEnabled((HWND)ctrl->priv[0]))?
                     FCS_ENBL : 0)
                 | ((SendMessage((HWND)ctrl->priv[0],
                                 BM_GETCHECK, 0, 0) == BST_CHECKED)?
                     FCS_MARK : 0);

        case MSG_BCLK:
            cmsg = SendMessage((HWND)ctrl->priv[0], BM_GETCHECK, 0, 0);
            SendMessage((HWND)ctrl->priv[0], BM_SETCHECK,
                        (data)? BST_CHECKED : BST_UNCHECKED, 0);
            return cmsg == BST_CHECKED;
    }
    return 0;
}



intptr_t FE2CL(CTRL *ctrl, uint32_t cmsg, intptr_t data) {
    LRESULT APIENTRY (*SMSG)(HWND, UINT, WPARAM, LPARAM);

    switch (cmsg) {
        case MSG__ENB:
            EnableWindow((HWND)ctrl->priv[0], !!data);
            break;

        case MSG_LCOL: {
            LVCOLUMN vcol = {LVCF_TEXT, 0, 0, rConvertUTF8((char*)data)};

            if (OldWin32()) {
                SMSG = SendMessageA;
                cmsg = LVM_SETCOLUMNA;
            }
            else {
                SMSG = SendMessageW;
                cmsg = LVM_SETCOLUMNW;
            }
            SMSG((HWND)ctrl->priv[0], cmsg, 0, (LPARAM)&vcol);
            SMSG((HWND)ctrl->priv[0], LVM_REDRAWITEMS, 0, MAXWORD);
            InvalidateRect((HWND)ctrl->priv[0], 0, FALSE);
            free(vcol.pszText);
            break;
        }
        case MSG_LADD: {
            LVITEM item = {LVIF_TEXT, MAXWORD,
                          .pszText = rConvertUTF8((char*)data)};

            if (OldWin32()) {
                SMSG = SendMessageA;
                cmsg = LVM_INSERTITEMA;
            }
            else {
                SMSG = SendMessageW;
                cmsg = LVM_INSERTITEMW;
            }
            SMSG((HWND)ctrl->priv[0], cmsg, 0, (LPARAM)&item);
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
            ctrl->priv[2] = -(uint16_t)data;
            ctrl->priv[3] = (uint16_t)(data >> 16);
            SendMessage((HWND)ctrl->priv[1], UDM_SETRANGE32,
                        (WPARAM)ctrl->priv[2], (LPARAM)ctrl->priv[3]);
            SendMessage((HWND)ctrl->priv[1], UDM_SETPOS32, 0, 0);
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

        case FCT_RBOX:
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



#define WC_MAINWND "W"
#define WC_LISTROOT "L"
#define WC_SIZEBOX "S"
#define WC_IMGBOX "I"
void rMakeControl(CTRL *ctrl, long *xoff, long *yoff, char *text) {
    static struct {
        LPSTR name;
        DWORD wsty;
    } base[] =
       {{WC_MAINWND,     WS_CLIPCHILDREN},
        {WC_STATIC,      SS_CENTERIMAGE},
        {WC_BUTTON,      BS_FLAT | BS_MULTILINE},
        {WC_BUTTON,      BS_FLAT | BS_AUTOCHECKBOX},
        {WC_BUTTON,      BS_FLAT | BS_AUTORADIOBUTTON},
        {WC_EDIT,        ES_MULTILINE | ES_AUTOHSCROLL},
        {WC_LISTVIEW,    LVS_REPORT},
        {PROGRESS_CLASS, PBS_SMOOTH},
        {WC_SIZEBOX,     WS_CLIPCHILDREN | WS_VSCROLL},
        {WC_IMGBOX,      WS_CLIPCHILDREN},
    };
    WNDCLASSEX test, wndc = {sizeof(wndc), CS_HREDRAW | CS_VREDRAW,
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
        wndc.lpszClassName = WC_MAINWND;
        wndc.hbrBackground = (HBRUSH)COLOR_WINDOW;
        if (!GetClassInfoEx(wndc.hInstance, wndc.lpszClassName, &test))
            RegisterClassEx(&wndc);
        cwnd = CreateWindowEx(0, WC_MAINWND, 0,
                              WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
                              0, 0, 0, 0, 0, 0, wndc.hInstance, 0);
        SetWindowLongPtr(cwnd, GWLP_USERDATA, (LONG_PTR)ctrl);
        ctrl->fe2c = FE2CW;
    }
    else if (root) {
        xsty = 0;
        wsty = ctrl->flgs & FCT_TTTT;
        name = base[wsty].name;
        wsty = base[wsty].wsty | WS_CHILD | WS_VISIBLE
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
                break;

            case FCT_CBOX:
                wsty |= (ctrl->flgs & FSX_LEFT)? BS_LEFTTEXT : 0;
                ctrl->fe2c = FE2CX;
                break;

            case FCT_RBOX:
                wsty |= (ctrl->flgs & FSR_NGRP)? WS_GROUP : 0;
                break;

            case FCT_SPIN:
                xsty = WS_EX_CLIENTEDGE;
                ctrl->priv[4] = root->priv[4]; /// copying alignment
                ctrl->fe2c = FE2CN;
                break;

            case FCT_LIST:
                wndc.hCursor = LoadCursor(0, IDC_ARROW);
                wndc.lpfnWndProc = ListProc;
                wndc.lpszClassName = WC_LISTROOT;
                wndc.hbrBackground = (HBRUSH)COLOR_WINDOW;
                if (!GetClassInfoEx(wndc.hInstance, wndc.lpszClassName, &test))
                    RegisterClassEx(&wndc);
                cwnd = CreateWindowEx(0, WC_LISTROOT, 0, WS_CHILD | WS_VISIBLE,
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
                wndc.lpszClassName = WC_SIZEBOX;
                wndc.hbrBackground = (HBRUSH)COLOR_WINDOW;
                if (!GetClassInfoEx(wndc.hInstance, wndc.lpszClassName, &test))
                    RegisterClassEx(&wndc);
                ctrl->fe2c = FE2CS;
                break;

            case FCT_IBOX:
                wndc.hCursor = LoadCursor(0, IDC_ARROW);
                wndc.lpfnWndProc = IBoxProc;
                wndc.lpszClassName = WC_IMGBOX;
                wndc.hbrBackground = (HBRUSH)COLOR_MENU;
                if (!GetClassInfoEx(wndc.hInstance, wndc.lpszClassName, &test))
                    RegisterClassEx(&wndc);
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
    if (ctrl->priv[0] && text) {
        name = rConvertUTF8(text);
        if (OldWin32())
            SendMessageA((HWND)ctrl->priv[0], WM_SETTEXT, 0, (LPARAM)name);
        else
            SendMessageW((HWND)ctrl->priv[0], WM_SETTEXT, 0, (LPARAM)name);
        free(name);
    }
}



void rInternalMainLoop(CTRL *root, uint32_t fram, UPRE upre,
                       ENGC *engc, intptr_t data) {
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
        upre(engc, data, time);
    }
}



int APIENTRY WinMain(HINSTANCE inst, HINSTANCE prev, LPSTR cmdl, int show) {
    INITCOMMONCONTROLSEX icct = {sizeof(icct), ICC_STANDARD_CLASSES};
    RECT area = {MAXLONG, MAXLONG, MINLONG, MINLONG};
    CHAR *conf, path[4 * (MAX_PATH + 1)] = {};
    HRESULT APIENTRY (*GFP)(HWND, int, HANDLE, DWORD, LPSTR);
    LPWSTR wide = 0;
    HANDLE hdir;
    ENGC *engc;
    BOOL retn;

//    if (flgs & FLG_CONS) {
//        AllocConsole();
//        freopen("CONOUT$", "wb", stdout);
//    }

    retn = 0;
    if (OldWin32()) {
        hdir = LoadLibrary("shfolder");
        GFP = (typeof(GFP))GetProcAddress(hdir, "SHGetFolderPathA");
        if (GFP(NULL, CSIDL_APPDATA | CSIDL_FLAG_CREATE, NULL,
                SHGFP_TYPE_CURRENT, path) == S_OK) {
            strcat(path, DEF_OPTS);
            wide = UTF16(path);
            retn = CreateDirectoryA((LPSTR)path, 0);
            if (!retn && (GetLastError() == ERROR_ALREADY_EXISTS))
                retn = 1;
        }
    }
    else {
        hdir = LoadLibrary("shell32");
        GFP = (typeof(GFP))GetProcAddress(hdir, "SHGetFolderPathW");
        if (GFP(NULL, CSIDL_APPDATA | CSIDL_FLAG_CREATE, NULL,
                SHGFP_TYPE_CURRENT, path) == S_OK) {
            wcscat((LPWSTR)path, L""DEF_OPTS);
            wide = _wcsdup((LPWSTR)path);
            retn = CreateDirectoryW((LPWSTR)path, 0);
            if (!retn && (GetLastError() == ERROR_ALREADY_EXISTS))
                retn = 1;
        }
    }
    FreeLibrary(hdir);
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

    InitCommonControlsEx(&icct);
    EnumDisplayMonitors(0, 0, CalcScreen, (LPARAM)&area);
    eExecuteEngine(engc, GetSystemMetrics(SM_CXSMICON),
                   GetSystemMetrics(SM_CYSMICON),
                   area.left, area.top, area.right, area.bottom);
    fclose(stdout);
    FreeConsole();
    exit(0);
    return 0;
}
