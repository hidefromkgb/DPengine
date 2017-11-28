#include "load/gif_load.h"
#include "gifstd.h"
#include <string.h>



static void WriteFrameStd(void *data, GIF_WHDR *whdr) {
    long x, y, yoff, iter, ifin, dsrc, ddst;
    ASTD *retn = (ASTD*)data;

    ddst = (y = whdr->xdim * whdr->ydim) * whdr->ifrm
         + whdr->xdim * whdr->fryo + whdr->frxo;
    if (!retn->bpal) { /** the animation is empty, initializing **/
        retn->bpal = malloc(256 * sizeof(*retn->bpal));
        for (x = whdr->clrs - 1; x >= 0; x--) {
            retn->bpal[x].chnl[0] = whdr->cpal[x].B;
            retn->bpal[x].chnl[1] = whdr->cpal[x].G;
            retn->bpal[x].chnl[2] = whdr->cpal[x].R;
            retn->bpal[x].chnl[3] = 0xFF;
        }
        retn->bpal[0xFF].bgra = 0x00000000;
        retn->xdim = whdr->xdim;
        retn->ydim = whdr->ydim;
        retn->fcnt = (whdr->nfrm < 0)? -whdr->nfrm : whdr->nfrm;
        retn->time = malloc(retn->fcnt * sizeof(*retn->time));
        memset(retn->bptr = malloc(retn->fcnt * y), 0xFF, y);
    }
    retn->time[whdr->ifrm] = whdr->time * 10;
    ifin = (!(iter = (whdr->intr)? 0 : 4))? 4 : 5; /** interlacing support **/

    /** [TODO:] the frame is assumed to be inside global bounds,
                however it might exceed them in some GIFs; fix me. **/
    for (dsrc = -1; iter < ifin; iter++)
        for (yoff = 16 >> ((iter > 1)? iter : 1), y = (8 >> iter) & 7;
             y < whdr->fryd; y += yoff)
            for (x = 0; x < whdr->frxd; x++)
                if (whdr->tran != (long)whdr->bptr[++dsrc])
                    retn->bptr[whdr->xdim * y + x + ddst] = whdr->bptr[dsrc];
    if (whdr->ifrm + 1 < whdr->nfrm) { /** background for the next frame **/
        dsrc = (y = whdr->xdim * whdr->ydim) * (whdr->ifrm + 1);
        if ((whdr->mode != GIF_BKGD) || (whdr->frxo | whdr->fryo)
        ||  (whdr->frxd != whdr->xdim) || (whdr->fryd != whdr->ydim))
            memcpy(retn->bptr + dsrc, retn->bptr + dsrc
                - ((whdr->ifrm && (whdr->mode == GIF_PREV))? y + y : y), y);
        if (whdr->mode == GIF_BKGD) /** cutting a hole if need be **/
            for (ddst += whdr->xdim * whdr->ydim, y = 0; y < whdr->fryd; y++)
                for (x = 0; x < whdr->frxd; x++)
                    retn->bptr[whdr->xdim * y + x + ddst] = 0xFF;
    }
}



static void ReadMetadataStd(void *data, GIF_WHDR *whdr) {
    #define RMS_BGRA(i) do { uint32_t temp;                              \
            temp = retn->bpal[bptr[0]].bgra; retn->bpal[bptr[0]].bgra =  \
            ((((temp & 0x00FF00FF) * (1 + bptr[i])) >> 8) & 0x00FF00FF)| \
            ((((temp & 0xFF00FF00) >> 8) * (1 + bptr[i])) & 0xFF00FF00); \
            bptr += 1 + i; } while (0)
    ASTD *retn = (ASTD*)data;
    uint8_t *bptr;
    long iter;

    if ((whdr->bptr[0] != 'O') || (whdr->bptr[4] != 'i')
    ||  (whdr->bptr[1] != 'p') || (whdr->bptr[5] != 't')
    ||  (whdr->bptr[2] != 'a') || (whdr->bptr[6] != 'y')
    ||  (whdr->bptr[3] != 'c') || (whdr->bptr[7] != ':'))
        return;

    bptr = (uint8_t*)(whdr->bptr + 8 + 3);
    iter = *bptr++;
    while (!0) {
        for (; iter > 1; iter -= 2)
            RMS_BGRA(1);
        if (!iter && bptr[0]) {
            iter = *bptr++;
            continue;
        }
        else if (iter && bptr[1]) {
            iter = bptr[1] - 1;
            RMS_BGRA(2);
            continue;
        }
        break;
    }
    #undef RMS_BGRA
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
    if (!GIF_Load((void*)data, size, WriteFrameStd, ReadMetadataStd,
                  (void*)(retn = calloc(1, sizeof(*retn))), 0))
        FreeAnimStd(&retn);
    return retn;
}
