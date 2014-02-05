#include "gifstd.h"



long InitAnimStd(GHDR *ghdr, void *anim, long cfrm) {
    ASTD *retn = (ASTD*)anim;
    long clrs, iter;
    RGBX *gpal;

    if (retn && ghdr && (ghdr->flgs & GIF_FPAL)) {
        retn->fcnt = cfrm;
        retn->xdim = ghdr->xdim;
        retn->ydim = ghdr->ydim;
        retn->time = malloc(cfrm * sizeof(*retn->time));

        if (!retn->bpal)
            retn->bptr.bgra = malloc(sizeof(*retn->bptr.bgra) *
                                     ghdr->xdim * ghdr->ydim  * cfrm);
        else {
            retn->bptr.indx = malloc(sizeof(*retn->bptr.indx) *
                                     ghdr->xdim * ghdr->ydim  * cfrm);
            clrs = 2 << (ghdr->flgs & 7);
            gpal = (RGBX*)((uint8_t*)ghdr + sizeof(*ghdr));
            for (iter = 0; iter < clrs; iter++) {
                retn->bpal[iter].B = gpal[iter].B;
                retn->bpal[iter].G = gpal[iter].G;
                retn->bpal[iter].R = gpal[iter].R;
                retn->bpal[iter].A = 0xFF;
            }
            retn->bpal[ghdr->bkgd].A = 0xFF;
            retn->bpal[0xFF].BGRA = 0x00000000;
        }
        return 1;
    }
    return -1;
}



long WriteFrameStd(GHDR *ghdr, FHDR *fhdr, void *anim, BGRA *bptr,
                   long tran, long time, long curr, long from) {
    ASTD *retn = (ASTD*)anim;
    long x, y, dsrc, ddst;

    retn->time[curr] = time * 10;

    /// TODO: PROPERLY HANDLE INTERLACING!!! (fhdr->flgs & GIF_FINT)

    if (!curr) {
        x = ghdr->xdim * ghdr->ydim;
        if (retn->bpal)
            for (y = 0; y < x; y++)
                retn->bptr.indx[y] = 0xFF;
        else
            for (y = 0; y < x; y++)
                retn->bptr.bgra[y].BGRA = 0;
    }
    ddst = ghdr->xdim * (ghdr->ydim * curr + fhdr->yoff) + fhdr->xoff;
    if (retn->bpal) {
        for (y = 0; y < fhdr->ydim; y++)
            for (x = 0; x < fhdr->xdim; x++)
                if (bptr[fhdr->xdim * y + x].A != tran)
                    retn->bptr.indx[ghdr->xdim * y + x + ddst] =
                               bptr[fhdr->xdim * y + x].A;
    }
    else {
        for (y = 0; y < fhdr->ydim; y++)
            for (x = 0; x < fhdr->xdim; x++)
                if (bptr[fhdr->xdim * y + x].A)
                    retn->bptr.bgra[ghdr->xdim * y + x + ddst] =
                               bptr[fhdr->xdim * y + x];
    }
    if (from >= 0) {
        x = ghdr->xdim * ghdr->ydim;
        dsrc = x * ((from)? (from - 1) : curr);
        ddst = x * (curr + 1);
        if (retn->bpal)
            for (y = 0; y < x; y++)
                retn->bptr.indx[ddst + y] = retn->bptr.indx[dsrc + y];
        else
            for (y = 0; y < x; y++)
                retn->bptr.bgra[ddst + y] = retn->bptr.bgra[dsrc + y];
    }
    if (!from) {
        ddst = ghdr->xdim * (ghdr->ydim * ++curr + fhdr->yoff) + fhdr->xoff;
        if (retn->bpal)
            for (y = 0; y < fhdr->ydim; y++)
                for (x = 0; x < fhdr->xdim; x++)
                    retn->bptr.indx[ghdr->xdim * y + x + ddst] = 0xFF;
        else
            for (y = 0; y < fhdr->ydim; y++)
                for (x = 0; x < fhdr->xdim; x++)
                    retn->bptr.bgra[ghdr->xdim * y + x + ddst].BGRA = 0;
    }
    return 1;
}



ASTD *MakeAnimStd(char *name, long indx) {
    ASTD *retn = malloc(sizeof(*retn));

    retn->bpal = (indx)? malloc(256 * sizeof(*retn->bpal)) : NULL;
    if (MakeAnim((void*)name, MAF_FILE | ((indx)? MAF_AIND : 0), (void*)retn,
                  NULL, InitAnimStd, WriteFrameStd, NULL) <= 0) {
        free(retn->bpal);
        free(retn);
        retn = NULL;
    }
    return retn;
}



void FreeAnimStd(ASTD **anim) {
    if (anim && *anim) {
        free((*anim)->bptr.bgra);
        free((*anim)->bpal);
        free((*anim)->time);
        free(*anim);
        *anim = NULL;
    }
}
