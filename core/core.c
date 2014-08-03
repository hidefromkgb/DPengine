#include "core.h"



uint32_t guid = 0;
TREE *hstr = 0, *hpix = 0;
TMRD tmrd;



THR_FUNC ThrdFunc(THRD *data) {
    while (~0) {
        WaitSemaphore(data->orig, 0, data->uuid);
        if (!data->loop)
            break;
        data->func(data);
        if (!PickSemaphore(data->orig, 0, data->uuid))
            return THR_FAIL;
    }
    printf(TXT_EXIT"\n");
    PickSemaphore(data->orig, 0, data->uuid);
    return THR_EXIT;
}



void LTHR(THRD *data) {
    struct {
        char *name;
        long size;
    } apal;
    char *path, *file;
    long  indx;
    TREE *elem;
    ASTD *retn;

    retn = 0;
    path = data->path;
    elem = data->elem;
    if (path) {
        indx = strlen(path);
        if (((path[indx - 3] == 'g') || (path[indx - 3] == 'G'))
        &&  ((path[indx - 2] == 'i') || (path[indx - 2] == 'I'))
        &&  ((path[indx - 1] == 'f') || (path[indx - 1] == 'F'))
        &&  (retn = MakeAnimStd(path))) {
            *elem->epix->xdim = retn->xdim;
            *elem->epix->ydim = retn->ydim;
            *elem->epix->fcnt = retn->fcnt;
            *elem->epix->time = retn->time;
             elem->epix->scal = DownsampleAnimStd(retn, &elem->epix->xoff,
                                                        &elem->epix->yoff);
            apal.name = strdup(path);
            apal.name[indx - 3] = 'a';
            apal.name[indx - 2] = 'r';
            apal.name[indx - 1] = 't';
            file = LoadFile((void*)&apal);
            RecolorPalette(retn->bpal, file, apal.size);
            free(apal.name);
            free(file);
        }
        elem->epix->anim = retn;
    }
}



void PTHR(THRD *data) {
    DrawPixStdThrd(&data->orig->pict, data->orig->uarr, data->orig->data,
                    data->orig->size, data->ymin, data->ymax);
}



SEM_TYPE FindBit(SEM_TYPE inpt) {
    inpt &= inpt ^ (inpt - 1);
    return ((inpt & 0xFFFFFFFF00000000)? 32 : 0)
         + ((inpt & 0xFFFF0000FFFF0000)? 16 : 0)
         + ((inpt & 0xFF00FF00FF00FF00)?  8 : 0)
         + ((inpt & 0xF0F0F0F0F0F0F0F0)?  4 : 0)
         + ((inpt & 0xCCCCCCCCCCCCCCCC)?  2 : 0)
         + ((inpt & 0xAAAAAAAAAAAAAAAA)?  1 : 0);
}



TREE *LoadUnitStdThrd(TREE *elem, char *path) {
    SEM_TYPE curr;
    TREE *retn;

    curr = FindBit(WaitSemaphore(&tmrd, 1, SEM_NULL));
    retn = tmrd.thrd[curr].elem;
    tmrd.thrd[curr].elem = elem;
    tmrd.thrd[curr].path = path;
    PickSemaphore(&tmrd, 1, 1 << curr);
    return retn;
}



uint32_t DownsampleAnimStd(ASTD *anim, uint32_t *xoff, uint32_t *yoff) {
    long x, y, fram, dpos, retn,
         xmin, xmax, ymin, ymax;

    if (xoff)
        *xoff = 0;
    if (yoff)
        *yoff = 0;

    xmax = 0;
    xmin = anim->xdim;
    ymax = 0;
    ymin = anim->ydim;
    for (fram = 0; fram < anim->fcnt; fram++) {
        for (y = 0; y < anim->ydim; y++) {
            dpos = (fram * anim->ydim + y) * anim->xdim;
            retn = -1;
            for (x = 0; x < anim->xdim; x++)
                if (anim->bptr[dpos + x] != 0xFF) {
                    xmax = (x > xmax)? x : xmax;
                    xmin = (x < xmin)? x : xmin;
                    retn = 0;
                }
            if (!retn) {
                ymax = (y > ymax)? y : ymax;
                ymin = (y < ymin)? y : ymin;
            }
        }
        if ((xmin == 0) && (ymin == 0) &&
            (xmax == anim->xdim - 1) && (ymax == anim->ydim - 1)) {
            xmin = xmax + 1;
            break;
        }
    }
    if ((xmin <= xmax) && (ymin <= ymax)) {
        for (retn = fram = 0; fram < anim->fcnt; fram++)
            for (y = ymin; y <= ymax; y++) {
                dpos = (fram * anim->ydim + y) * anim->xdim;
                for (x = xmin; x <= xmax; x++)
                    anim->bptr[retn++] = anim->bptr[dpos + x];
            }
        anim->xdim = xmax - xmin + 1;
        anim->ydim = ymax - ymin + 1;
        anim->bptr = realloc(anim->bptr, anim->fcnt * anim->xdim * anim->ydim);
        if (xoff)
            *xoff = xmin;
        if (yoff)
            *yoff = ymin;
    }

    retn = 0;
    while (!(anim->xdim & 1) && !(anim->ydim & 1)) {
        for (fram = 0; fram < anim->fcnt; fram++)
            for (y = 0; y < anim->ydim; y += 2) {
                dpos = (fram * anim->ydim + y) * anim->xdim;
                for (x = 0; x < anim->xdim; x += 2)
                    if ((anim->bptr[dpos + x] !=
                         anim->bptr[dpos + x + 1])
                    ||  (anim->bptr[dpos + x] !=
                         anim->bptr[dpos + x + anim->xdim])
                    ||  (anim->bptr[dpos + x] !=
                         anim->bptr[dpos + x + anim->xdim + 1]))
                        goto _quit;
            }
        for (xmin = fram = 0; fram < anim->fcnt; fram++)
            for (y = 0; y < anim->ydim; y += 2) {
                dpos = (fram * anim->ydim + y) * anim->xdim;
                for (x = 0; x < anim->xdim; x += 2)
                    anim->bptr[xmin++] = anim->bptr[dpos + x];
            }
        anim->xdim >>= 1;
        anim->ydim >>= 1;
        retn++;
    }
    _quit:
    if (retn)
        anim->bptr = realloc(anim->bptr, anim->fcnt * anim->xdim * anim->ydim);

    return retn;
}



long CompareAnimStd(ASTD *a, ASTD *b, long flgs) {
    long x, y, fram, dpos, spos;

    if ((a->fcnt != b->fcnt) || (a->xdim != b->xdim) || (a->ydim != b->ydim))
        return 0;

    for (fram = 0; fram < a->fcnt; fram++)
        for (y = 0; y < a->ydim; y++) {
            spos = dpos = (fram * a->ydim + y) * a->xdim;
            if (flgs & 2)
                spos = (fram * a->ydim - y + a->ydim - 1) * a->xdim;
            if (flgs & 1) {
                spos += a->xdim - 1;
                for (x = 0; x < a->xdim; x++)
                    if (a->bpal[a->bptr[dpos + x]].BGRA !=
                        b->bpal[b->bptr[spos - x]].BGRA)
                        return 0;
            }
            else {
                for (x = 0; x < a->xdim; x++)
                    if (a->bpal[a->bptr[dpos + x]].BGRA !=
                        b->bpal[b->bptr[spos + x]].BGRA)
                        return 0;
            }
        }

    return ~0;
}



/// String-oriented linear hash multiplier
#define SLH_MULT 0xFBC5
/// String-oriented linear hash shift
#define SLH_PLUS 0x11

#define HASH(hash, trgt) hash = SLH_PLUS + SLH_MULT * hash + trgt.BGRA;
uint64_t HashAnimStd(ASTD *anim, long *flgs) {
    uint64_t hh00, hh01, hh10, hh11;
    long x, y, fram, dpos;

    if (!anim)
        return 0;

    hh00 = hh01 = hh10 = hh11 = 0;
    for (fram = 0; fram < anim->fcnt; fram++)
        for (y = 0; y < anim->ydim; y++) {
            dpos = (fram * anim->ydim + y) * anim->xdim;
            for (x = 0; x < anim->xdim; x++) {
                HASH(hh00, anim->bpal[anim->bptr[dpos + x]]);
                HASH(hh01, anim->bpal[anim->bptr[dpos - x + anim->xdim - 1]]);
            }
            dpos = (fram * anim->ydim - y + anim->ydim - 1) * anim->xdim;
            for (x = 0; x < anim->xdim; x++) {
                HASH(hh10, anim->bpal[anim->bptr[dpos + x]]);
                HASH(hh11, anim->bpal[anim->bptr[dpos - x + anim->xdim - 1]]);
            }
        }
    fram  = (hh00 > hh01)? 0b00 : 0b01;
    hh00  = (hh00 > hh01)? hh00 : hh01;
    dpos  = (hh10 > hh11)? 0b10 : 0b11;
    hh10  = (hh10 > hh11)? hh10 : hh11;
    *flgs = (hh00 > hh10)? fram : dpos;
    return  (hh00 > hh10)? hh00 : hh10;
}
#undef HASH



uint64_t HashLine(char *line) {
    uint64_t hash = 0;

    while (*line)
        hash = SLH_PLUS + SLH_MULT * hash + *line++;
    return hash;
}



char *ExtractLastDirs(char *path, long dcnt) {
    long iter;

    if (path && ((iter = strlen(path)) > 0)) {
        while (--iter)
            if (path[iter] == DEF_DSEP)
                if (!--dcnt) {
                    ++iter;
                    break;
                }
        path += iter;
    }
    return path;
}



void DrawPixStdThrd(PICT *pict, UNIT *uarr, T2UV *data,
                    long size, long yinf, long ysup) {
    long iter, x, y, ysrc, ydst, xmin, ymin, xmax, ymax,
         xoff, yoff, xinc, yinc, xpos, ypos;
    BGRA b_r_, _g_a, *bptr;
    UNIT *tail;
    ASTD *anim;

    for (iter = 0; iter < size; iter++) {
        tail = &uarr[(data[iter].y >> 2) & 0xFFFF];
        xpos = (int16_t)data[iter].x;
        ypos = (int16_t)(data[iter].x >> 16);
        anim = tail->anim;
        bptr = pict->bptr;
        xoff = pict->xdim;
        yoff = anim->xdim * anim->ydim * (data[iter].y >> 18);
        ymax = min(0, ysup - ypos);
        ymin = anim->ydim << tail->scal;
        xinc = anim->xdim << tail->scal;
        if (data[iter].y & 1) {
            xmax = min(xinc, xinc + xpos);
            xmin = max(   0, xinc + xpos - xoff);
            yinc = -1;
        }
        else {
            xmax = min(xinc, xoff - xpos);
            xmin = max(   0,    0 - xpos);
            yinc = 1;
        }
        xinc = ((yinc < 0)? xinc - 1 - xmin : xmin) + xpos;

        /// alpha blending here
        for (y = max(-ymin, yinf - ypos); y < ymax; y++) {
            ydst = (y + ypos) * xoff + xinc;
            ysrc = (data[iter].y & 2)? -y - 1 : y + ymin;
            ysrc = (ysrc >> tail->scal) * anim->xdim + yoff;
            for (x = xmin; x < xmax; x++, ydst += yinc)
                if (bptr[ydst].A != 0xFF) {
                    b_r_ = anim->bpal[anim->bptr[ysrc + (x >> tail->scal)]];
                    if (bptr[ydst].A == 0x00)
                        bptr[ydst] = b_r_;
                    else {
                        _g_a.BGRA = ((b_r_.BGRA >> 8) & 0x00FF00FF)
                                  * (0xFF - bptr[ydst].A);
                        b_r_.BGRA = ((b_r_.BGRA     ) & 0x00FF00FF)
                                  * (0xFF - bptr[ydst].A);
                        bptr[ydst].BGRA += ((b_r_.BGRA >> 8) & 0x00FF00FF)
                                        +  ((_g_a.BGRA     ) & 0xFF00FF00);
                    }
                }
        }
    }
}



void TreeDelPath(TREE *root) {
    free(root->path);
}



void TreeDelAnim(TREE *root) {
    FreeAnimStd((ASTD**)&root->anim);
}



void TreeDel(TREE **root, void (*func)(TREE*)) {
    if (*root) {
        TreeDel(&(*root)->fill, func);
        TreeDel(&(*root)->coll, func);
        TreeDel(&(*root)->next[0], func);
        TreeDel(&(*root)->next[1], func);
        if (func)
            func(*root);
        free(*root);
        *root = 0;
    }
}



TREE *TreeFind(TREE *root, uint64_t hash) {
    while (root) {
        if (hash == root->hash)
            break;
        root = root->next[(hash < root->hash)? 0 : 1];
    }
    return root;
}



/// modified and adapted from libAVL
void TreeAdd(TREE **root, TREE *elem) {
    TREE *btop, *iter, *rnew, *temp;
    long diff, rsub, lsub;

    elem->diff = 0;
    elem->prev = elem->coll = elem->fill = elem->next[0] = elem->next[1] = 0;
    iter = btop = *root;
    while (iter) {
        if (iter->diff)
            btop = iter;

        if (elem->hash == iter->hash) {
            elem->coll = iter->coll;
            iter->coll = elem;
            return;
        }
        diff = (elem->hash < iter->hash)? 0 : 1;
        elem->prev = iter;
        iter = iter->next[diff];
    }
    if (elem->prev) {
        elem->prev->next[diff] = iter = elem;
        do {
            iter->prev->diff += (iter->prev->next[0] == iter)? -1 : +1;
            iter = iter->prev;
        } while (iter != btop);

        if (btop->diff == ((btop->diff < 0)? -2 : +2)) {
            diff = (btop->diff < 0)? -1 : +1;
            rsub = 1 ^ (lsub = (diff + 1) >> 1);
            temp = btop->next[lsub];
            if (temp->diff == diff) {
                rnew = temp;
                btop->next[lsub] = temp->next[rsub];
                temp->next[rsub] = btop;
                temp->diff = btop->diff = 0;
                temp->prev = btop->prev;
                btop->prev = temp;
            }
            else {
                rnew = temp->next[rsub];
                temp->next[rsub] = rnew->next[lsub];
                rnew->next[lsub] = temp;
                btop->next[lsub] = rnew->next[rsub];
                rnew->next[rsub] = btop;
                temp->diff = (rnew->diff == -diff)? -rnew->diff : 0;
                btop->diff = (rnew->diff == +diff)? -rnew->diff : 0;
                rnew->diff = 0;
                rnew->prev = btop->prev;
                temp->prev = btop->prev = rnew;
                if (temp->next[rsub])
                    temp->next[rsub]->prev = temp;
            }
            if (btop->next[lsub])
                btop->next[lsub]->prev = btop;

            if (rnew->prev)
                rnew->prev->next[(rnew->prev->next[0] == btop)? 0 : 1] = rnew;
            else
                *root = rnew;
        }
    }
    else
        *root = elem;
}



void RecolorPalette(BGRA *bpal, char *file, long size) {
    #pragma pack(push, 1)
    struct {
        uint8_t srcr, srcg, srcb;
        uint8_t tran;
        uint8_t dstr, dstg, dstb;
    } *amap;
    #pragma pack(pop)

    char *apal;

    if (!bpal || !file)
        return;

    for (apal  = file + size -  sizeof(*amap);
         apal >= file;  apal -= sizeof(*amap))
        for (amap = (typeof(amap))apal,
             size = 0; size < 256; size++)
            if ((bpal[size].R == amap->srcr)
            &&  (bpal[size].G == amap->srcg)
            &&  (bpal[size].B == amap->srcb)) {
                 bpal[size].R = ((long)amap->dstr * amap->tran) >> 8;
                 bpal[size].G = ((long)amap->dstg * amap->tran) >> 8;
                 bpal[size].B = ((long)amap->dstb * amap->tran) >> 8;
                 bpal[size].A = amap->tran;
                 break;
            }
}



/// uses 1 global var: HPIX
long TryUpdatePixTree(TREE* estr) {
    TREE *epix;
    char stat;

    if (!estr || (estr->epix && (estr->epix->flgs & 4)))
        return 0;

    stat = ' ';
    estr->epix->hash = HashAnimStd(estr->epix->anim, &estr->epix->flgs);
    if (estr->epix->anim) {
        epix = TreeFind(hpix, estr->epix->hash);
        while (epix) {
            if (CompareAnimStd(epix->anim, estr->epix->anim,
                               epix->flgs ^ estr->epix->flgs))
                break;
            epix = epix->coll;
        }
        if (!epix) {
            TreeAdd(&hpix, estr->epix);
            *estr->epix->uuid = ++guid << 2;
            estr->epix->flgs |= 4;
        }
        else {
            estr->flgs = (estr->flgs & -4)
                       + ((epix->flgs ^ estr->epix->flgs) & 3);
            *estr->epix->uuid = (*epix->uuid & -4) + (estr->flgs & 3);
            *estr->epix->time = *epix->time;
            FreeAnimStd((ASTD**)&estr->epix->anim);
            TreeDel(&estr->epix, 0);
            estr->epix = epix;
            stat = '#';
        }
    }
    else {
        *estr->epix->uuid = 0;
        TreeDel(&estr->epix, 0);
    }
    if (estr->epix)
        printf(TXT_FANI" %s\n", *estr->epix->uuid >> 2, stat,
              (estr->flgs & 2)? 'D' : 'U', (estr->flgs & 1)? 'L' : 'R',
               estr->path);
    else
        printf(TXT_FAIL" %s\n", estr->path);
    return ~0;
}



/// uses 2 global vars: HSTR, GUID
void EngineLoadAnimAsync(uint8_t  *path, uint32_t *uuid,
                         uint32_t *xdim, uint32_t *ydim,
                         uint32_t *fcnt, uint32_t **time) {
    TREE *estr, *epix;
    char *ptrn;
    uint64_t hash;

    epix = calloc(1, sizeof(*epix));
    epix->uuid = uuid;
    epix->xdim = xdim;
    epix->ydim = ydim;
    epix->fcnt = fcnt;
    epix->time = time;
    ptrn = ExtractLastDirs((char*)path, 2);
    hash = (path)? HashLine(ptrn) : 0;
    estr = (path)? TreeFind(hstr, hash) : 0;
    while (estr) {
        if (!strcmp(estr->path, ptrn))
            break;
        estr = estr->coll;
    }
    if (!estr) {
        if (path) {
            estr = calloc(1, sizeof(*estr));
            estr->path = strdup(ptrn);
            estr->hash = hash;
            estr->epix = epix;
            TreeAdd(&hstr, estr);
        }
        else
            free(epix);
        /// the unit returned by LoadUnitStdThrd()
        /// may NOT be a unit for the name passed!
        TryUpdatePixTree(LoadUnitStdThrd(estr, (char*)path));
    }
    else if (estr->epix) {
        epix->fill = estr->fill;
        estr->fill = epix;
        printf(TXT_DUPL" %s\n", estr->path);
    }
    else
        free(epix);
}



void StopThreads(TMRD *tmrd) {
    ulong iter, loop;

    for (loop = iter = 0; iter < tmrd->ncpu; iter++)
        loop |= tmrd->thrd[iter].loop;
    if (loop) {
        WaitSemaphore(tmrd, 1, SEM_FULL);
        for (iter = 0; iter < tmrd->ncpu; iter++)
            tmrd->thrd[iter].loop = 0;
        PickSemaphore(tmrd, 1, SEM_FULL);
        WaitSemaphore(tmrd, 1, SEM_FULL);
    }
}



void SwitchThreads(TMRD *tmrd, long draw) {
    ulong iter, temp;

    if (!draw) {
        for (iter = 0; iter < tmrd->ncpu; iter++) {
            tmrd->thrd[iter] = (THRD){1, 1 << iter, tmrd, LTHR};
            MakeThread(&tmrd->thrd[iter]);
        }
    }
    else {
        temp = (tmrd->pict.ydim / tmrd->ncpu) + 1;
        for (iter = 0; iter < tmrd->ncpu; iter++) {
            tmrd->thrd[iter] = (THRD){1, 1 << iter, tmrd, PTHR,
                                     {temp * iter}, {temp * (iter + 1)}};
            MakeThread(&tmrd->thrd[iter]);
        }
        tmrd->thrd[tmrd->ncpu - 1].ymax = tmrd->pict.ydim;
    }
}



void EngineBeginAddition() {
    StopThreads(&tmrd);
    SwitchThreads(&tmrd, 0);
}



void EngineFinishLoading(long loop) {
    ulong mtmp, iter;

    tmrd.draw = 0;
    StopThreads(&tmrd);
    for (iter = 0; iter < tmrd.ncpu; iter++)
        TryUpdatePixTree(tmrd.thrd[iter].elem);
    MakeUnitArray(&tmrd.uarr);

    if (tmrd.uarr) {
        mtmp = TimeFunc() - tmrd.time;
        tmrd.uniq = tmrd.uarr[0].scal - 1;
        if (!loop)
            printf(TXT_AEND"\n", tmrd.uniq, mtmp, (float)mtmp / tmrd.uniq,
                  (tmrd.rndr == BRT_ROGL)? TXT_ROGL : TXT_RSTD);
        else
            InitRenderer(&tmrd);
        tmrd.draw = ~0;
    }
}



void FreeHashTrees() {
    TreeDel(&hstr, TreeDelPath);
    TreeDel(&hpix, TreeDelAnim);
    guid = 0;
}



void FreeUnitArray(UNIT **uarr) {
    free(*uarr);
    *uarr = 0;
}



void FillDuplicates(TREE *root) {
    if (!root)
        return;
    FillDuplicates(root->next[1]);
    FillDuplicates(root->next[0]);
    FillDuplicates(root->coll);

    TREE *fill = root->fill;
    while (root->epix && fill) {
        *fill->uuid = *root->epix->uuid;
        *fill->xdim = *root->epix->xdim;
        *fill->ydim = *root->epix->ydim;
        *fill->fcnt = *root->epix->fcnt;
        *fill->time = *root->epix->time;
        fill = fill->fill;
    }
}



void UnitArrayFromTree(UNIT *uarr, TREE *root) {
    if (!root || !uarr)
        return;
    UnitArrayFromTree(uarr, root->next[1]);
    UnitArrayFromTree(uarr, root->next[0]);
    UnitArrayFromTree(uarr, root->coll);
    uarr[*root->uuid >> 2].anim = root->anim;
    uarr[*root->uuid >> 2].scal = root->scal;
    uarr[*root->uuid >> 2].offs[0] = root->xoff;
    uarr[*root->uuid >> 2].offs[2] = root->yoff;
    uarr[*root->uuid >> 2].offs[1] = *root->xdim - root->xoff
                                   - (((ASTD*)root->anim)->xdim << root->scal);
    uarr[*root->uuid >> 2].offs[3] = *root->ydim - root->yoff
                                   - (((ASTD*)root->anim)->ydim << root->scal);
}



/// uses 3 global vars: HSTR, HPIX, GUID
void MakeUnitArray(UNIT **uarr) {
    FreeUnitArray(uarr);
    if (hpix) {
        FillDuplicates(hstr);
        *uarr = calloc(guid + 1, sizeof(**uarr));
        UnitArrayFromTree(*uarr, hpix);
        (*uarr)[0].scal = guid + 1;
    }
}



long SelectUnit(UNIT *uarr, T2UV *data, long size, long xptr, long yptr) {
    long iter, xpos, ypos;
    ASTD *anim;

    for (iter = 0; iter < size; iter++) {
        if (!(ypos = (data[iter].y >> 2) & 0xFFFF))
            continue;
        anim = uarr[ypos].anim;
        xpos = ( xptr - uarr[ypos].offs[(data[iter].y & 1)? 1 : 0]
             - (int16_t)(data[iter].x      )) >> uarr[ypos].scal;
        ypos = (-yptr - uarr[ypos].offs[(data[iter].y & 2)? 2 : 3]
             + (int16_t)(data[iter].x >> 16)) >> uarr[ypos].scal;
        if ((xpos >= 0) && (xpos < anim->xdim)
        &&  (ypos >= 0) && (ypos < anim->ydim)) {
            if (data[iter].y & 1)
                xpos = anim->xdim - 1 - xpos;
            if (!(data[iter].y & 2))
                ypos = anim->ydim - 1 - ypos;
            if (anim->bptr[((data[iter].y >> 18) * anim->ydim + ypos)
                           * anim->xdim + xpos] != 0xFF)
                break;
        }
    }
    return (iter < size)? iter : -1;
}
