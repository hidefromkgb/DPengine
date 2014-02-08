#define _WIN32_WINNT 0x0501
#define _WIN32_IE 0x0300
#define WINVER 0x0410

#include <windows.h>
#include <mmsystem.h>
#include <commctrl.h>

#include "rsrc/rsrc.h"
#include "../core/core.h"



typedef struct _TMRD {
    HANDLE quit;
    ulong time;
} TMRD;

typedef struct _THRD {
    union {
        FILL fill;
        DRAW draw;
    } fprm;
    long loop;
    HANDLE evti, *evto;
    void (*func)(void*);
} THRD;



UNIT *pick = NULL;
VEC2 cptr = {};
UINT fcnt = 0;



DWORD CALLBACK TimeFunc(LPVOID data) {
    #define data ((TMRD*)data)
    while (!data->quit) {
        data->time = timeGetTime();
        Sleep(MIN_WAIT);
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
            data->func(&data->fprm);
        SetEvent(*data->evto);
    } while (data->loop);
    return TRUE;
    #undef data
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
                cptr.x = (short)(lPrm);
                cptr.y = (short)(lPrm >> 16);
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
    INITCOMMONCONTROLSEX icct = {sizeof(icct), ICC_STANDARD_CLASSES};
    BLENDFUNCTION bfun = {AC_SRC_OVER, 0, 255, AC_SRC_ALPHA};
    BITMAPINFO bmpi = {{sizeof(bmpi.bmiHeader), 0, 0,
                        1, 8 * sizeof(BGRA), BI_RGB}};
    SYSTEM_INFO syin = {};
    SYSTEMTIME init = {};
    POINT zpos = {};
    MSG pmsg = {};

    TMRD  tmrd = {};
    THRD *thrd;
    ULIB *ulib;
    UNIT *tail, *iter;

    char *temp;
    WIN32_FIND_DATAW fdir;
    HANDLE hdir, hwnd, *evto;
    UINT time, indx, nlim, ncpu;
    RECT scrr;

    HBITMAP hdib;
    PICT draw;
    HDC devc;

    InitCommonControlsEx(&icct);

    AllocConsole();
    freopen("CONOUT$", "wb", stdout);

    GetLocalTime(&init);
    seed = (init.wMilliseconds ^ init.wHour)
         + (init.wSecond ^ init.wMinute) * 0x10000;
    draw.size.x = GetSystemMetrics(SM_CXVIRTUALSCREEN);
    draw.size.y = GetSystemMetrics(SM_CYVIRTUALSCREEN);
    scrr = (RECT){0, 0, draw.size.x, draw.size.y};

    time = timeGetTime();
    GetSystemInfo(&syin);
    ncpu = min(MAXIMUM_WAIT_OBJECTS, max(1, syin.dwNumberOfProcessors));
    evto = LocalAlloc(LMEM_FIXED, ncpu * sizeof(*evto));
    thrd = LocalAlloc(LMEM_FIXED, ncpu * sizeof(*thrd));
    for (indx = 0; indx < ncpu; indx++) {
        evto[indx] = CreateEvent(NULL, FALSE, TRUE, NULL);
        thrd[indx] = (THRD){{(FILL){NULL, draw.size, 0}}, TRUE,
                            CreateEvent(NULL, FALSE, FALSE, NULL),
                            &evto[indx], (void (*)(void*))FillLibStdThrd};
        CreateThread(NULL, 0, ThrdFunc, &thrd[indx], 0, NULL);
    }
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
            thrd[indx].fprm.fill.ulib = ulib;
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
        nlim += thrd[indx].fprm.fill.load;
    }
    if (nlim) {
        time = timeGetTime() - time;
        printf("\nLoading complete: %u objects, %u ms [%0.3f ms/obj]\n\n",
               nlim, time, (float)time / (float)nlim);

        UnitListFromLib(ulib, &tail);
        for (indx = 0; indx < ncpu; indx++) {
            thrd[indx].loop = TRUE;
            thrd[indx].func = (void (*)(void*))DrawPixStdThrd;
            thrd[indx].fprm.draw = (DRAW){NULL, &draw,
                                   ((draw.size.y + 1) / ncpu)* indx,
                                   ((draw.size.y + 1) / ncpu)*(indx + 1)};
            CreateThread(NULL, 0, ThrdFunc, &thrd[indx], 0, NULL);
        }
        thrd[ncpu - 1].fprm.draw.ymax = draw.size.y;

        CreateThread(NULL, 0, TimeFunc, &tmrd, 0, NULL);
        devc = CreateCompatibleDC(NULL);
        bmpi.bmiHeader.biWidth  =  draw.size.x;
        bmpi.bmiHeader.biHeight = -draw.size.y;
        hdib = CreateDIBSection(devc, &bmpi, DIB_RGB_COLORS,
                               (void*)&draw.bptr, 0, 0);
        SelectObject(devc, hdib);

        RegisterClassEx(&wndc);
        hwnd = CreateWindowEx(WS_EX_LAYERED | WS_EX_TOPMOST,
                              wndc.lpszClassName, NULL, WS_POPUP | WS_VISIBLE,
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
                if ((iter = UpdateFrameStd(&tail, &pick, &tmrd.time, cptr))) {
                    FillRect(devc, &scrr, GetStockObject(BLACK_BRUSH));
                    for (indx = 0; indx < ncpu; indx++) {
                        thrd[indx].fprm.draw.tail = iter;
                        ResetEvent(*thrd[indx].evto);
                        SetEvent(thrd[indx].evti);
                    }
                    WaitForMultipleObjects(ncpu, evto, TRUE, INFINITE);

                    UpdateLayeredWindow(hwnd, NULL, &zpos, (SIZE*)&draw.size,
                                        devc, &zpos, 0, &bfun, ULW_ALPHA);
                }
                fcnt++;
                while (timeGetTime() - time < FRM_WAIT)
                    Sleep(MIN_WAIT);
            }
        }
        DeleteDC(devc);
        DeleteObject(hdib);
        tmrd.quit = CreateEvent(NULL, FALSE, FALSE, NULL);
        WaitForSingleObject(tmrd.quit, INFINITE);
        CloseHandle(tmrd.quit);

        for (indx = 0; indx < ncpu; indx++) {
            thrd[indx].loop = FALSE;
            SetEvent(thrd[indx].evti);
        }
        WaitForMultipleObjects(ncpu, evto, TRUE, INFINITE);

        FreeLibList(&ulib, (void (*)(void**))FreeAnimStd);
        FreeUnitList(&tail, NULL);
    }
    else
        printf("No animation base found! Exiting...\n");
    for (indx = 0; indx < ncpu; indx++) {
        CloseHandle(*thrd[indx].evto);
        CloseHandle(thrd[indx].evti);
    }
    LocalFree(evto);
    LocalFree(thrd);
    fclose(stdout);
    FreeConsole();
    return 0;
}
