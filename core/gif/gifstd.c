#include "load/gif_load.h"
#include "gifstd.h"



static void WriteFrameStd(GIF_GHDR *ghdr, GIF_FHDR *curr, GIF_FHDR *prev,
                          GIF_RGBX *cpal, long clrs, uint8_t *bptr, void *data,
                          long nfrm, long tran, long time, long indx) {
    long x, y, yoff, iter, ifin, dsrc, ddst;
    ASTD *retn = (ASTD*)data;

    yoff = ghdr->xdim * ghdr->ydim;
    if (retn->fcnt < (x = labs(nfrm))) {
        retn->bptr = realloc(retn->bptr, x * yoff);
        retn->xdim = ghdr->xdim;
        retn->ydim = ghdr->ydim;
        retn->fcnt = x;
        retn->time = realloc(retn->time, x * sizeof(*retn->time));
    }
    retn->time[indx] = time * 10;
    if (!retn->bpal) {
        retn->bpal = malloc(256 * sizeof(*retn->bpal));
        for (clrs--; clrs >= 0; clrs--) {
            retn->bpal[clrs].chnl[0] = cpal[clrs].B;
            retn->bpal[clrs].chnl[1] = cpal[clrs].G;
            retn->bpal[clrs].chnl[2] = cpal[clrs].R;
            retn->bpal[clrs].chnl[3] = 0xFF;
        }
        retn->bpal[0xFF].bgra = 0x00000000;
    }
    iter = ghdr->xdim;
    ifin = ghdr->ydim;
    dsrc = indx * yoff;
    ddst = 0;
    /** background = some previous frame (maybe with a hole) **/
    if (prev) {
        ddst = ((uintptr_t)prev <= (uintptr_t)sizeof(prev))?
               yoff * (indx - (uintptr_t)prev) : yoff * (indx - 1);
        for (y = 0; y < yoff; y++)
            retn->bptr[dsrc + y] = retn->bptr[ddst + y];

        if ((uintptr_t)prev > (uintptr_t)sizeof(prev)) {
            ddst = dsrc + ghdr->xdim * prev->yoff + prev->xoff;
            iter = prev->xdim;
            ifin = prev->ydim;
            prev = 0;
        }
    }
    /** empty background or hole in the previous frame **/
    if (!prev)
        for (y = 0; y < ifin; y++)
            for (x = 0; x < iter; x++)
                retn->bptr[ghdr->xdim * y + x + ddst] = 0xFF;

    ddst = dsrc + ghdr->xdim * curr->yoff + curr->xoff;
    iter = (curr->flgs & GIF_FINT)? 0 : 4;
    ifin = (curr->flgs & GIF_FINT)? 4 : 5;

    /** [TODO:] the frame is assumed to be inside global bounds,
                however it might exceed them in some GIFs; fix me. **/
    for (dsrc = -1; iter < ifin; iter++)
        for (yoff = 16 >> ((iter > 1)? iter : 1), y = (8 >> iter) & 7;
             y < curr->ydim; y += yoff)
            for (x = 0; x < curr->xdim; x++)
                if (tran != (long)bptr[++dsrc])
                    retn->bptr[ghdr->xdim * y + x + ddst] = bptr[dsrc];
}



void FreeAnimStd(ASTD **anim) {
    if (anim && *anim) {
        free((*anim)->bptr);
        free((*anim)->bpal);
        free((*anim)->time);
        free(*anim);
        *anim = 0;
    }
}



ASTD *MakeAnimStd(char *data, long size) {
    ASTD *retn;

    if (!data || (size <= 0))
        return 0;
    if (!GIF_Load((void*)data, size, 0, WriteFrameStd,
                  (void*)(retn = calloc(1, sizeof(*retn)))))
        FreeAnimStd(&retn);
    return retn;
}
