#include <dirent.h>
#include "../../exec/exec.h"



int main(int argc, char *argv[]) {
    struct dirent **dirs;
    long uses, rndr, iter;
    uint32_t xdim, ydim;
    ULIB *ulib;

    if (argc == 1)
        uses = 0;
    else
        uses = atol(argv[1]);
    if (!uses)
        uses = 128;
    rndr = (uses < 0)? BRT_RSTD : BRT_ROGL;
    uses = abs(uses);

    ulib = 0;
    xdim = ydim = 0;
    if (EngineInitialize(rndr, &xdim, &ydim, 0)) {                              /// Initialize
        if ((iter = scandir(DEF_FLDR, &dirs, 0, alphasort)) >= 0) {
            while (iter--) {
                if ((dirs[iter]->d_type == DT_DIR)
                &&  strcmp(dirs[iter]->d_name, ".")
                &&  strcmp(dirs[iter]->d_name, "..")) {
                    MakeEmptyLib(&ulib, DEF_FLDR, dirs[iter]->d_name);
                    FillLib(ulib, DEF_CONF, EngineLoadAnimAsync);               /// LoadAnimAsync
                }
                free(dirs[iter]);
            }
            free(dirs);
        }
        EngineFinishLoading();                                                  /// FinishLoading
        iter = UnitListFromLib(ulib, uses, xdim, ydim);
        EngineRunMainLoop(UpdateFrame, FRM_WAIT, iter);                         /// RunMainLoop
        FreeEverything(&ulib);
    }
    return 0;
}
