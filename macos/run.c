#include <time.h>
#include <unistd.h>
#include <dirent.h>
#include "../exec/exec.h"

#include <objc/objc-runtime.h>
#include <ApplicationServices/ApplicationServices.h>



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

    SEL fram = sel_registerName("visibleFrame");
    id scrn = objc_msgSend((id)objc_getClass("NSScreen"),
                            sel_registerName("mainScreen"));
    CGRect dims;

#ifdef __i386__
    typeof(dims) (*GetT4DV)(id, SEL) = (typeof(GetT4DV))objc_msgSend_stret;
    dims = GetT4DV(scrn, fram);
#else
    objc_msgSend_stret((void*)&dims, (void*)scrn, fram);
#endif
    engc.dims = (T2IV){dims.size.width  - dims.origin.x,
                       dims.size.height - dims.origin.y};

    uses = (argc > 1)? atol(argv[1]) : 0;
    uses = (uses > 0)? uses : 1;

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
        TTH_ITER(engc.libs, PrepareSpriteArr, &engc.libs);

        /// [TODO] substitute this by GUI selection
        libs = engc.libs;
        while (libs) {
            libs->icnt = labs(uses);
            libs = (LINF*)libs->prev;
        }
        engc.seed = time(0);
        printf("[((RNG))] seed = 0x%08X\n", engc.seed);
        MakeSpriteArr(&engc);
        EngineRunMainLoop(engc.engh, 0, 0, engc.dims.x, engc.dims.y, 0,
                          FRM_WAIT, (uses < 0)? SCM_RSTD : SCM_ROGL,
                          0, /// localization goes here
                         (uintptr_t)&engc, UpdateFrame);
        FreeEverything(&engc);
    }
    return 0;
}
