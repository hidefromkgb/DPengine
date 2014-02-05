#define _WIN32_WINNT 0x0501
#define _WIN32_IE 0x0300
#define WINVER 0x0410

#include <windows.h>
#include <mmsystem.h>
#include <commctrl.h>

#include "rsrc/rsrc.h"
#include "../core/core.h"

#define FRM_SKIP 40
#define DEF_SKIP 1



typedef struct _TTMR {
    HANDLE quit;
    ulong time;
} TTMR;

typedef struct _THRD {
    union {
        FILL fill;
        DRAW draw;
    } pass;
    long loop;
    HANDLE evti, *evto;
    void (*func)(void*);
} THRD;



UNIT *pick = NULL;
VEC2 cptr = {};
UINT fcnt = 0;



DWORD CALLBACK TimeFunc(LPVOID data) {
    #define data ((TTMR*)data)
    while (!data->quit) {
        data->time = timeGetTime();
        Sleep(DEF_SKIP);
    }
    SetEvent(data->quit);
    return TRUE;
    #undef data
}



DWORD CALLBACK ThrdFunc(LPVOID data) {
    #define data ((THRD*)data)
    do {
        WaitForSingleObject(data->evti, INFINITE);
        ResetEvent(data->evti);
        if (data->loop)
            data->func(&data->pass);
        SetEvent(*data->evto);
    } while (data->loop);
    return TRUE;
    #undef data
}



BOOL CALLBACK EnterProc(HWND hWnd, UINT uMsg, WPARAM wPrm, LPARAM lPrm) {
    switch (uMsg) {
        case WM_INITDIALOG:
            hWnd = GetDlgItem(hWnd, MSC_LIMC);
            SendMessage(hWnd, UDM_SETRANGE, 0, MAKELPARAM(8192, 0));
            SendMessage(hWnd, UDM_SETPOS, 0, 0);
            return TRUE;


        case WM_COMMAND:
            if (LOWORD(wPrm) == IDOK)
                EndDialog(hWnd, SendMessage(GetDlgItem(hWnd, MSC_LIMC),
                                            UDM_GETPOS, 0, 0));
            else if (LOWORD(wPrm) == IDCANCEL)

        case WM_CLOSE:
                EndDialog(hWnd, -1);

        default:
            return FALSE;
    }
}



LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wPrm, LPARAM lPrm) {
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
                cptr.x = LOWORD(lPrm);
                cptr.y = HIWORD(lPrm);
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



int CALLBACK WinMain(HINSTANCE inst, HINSTANCE prev, LPSTR cmdl, int show) {
    WNDCLASSEX wndc = {sizeof(wndc), CS_HREDRAW | CS_VREDRAW, WindowProc,
                       0, 0, inst, LoadIcon(inst, MAKEINTRESOURCE(ICN_MAIN)),
                       LoadCursor(NULL, IDC_HAND), NULL, NULL, "-", NULL};
    INITCOMMONCONTROLSEX icct = {sizeof(icct),
                                 ICC_STANDARD_CLASSES | ICC_UPDOWN_CLASS};
    BLENDFUNCTION bfun = {AC_SRC_OVER, 0, 255, AC_SRC_ALPHA};
    BITMAPINFO bmpi = {{sizeof(bmpi.bmiHeader), 0, 0,
                        1, 8 * sizeof(BGRA), BI_RGB}};
    SYSTEM_INFO syin = {};
    SYSTEMTIME init = {};
    POINT zpos = {};
    TTMR ttmr = {};
    MSG pmsg = {};

    UNIT *tail, *iter;
    ULIB *ulib;
    THRD *thrd;

    char *temp;
    WIN32_FIND_DATAW fdir;
    HANDLE hdir, hwnd, *evto;
    UINT time, indx, nlim, ncpu;
    RECT scrr;

    HBITMAP hdib;
    PICT draw;
    HDC devc;

    InitCommonControlsEx(&icct);
    nlim = DialogBoxParam(inst, MAKEINTRESOURCE(DLG_MAIN), NULL, EnterProc, 0);
    if (++nlim) {
        AllocConsole();
//        freopen("w.txt", "wb", stdout);
        freopen("CONOUT$", "wb", stdout);

        GetLocalTime(&init);
        seed = (init.wMilliseconds ^ init.wHour)
             + (init.wSecond ^ init.wMinute) * 0x10000;
        draw.size.x = GetSystemMetrics(SM_CXVIRTUALSCREEN);
        draw.size.y = GetSystemMetrics(SM_CYVIRTUALSCREEN) - 1;

        time = timeGetTime();
        GetSystemInfo(&syin);
        ncpu = min(MAXIMUM_WAIT_OBJECTS, max(1, syin.dwNumberOfProcessors));
        evto = LocalAlloc(LMEM_FIXED, ncpu * sizeof(*evto));
        thrd = LocalAlloc(LMEM_FIXED, ncpu * sizeof(*thrd));
        for (indx = 0; indx < ncpu; indx++) {
            evto[indx] = CreateEvent(NULL, FALSE, TRUE, NULL);
            thrd[indx].evto = &evto[indx];
            thrd[indx].evti = CreateEvent(NULL, FALSE, FALSE, NULL);
            thrd[indx].loop = TRUE;
            thrd[indx].func = FillLibStdThrd;
            thrd[indx].pass.fill.scrn = draw.size;
            thrd[indx].pass.fill.load = 0;
            CreateThread(NULL, 0, ThrdFunc, &thrd[indx], 0, NULL);
        }
        indx = 0;
        if (!--nlim)
            nlim = (UINT)-1;

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
                thrd[indx].pass.fill.ulib = ulib;
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
            nlim += thrd[indx].pass.fill.load;
        }
        if (nlim) {
            time = timeGetTime() - time;
            printf("\nLoading complete: %u objects, %u ms [%0.3f ms/obj]\n\n",
                   nlim, time, (float)time / (float)nlim);

            UnitListFromLib(ulib, &tail);
            for (indx = 0; indx < ncpu; indx++) {
                thrd[indx].loop = TRUE;
                thrd[indx].func = DrawPixStdThrd;
                thrd[indx].pass.draw = (DRAW){NULL, &draw,
                                       ((draw.size.y + 1) / ncpu)* indx,
                                       ((draw.size.y + 1) / ncpu)*(indx + 1)};
                CreateThread(NULL, 0, ThrdFunc, &thrd[indx], 0, NULL);
            }
            thrd[ncpu - 1].pass.draw.ymax = draw.size.y;

            CreateThread(NULL, 0, TimeFunc, &ttmr, 0, NULL);
            devc = CreateCompatibleDC(NULL);
            bmpi.bmiHeader.biWidth  =  draw.size.x;
            bmpi.bmiHeader.biHeight = -draw.size.y;
            hdib = CreateDIBSection(devc, &bmpi, DIB_RGB_COLORS,
                                   (void*)&draw.bptr, 0, 0);
            SelectObject(devc, hdib);

            RegisterClassEx(&wndc);
            hwnd = CreateWindowEx(WS_EX_LAYERED | WS_EX_TOPMOST,
                                  wndc.lpszClassName, NULL,
                                  WS_POPUP | WS_VISIBLE,
                                  0, 0, draw.size.x, draw.size.y,
                                  NULL, NULL, wndc.hInstance, wndc.hIcon);
            while (!0) {
                time = timeGetTime();
                if (PeekMessage(&pmsg, 0, 0, 0, PM_REMOVE)) {
                    if (pmsg.message == WM_QUIT)
                        break;
                    TranslateMessage(&pmsg);
                    DispatchMessage(&pmsg);
                }
                else {
                    /// global vars used: PICK, CPTR, FCNT
                    if ((iter = UpdateFrameStd(&tail, &pick,
                                               &ttmr.time, cptr))) {
                        scrr = (RECT){0, 0, draw.size.x, draw.size.y};
                        FillRect(devc, &scrr, GetStockObject(BLACK_BRUSH));
                        for (indx = 0; indx < ncpu; indx++) {
                            thrd[indx].pass.draw.tail = iter;
                            SetEvent(thrd[indx].evti);
                        }
                        WaitForMultipleObjects(ncpu, evto, TRUE, INFINITE);
                        for (indx = 0; indx < ncpu; indx++)
                            ResetEvent(*thrd[indx].evto);

                        UpdateLayeredWindow(hwnd, NULL, &zpos,
                                           (SIZE*)&draw.size, devc,
                                           &zpos, 0, &bfun, ULW_ALPHA);
                    }
                    fcnt++;
                    while (timeGetTime() - time < FRM_SKIP)
                        Sleep(DEF_SKIP);
                }
            }
            DeleteDC(devc);
            DeleteObject(hdib);
            ttmr.quit = CreateEvent(NULL, FALSE, FALSE, NULL);
            WaitForSingleObject(ttmr.quit, INFINITE);
            CloseHandle(ttmr.quit);

            for (indx = 0; indx < ncpu; indx++) {
                thrd[indx].loop = FALSE;
                SetEvent(thrd[indx].evti);
            }
            WaitForMultipleObjects(ncpu, evto, TRUE, INFINITE);

            FreeLibList(&ulib, FreeAnimStd);
            FreeUnitList(&tail, NULL);
        }
        for (indx = 0; indx < ncpu; indx++) {
            CloseHandle(*thrd[indx].evto);
            CloseHandle(thrd[indx].evti);
        }
        LocalFree(evto);
        LocalFree(thrd);
        fclose(stdout);
        FreeConsole();
    }
    return 0;
}
