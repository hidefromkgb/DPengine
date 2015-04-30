#define _WIN32_IE 0x0300
#define _WIN32_WINNT 0x0501

#include <time.h>
#include <windows.h>
#include <commctrl.h>

#include "rsrc/run.h"
#include "../exec/exec.h"



#define FLG_CONS (1 << 16)
#define FLG_EOGL (1 << 17)
#define FLG_IBGR (1 << 18)
#define FLG_IPBO (1 << 19)
#define FLG_IOPQ (1 << 20)
#define FLG_IRGN (1 << 21)



char *UTF8(LPWSTR wide) {
    long size = WideCharToMultiByte(CP_UTF8, 0, wide, -1, NULL, 0, NULL, NULL);
    char *retn = calloc(size * 2, sizeof(*retn));
    WideCharToMultiByte(CP_UTF8, 0, wide, -1, retn, size * 2, NULL, NULL);
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



BOOL APIENTRY EnterProc(HWND hWnd, UINT uMsg, WPARAM wPrm, LPARAM lPrm) {
    switch (uMsg) {
        case WM_INITDIALOG: {
            HWND temp = GetDlgItem(hWnd, MSC_INST);
            SendMessage(temp, UDM_SETRANGE, 0, MAKELPARAM(4096, 1));
            SendMessage(temp, UDM_SETPOS, 0, 128);
            SendMessage(GetDlgItem(hWnd, MCB_EOGL),
                        BM_SETCHECK, BST_CHECKED, 0);
            if ((GetVersion() & 0xFF) < 5) {
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
                    lPrm = ((GetVersion() & 0xFF) < 5)? FALSE : lPrm;
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



int APIENTRY WinMain(HINSTANCE inst, HINSTANCE prev, LPSTR cmdl, int show) {
    INITCOMMONCONTROLSEX icct = {sizeof(icct), ICC_STANDARD_CLASSES};
    HANDLE hdir;
    long flgs;

    ENGC engc = {};
    LINF *libs;

    InitCommonControlsEx(&icct);
    flgs = DialogBoxParam(inst, MAKEINTRESOURCE(DLG_MAIN), NULL, EnterProc, 0);
    if (!flgs)
        return 0;

    if (flgs & FLG_CONS) {
        AllocConsole();
        freopen("CONOUT$", "wb", stdout);
    }
    engc.dims.x = GetSystemMetrics(SM_CXVIRTUALSCREEN);
    engc.dims.y = GetSystemMetrics(SM_CYVIRTUALSCREEN);
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

        InitMainMenu(&engc);
        EngineFinishLoading(engc.engh);
        TTH_ITER(engc.libs, PrepareSpriteArr, &engc.libs);

        /// [TODO] substitute this by GUI selection
        libs = engc.libs;
        while (libs) {
            libs->icnt = flgs & 0xFFFF;
            libs = (LINF*)libs->prev;
        }
        engc.seed = time(0);
        printf("[((RNG))] seed = 0x%08X\n", engc.seed);
        MakeSpriteArr(&engc);
        EngineRunMainLoop(engc.engh, 0, 0, engc.dims.x, engc.dims.y,
                         ((flgs & FLG_IBGR)? WIN_IBGR : 0) |
                         ((flgs & FLG_IPBO)? WIN_IPBO : 0) |
                         ((flgs & FLG_IRGN)? WIN_IRGN : 0) |
                         ((flgs & FLG_IOPQ)? COM_IOPQ : 0), FRM_WAIT,
                          (flgs & FLG_EOGL)? SCM_ROGL : SCM_RSTD,
                           0, /// localization goes here
                          (uintptr_t)&engc, UpdateFrame);
        FreeEverything(&engc);
    }
    fclose(stdout);
    FreeConsole();
    return 0;
}
