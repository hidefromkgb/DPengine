#include <time.h>
#include <unistd.h>
#include <dirent.h>
#include <X11/Xlib.h>
#include "../exec/exec.h"



char *LoadFileZ(char *name, long *size) {
    char *retn = 0;
    long file, flen;

    if ((file = open(name, O_RDONLY)) > 0) {
        flen = lseek(file, 0, SEEK_END);
        lseek(file, 0, SEEK_SET);
        retn = malloc(flen + 1);
        read(file, retn, flen);
        retn[flen] = '\0';
        close(file);
        if (size)
            *size = flen;
    }
    return retn;
}



int main(int argc, char *argv[]) {
    struct dirent **dirs;
    long uses, size;
    ENGC engc = {};
    LINF *libs;

    Display *disp = XOpenDisplay(0);
    if (!disp) {
        printf("Failed to init X Window System!\n");
        return -1;
    }
    Screen *pscr = DefaultScreenOfDisplay(disp);
    engc.dims.x = pscr->width;
    engc.dims.y = pscr->height;
    XCloseDisplay(disp);

    uses = (argc >  1)? atol(argv[1]) : 0;
    uses = (uses != 0)? uses : 1;

    if ((engc.engh = EngineInitialize())) {
        if ((size = scandir(DEF_FLDR, &dirs, 0, alphasort)) >= 0) {
            while (size--) {
                if ((dirs[size]->d_type == DT_DIR)
                &&  strcmp(dirs[size]->d_name, ".")
                &&  strcmp(dirs[size]->d_name, "..")) {
                    AppendLib(&engc, DEF_CONF, DEF_FLDR, dirs[size]->d_name);
                }
                free(dirs[size]);
            }
            free(dirs);
        }
        InitMainMenu(&engc);
        EngineFinishLoading(engc.engh);
        ListIterateTailToHead((LHDR*)engc.libs,
                              (ITER)FreeEmptySprite, (uintptr_t)&engc.libs);

        /// [TODO] substitute this by GUI selection
        libs = engc.libs;
        while (libs) {
            libs->icnt = abs(uses);
            libs = (LINF*)libs->prev;
        }
        engc.seed = time(0);
        printf("[((RNG))] seed = 0x%08X\n", engc.seed);
        EngineRunMainLoop(engc.engh, engc.dims.x, engc.dims.y, 0, FRM_WAIT,
                         (uses < 0)? SCM_RSTD : SCM_ROGL, (uintptr_t)&engc,
                          0, MakeSpriteArr(&engc), UpdateFrame);
        FreeEverything(&engc);
    }
    return 0;
}
