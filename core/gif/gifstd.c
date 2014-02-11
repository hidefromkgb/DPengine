#include "gifstd.h"



long InitAnimStd(GHDR *ghdr, void *anim, long cfrm) {
    ASTD *retn = (ASTD*)anim;

    if (retn && ghdr && (ghdr->flgs & GIF_FPAL)) {
        retn->fcnt = cfrm;
        retn->xdim = ghdr->xdim;
        retn->ydim = ghdr->ydim;
        retn->time = malloc(cfrm * sizeof(*retn->time));
        retn->bptr = malloc(cfrm * ghdr->xdim * ghdr->ydim);
        return 1;
    }
    return -1;
}



long WriteFrameStd(GHDR *ghdr, FHDR *fhdr, void *anim,
                   uint8_t *bptr, RGBX *cpal, long clrs,
                   long tran, long time, long curr, long from) {
    ASTD *retn = (ASTD*)anim;
    long x, y, dsrc, ddst;

    retn->time[curr] = time * 10;
    if (!retn->bpal) {
        retn->bpal = malloc(256 * sizeof(*retn->bpal));
        for (clrs--; clrs >= 0; clrs--) {
            retn->bpal[clrs].B = cpal[clrs].B;
            retn->bpal[clrs].G = cpal[clrs].G;
            retn->bpal[clrs].R = cpal[clrs].R;
            retn->bpal[clrs].A = 0xFF;
        }
        retn->bpal[0xFF].BGRA = 0x00000000;
    }

    /// TODO: PROPERLY HANDLE INTERLACING!!! (fhdr->flgs & GIF_FINT)

    if (!curr)
        for (y = ghdr->xdim * ghdr->ydim - 1; y >= 0; y--)
            retn->bptr[y] = 0xFF;

    ddst = ghdr->xdim * (ghdr->ydim * curr + fhdr->yoff) + fhdr->xoff;
    for (y = 0; y < fhdr->ydim; y++)
        for (x = 0; x < fhdr->xdim; x++)
            if (bptr[fhdr->xdim * y + x] != tran)
                retn->bptr[ghdr->xdim * y + x + ddst] =
                      bptr[fhdr->xdim * y + x];
    if (from >= 0) {
        x = ghdr->xdim * ghdr->ydim;
        dsrc = x * ((from)? (from - 1) : curr);
        ddst = x * (curr + 1);
        for (y = 0; y < x; y++)
            retn->bptr[ddst + y] = retn->bptr[dsrc + y];
    }
    if (!from) {
        ddst = ghdr->xdim * (ghdr->ydim * ++curr + fhdr->yoff) + fhdr->xoff;
        for (y = 0; y < fhdr->ydim; y++)
            for (x = 0; x < fhdr->xdim; x++)
                retn->bptr[ghdr->xdim * y + x + ddst] = 0xFF;
    }
    return 1;
}



ASTD *MakeAnimStd(char *name) {
    ASTD *retn;

    retn = malloc(sizeof(*retn));
    retn->bpal = NULL;
    if (MakeAnim((void*)name, MAF_FILE, (void*)retn,
                  NULL, InitAnimStd, WriteFrameStd, NULL) <= 0) {
        free(retn->bpal);
        free(retn);
        retn = NULL;
    }
    return retn;
}



void FreeAnimStd(ASTD **anim) {
    if (anim && *anim) {
        free((*anim)->bptr);
        free((*anim)->bpal);
        free((*anim)->time);
        free(*anim);
        *anim = NULL;
    }
}
