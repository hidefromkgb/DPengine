#include "core.h"
#include <string.h>

#define max(a, b) (((a) > (b))? (a) : (b))
#define min(a, b) (((a) < (b))? (a) : (b))



uint32_t seed;



uint32_t PRNG(uint32_t *seed) {
    return *seed = 1 + (*seed * 279470273) % 4294967291u;
}



INST *SortByY(INST **tail) {
    int ymin, ymax, ytmp;
    INST *curr, *retn;

    static struct {
        INST *lbgn, *lend;
    } elem[0x2000];

    retn = curr = *tail;
    if (curr->prev) {
        ymin = ymax = curr->cpos.y;
        while (!0) {
            ytmp = curr->cpos.y;
            if (ytmp > ymax)
                ymax = ytmp;
            else if (ytmp < ymin)
                ymin = ytmp;
            if (!curr->prev)
                break;
            curr = curr->prev;
        }
        memset(elem, 0, (ymax -= ymin - 1) * sizeof(*elem));

        while (curr) {
            ytmp = curr->cpos.y - ymin;
            if (!elem[ytmp].lbgn)
                elem[ytmp].lbgn = elem[ytmp].lend = curr;
            else {
                curr->prev = elem[ytmp].lend;
                elem[ytmp].lend->next = curr;
                elem[ytmp].lend = curr;
            }
            curr = curr->next;
        }

        for (ymin = 0; ymin < ymax; ymin++)
            if (elem[ymin].lbgn) {
                elem[ymin].lbgn->prev = curr;
                if (curr)
                    curr->next = elem[ymin].lbgn;
                else
                    retn = elem[ymin].lbgn;
                curr = elem[ymin].lend;
            }

        curr->next = 0;
        *tail = curr;
    }
    return retn;
}



INST *UpdateFrame(INST **tail, INST **pick, unsigned *time, VEC2 cptr) {
    INST *iter, *temp;
    ASTD *anim;
    long ytmp;

    iter = 0;
    if (*tail) {
        iter = temp = *tail;
        if (*pick == EMP_PICK) {
            while (temp) {
                anim = temp->anim;
                ytmp = temp->cpos.y - (anim->ydim << temp->scal);
                if ((cptr.x <  temp->cpos.x + (anim->xdim << temp->scal))
                &&  (cptr.x >= temp->cpos.x)
                &&  (cptr.y <  temp->cpos.y)
                &&  (cptr.y >= ytmp)

                &&  (anim->bptr.indx[((cptr.x - temp->cpos.x) >> temp->scal) +
                     anim->xdim * (((cptr.y - ytmp) >> temp->scal) +
                     anim->ydim * anim->fcur)] != 0xFF)) {
//                &&  (anim->bptr.bgra[((cptr.x - temp->cpos.x) >> temp->scal) +
//                     anim->xdim * (((cptr.y - ytmp) >> temp->scal) +
//                     anim->ydim * anim->fcur)].A)) {

                    temp->cptr.x = cptr.x - temp->cpos.x;
                    temp->cptr.y = cptr.y - temp->cpos.y;
                    *pick = temp;
                    break;
                }
                temp = temp->prev;
            }
            if (*pick != EMP_PICK)
                printf("%s\n", (*pick)->path);
        }
        while (!0) {
            anim = iter->anim;
            if ((*time - iter->time) > anim->time[anim->fcur]) {
                iter->time = *time;
                anim->fcur = (anim->fcur + 1) % anim->fcnt;
            }
            if (!iter->prev)
                break;
            iter = iter->prev;
        }
        if (*pick && (*pick != EMP_PICK)) {
            (*pick)->cpos.x = cptr.x - (*pick)->cptr.x;
            (*pick)->cpos.y = cptr.y - (*pick)->cptr.y;
            iter = SortByY(tail);
        }
    }
    return iter;
}



int BlendPixStdThrd(INST *head, PICT *draw, VEC2 *scrn) {
    int x, y, xmin, ymin, xmax, ymax, ysrc, ydst, yoff;
    BGRA pixl, b_r_, _g_a;
    ASTD *anim;

    if (!head)
        return 0;

    while (head) {
        anim = head->anim;

        yoff = head->cpos.y - (anim->ydim << head->scal);
        ymin = max(0, scrn->x - yoff);
        ymax = min(anim->ydim << head->scal, scrn->y - yoff);
        xmin = max(0, -head->cpos.x);
        xmax = min(anim->xdim << head->scal, draw->size.x - head->cpos.x);
        yoff = anim->xdim * anim->ydim * anim->fcur;

        /// alpha blending here
        for (y = ymin; y < ymax; y++) {
            ydst = (y + head->cpos.y - (anim->ydim << head->scal))
                 * draw->size.x + head->cpos.x;
            ysrc = (y >> head->scal) * anim->xdim + yoff;
            for (x = xmin; x < xmax; x++) {

                pixl = anim->bpal[
                       anim->bptr.indx[ysrc + (x >> head->scal)]];
//                pixl = anim->bptr.bgra[ysrc + (x >> head->scal)];

                if (pixl.A == 0xFF)
                    draw->bptr[ydst + x] = pixl;
                else if (pixl.A) {
                    _g_a.BGRA = ((draw->bptr[ydst + x].BGRA >> 8)
                              &  0x00FF00FF) * (0xFF - pixl.A);
                    b_r_.BGRA = ((draw->bptr[ydst + x].BGRA     )
                              &  0x00FF00FF) * (0xFF - pixl.A);
                    draw->bptr[ydst + x].BGRA = pixl.BGRA
                              + (0x00FF00FF  & (b_r_.BGRA >> 8))
                              + (0xFF00FF00  & (_g_a.BGRA     ));
                }
            }
        }
        head = head->next;
    }
    return ~0;
}



int MakeAnimStdThrd(INST *head, PICT *none, VEC2 *loop) {
    #define anih ((ASTD*)head->anim)
    if (!loop->y) {
        printf("Loader thread exited (%ld objects loaded).\n", loop->x);
        return 0;
    }
    if ((head->anim = MakeAnimStd(head->path, ~0))) {
        head->cpos.x = PRNG(&seed)
                     % (head->cpos.x - (anih->xdim << head->scal));
        head->cpos.y = PRNG(&seed)
                     % (head->cpos.y - (anih->ydim << head->scal))
                     + (anih->ydim << head->scal);
        anih->fcur = PRNG(&seed) % anih->fcnt;
        loop->x++;
        printf("DONE:\t%s\n", head->path);
    }
    else
        printf("FAILED:\t%s\n", head->path);
    return ~0;
    #undef anih
}
