#define _WIN32_WINNT 0x0501
#define _WIN32_IE 0x0300

#include <windows.h>
#include <commctrl.h>

#include "rsrc/rsrc.h"
#include "../../exec/exec.h"



#define FLG_IBGR 0x10000
#define FLG_IPBO 0x20000
#define FLG_CONS 0x40000
#define FLG_EOGL 0x80000



char *UTF8(LPWSTR wide) {
    long size = WideCharToMultiByte(CP_UTF8, 0, wide, -1, NULL, 0, NULL, NULL);
    char *retn = calloc(size * 2, sizeof(*retn));
    WideCharToMultiByte(CP_UTF8, 0, wide, -1, retn, size * 2, NULL, NULL);
    return retn;
}



LPWSTR UTF16(char *utf8) {
    long size = strlen(utf8) * 4 + 2;
    LPWSTR retn = calloc(size, sizeof(*retn));
    MultiByteToWideChar(CP_UTF8, 0, utf8, -1, retn, size);
    return retn;
}



char *LoadFile(char *name, long *size) {
    char *retn = 0;
    DWORD  temp, flen;
    LPWSTR wide = UTF16(name);
    HANDLE file = CreateFileW(wide, GENERIC_READ, FILE_SHARE_READ, 0,
                              OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    if (file != INVALID_HANDLE_VALUE) {
        flen = GetFileSize(file, NULL);
        retn = malloc(flen + 1);
        ReadFile(file, retn, flen, &temp, NULL);
        CloseHandle(file);
        retn[flen] = '\0';
        if (size)
            *size = flen;
    }
    free(wide);
    return retn;
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
    INITCOMMONCONTROLSEX icct = {sizeof(icct), ICC_STANDARD_CLASSES};

    WIN32_FIND_DATAW fdir;
    HANDLE hdir;
    ULIB *ulib;

    uint32_t xdim, ydim;
    long flgs, rndr;
    char *temp;

    InitCommonControlsEx(&icct);
    flgs = DialogBoxParam(inst, MAKEINTRESOURCE(DLG_MAIN), NULL, EnterProc, 0);
    if (!flgs)
        return 0;

    rndr = (flgs & FLG_EOGL)? BRT_ROGL : BRT_RSTD;
    if (flgs & FLG_CONS) {
        AllocConsole();
        freopen("CONOUT$", "wb", stdout);
    }
    ulib = NULL;
    xdim = ydim = 0;
    if (EngineInitialize(rndr, &xdim, &ydim,                                    /// EngineInitialize
                        ((flgs & FLG_IBGR)? WIN_IBGR : 0)
                      | ((flgs & FLG_IPBO)? WIN_IPBO : 0))) {
        hdir = FindFirstFileW(L""DEF_FLDR""DEF_DSEP"*", &fdir);
        while ((hdir != INVALID_HANDLE_VALUE) &&
               (GetLastError() != ERROR_NO_MORE_FILES)) {
            MakeEmptyLib(&ulib, DEF_FLDR, temp = UTF8(fdir.cFileName));
            FillLib(ulib, DEF_CONF, EngineLoadAnimAsync);                       /// EngineLoadAnimAsync
            free(temp);
            FindNextFileW(hdir, &fdir);
        }
        FindClose(hdir);

        EngineFinishLoading(0);                                                 /// EngineFinishLoading
        flgs = UnitListFromLib(ulib, flgs & 0xFFFF, xdim, ydim);
        EngineRunMainLoop(UpdateFrame, FRM_WAIT, flgs);                         /// EngineRunMainLoop
        FreeEverything(&ulib);
    }
    fclose(stdout);
    FreeConsole();
    return 0;
}
