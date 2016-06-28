#include <errno.h>
#include <dirent.h>
#include <sys/stat.h>

#include "../exec/exec.h"
#include "mac.h"



void rOpenContextMenu(MENU *menu) {
    /// [TODO:]
}



inline MENU *rOSSpecificMenu(ENGC *engc) {
    return 0;
}



char *rConvertUTF8(char *utf8) {
    return strdup(utf8);
}



long rMessage(char *text, char *head, uint32_t flgs) {
    /// [TODO:]
    return 0;
}



intptr_t rMakeTrayIcon(MENU *mctx, char *text,
                       uint32_t *data, long xdim, long ydim) {
    id sbar = systemStatusBar(NSStatusBar);
    id icon = statusItemWithLength_(sbar, NSVariableStatusItemLength);

    setHighlightMode_(icon, true);
    /// [TODO:]
    return 0;
}



void rFreeTrayIcon(intptr_t icon) {
    /// [TODO:]
}



char *rLoadFile(char *name, long *size) {
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



long rSaveFile(char *name, char *data, long size) {
    long file;

    if ((file = open(name, O_CREAT | O_WRONLY, 0644)) > 0) {
        size = write(file, data, size);
        close(file);
        return size;
    }
    return 0;
}



void rInternalMainLoop(CTRL *root, uint32_t fram, UPRE upre,
                       ENGC *engc, intptr_t data) {
    /// [TODO:]
}



void rFreeControl(CTRL *ctrl) {
    /// [TODO:]
}



intptr_t FE2C(CTRL *ctrl, uint32_t cmsg, intptr_t data) {
    return 0;
}



void rMakeControl(CTRL *ctrl, long *xoff, long *yoff, char *text) {
    /// [TODO:]
    ctrl->fe2c = FE2C;
}



int main(int argc, char *argv[]) {
    ssize_t sdim;
    CGRect dims;

    char *home, *conf;
    struct dirent **dirs;
    ENGC *engc;

    /// very important call; without it, nothing below would ever work
    LoadObjC();

/*
    if (!(home = getenv("HOME")))
        home = getpwuid(getuid())->pw_dir;
    conf = calloc(32 + strlen(home), sizeof(*conf));
    strcat(conf, home);
    strcat(conf, "/.config");
    strcat(conf, DEF_OPTS);
    if (!(home = (mkdir(conf, 0755))? (errno != EEXIST)? 0 : conf : conf))
        printf("WARNING: cannot create '%s'!", conf);
//*/

//    NSArray *URLs = [[NSFileManager defaultManager] URLsForDirectory:NSDocumentDirectory inDomains:NSUserDomainMask];
//    NSURL *documentsURL = URLs[0];

    engc = eInitializeEngine(conf);
    free(conf);
    if ((sdim = scandir(DEF_FLDR, &dirs, 0, alphasort)) >= 0) {
        while (sdim--) {
            if ((dirs[sdim]->d_type == DT_DIR)
            &&  strcmp(dirs[sdim]->d_name, ".")
            &&  strcmp(dirs[sdim]->d_name, ".."))
                eAppendLib(engc, DEF_CONF, DEF_FLDR, dirs[sdim]->d_name);
            free(dirs[sdim]);
        }
        free(dirs);
    }
    GetT4DV(dims, mainScreen(NSScreen), VisibleFrame);
    GetT1DV(sdim, systemStatusBar(NSStatusBar), Thickness);
    eExecuteEngine(engc, sdim, sdim, 0, 0, dims.size.width  - dims.origin.x,
                                           dims.size.height - dims.origin.y);
    usleep(1000000000);
    return 0;
}
