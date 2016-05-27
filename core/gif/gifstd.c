#include "common.h"
#include "gifstd.h"



void WriteFrameStd(GHDR *ghdr, FHDR *fhdr, FHDR *back, RGBX *cpal,
                   long clrs, uint8_t *bptr, void *data, long nfrm,
                   long tran, long time, long indx) {
    long x, y, yoff, iter, ifin, dsrc, ddst;
    ASTD *retn = (ASTD*)data;

    yoff = ghdr->xdim * ghdr->ydim;
    if (retn->fcnt < (x = labs(nfrm)))
        *retn = (ASTD){realloc(retn->bptr, x * yoff), ghdr->xdim, ghdr->ydim,
                       x, realloc(retn->time, x * sizeof(*retn->time)),
                       retn->bpal};
    retn->time[indx] = time * 10;
    if (!retn->bpal) {
        retn->bpal = malloc(256 * sizeof(*retn->bpal));
        for (clrs--; clrs >= 0; clrs--)
            retn->bpal[clrs] =
                (BGRA){{cpal[clrs].B, cpal[clrs].G, cpal[clrs].R, 0xFF}};
        retn->bpal[0xFF].BGRA = 0x00000000;
    }
    iter = ghdr->xdim;
    ifin = ghdr->ydim;
    dsrc = indx * yoff;
    ddst = 0;
    /// background = some previous frame (maybe with a hole)
    if (back) {
        ddst = ((uintptr_t)back < (uintptr_t)sizeof(back))?
               yoff * (indx - (uintptr_t)back) : yoff * (indx - 1);
        for (y = 0; y < yoff; y++)
            retn->bptr[dsrc + y] = retn->bptr[ddst + y];

        if ((uintptr_t)back >= (uintptr_t)sizeof(back)) {
            ddst = dsrc + ghdr->xdim * back->yoff + back->xoff;
            iter = back->xdim;
            ifin = back->ydim;
            back = 0;
        }
    }
    /// empty background or hole in the previous frame
    if (!back)
        for (y = 0; y < ifin; y++)
            for (x = 0; x < iter; x++)
                retn->bptr[ghdr->xdim * y + x + ddst] = 0xFF;

    ddst = dsrc + ghdr->xdim * fhdr->yoff + fhdr->xoff;
    iter = (fhdr->flgs & GIF_FINT)? 0 : 4;
    ifin = (fhdr->flgs & GIF_FINT)? 4 : 5;

    /// [TODO:] the frame is assumed to be inside global bounds,
    ///         however it might exceed them in some GIFs; fix me.
    for (dsrc = -1; iter < ifin; iter++)
        for (yoff = 16 >> ((iter > 1)? iter : 1), y = (8 >> iter) & 7;
             y < fhdr->ydim; y += yoff)
            for (x = 0; x < fhdr->xdim; x++)
                if (tran != (typeof(tran))bptr[++dsrc])
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
    if (!MakeAnim((void*)data, size, 0, WriteFrameStd,
                  (void*)(retn = calloc(1, sizeof(*retn)))))
        FreeAnimStd(&retn);
    return retn;
}
