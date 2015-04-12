#include <core.h>



void OutputFPS(ENGD *engd, char retn[]) {
    float corr = ((engd->tfps > 0) && (engd->time > engd->tfps))?
                 1000.0 / (engd->time - engd->tfps) : 1.0;

    sprintf(retn, TXL_UFPS, corr * engd->fram);
    engd->tfps = TimeFunc();
    engd->fram = 0;
}



SEM_TYPE FindBit(SEM_TYPE inpt) {
    inpt &= inpt ^ (inpt - 1);
    return ((inpt & 0xFFFFFFFF00000000ULL)? 32 : 0)
         + ((inpt & 0xFFFF0000FFFF0000ULL)? 16 : 0)
         + ((inpt & 0xFF00FF00FF00FF00ULL)?  8 : 0)
         + ((inpt & 0xF0F0F0F0F0F0F0F0ULL)?  4 : 0)
         + ((inpt & 0xCCCCCCCCCCCCCCCCULL)?  2 : 0)
         + ((inpt & 0xAAAAAAAAAAAAAAAAULL)?  1 : 0);
}



long DownsampleAnimStd(ASTD *anim, long *xoff, long *yoff) {
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
uint64_t HashAnimStd(ASTD *anim, long *turn) {
    uint64_t hh00, hh01, hh10, hh11;
    long x, y, fram, dpos, rpos;

    if (!anim)
        return 0;

    hh00 = hh01 = hh10 = hh11 = 0;
    for (fram = 0; fram < anim->fcnt; fram++)
        for (y = anim->ydim >> 1; y >= 0; y--) {
            dpos = (fram * anim->ydim + y) * anim->xdim;
            rpos = (fram * anim->ydim - y + anim->ydim - 1) * anim->xdim;
            for (x = anim->xdim >> 1; x >= 0; x--) {
                HASH(hh00, anim->bpal[anim->bptr[dpos + x]]);
                HASH(hh01, anim->bpal[anim->bptr[dpos - x + anim->xdim - 1]]);
                HASH(hh10, anim->bpal[anim->bptr[rpos + x]]);
                HASH(hh11, anim->bpal[anim->bptr[rpos - x + anim->xdim - 1]]);
            }
        }
    fram  = (hh00 > hh01)?    0 :    1;
    hh00  = (hh00 > hh01)? hh00 : hh01;
    dpos  = (hh10 > hh11)?    2 :    3;
    hh10  = (hh10 > hh11)? hh10 : hh11;
    *turn = (hh00 > hh10)? fram : dpos;
    return  (hh00 > hh10)? hh00 : hh10;
}
#undef HASH



uint64_t HashLine64(char *line) {
    uint64_t hash = 0;

    while (*line)
        hash = SLH_PLUS + SLH_MULT * hash + *line++;
    return hash;
}



char *ExtractLastDirs(char *path, long dcnt) {
    long iter;

    if (path && ((iter = strlen(path)) > 0)) {
        while (--iter)
            if ((path[iter] == DEF_DSEP) && !--dcnt) {
                ++iter;
                break;
            }
        path += iter;
    }
    return path;
}



void FlushDest(SAVL *elem, long fill) {
    DEST *item;

    while (elem->dest) {
        item = elem->dest;
        elem->dest = elem->dest->next;
        if (fill) {
            if (elem->epix) {
                *item->ainf = elem->epix->ainf;
                item->ainf->uuid ^= elem->turn;
            }
            else
                *item->ainf = (AINF){};
        }
        free(item);
    }
}



void TreeDelPath(SAVL *elem) {
    FlushDest(elem, 0);
    free(elem->path);
}



void TreeDelAnim(PAVL *elem) {
    FreeAnimStd(&elem->anim);
}



void TreeDel(TREE **root, ITER func) {
    if (*root) {
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



void TreeAdd(TREE **root, TREE *elem) {
    TREE *btop, *iter, *rnew, *temp;
    long diff, rsub, lsub;

    diff = elem->diff = 0;
    elem->prev = elem->coll = elem->next[0] = elem->next[1] = 0;
    iter = btop = *root;
    while (iter) {
        if (iter->diff)
            btop = iter;

        if (elem->hash == iter->hash) {
            elem->coll = iter->coll;
            iter->coll = elem;
            return;
        }
        elem->prev = iter;
        iter = iter->next[diff = (elem->hash < iter->hash)? 0 : 1];
    }
    if (elem->prev) {
        elem->prev->next[diff] = iter = elem;
        do {
            iter->prev->diff += (iter->prev->next[0] == iter)? -1 : +1;
            iter = iter->prev;
        } while (iter != btop);

        diff = (btop->diff < 0)? -1 : +1;
        if (btop->diff == diff * 2) {
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



uint32_t RecolorPalette(BGRA *bpal, char *file, long size) {
    #pragma pack(push, 1)
    struct {
        uint8_t srcr, srcg, srcb;
        uint8_t tran;
        uint8_t dstr, dstg, dstb;
    } *amap;
    #pragma pack(pop)

    char *apal;
    uint32_t retn = 0;

    if (bpal && file)
        for (apal  = file + size -  sizeof(*amap);
             apal >= file;  apal -= sizeof(*amap))
            for (amap = (typeof(amap))apal, size = 0; size < 256; size++)
                if ((bpal[size].R == amap->srcr)
                &&  (bpal[size].G == amap->srcg)
                &&  (bpal[size].B == amap->srcb)
                &&  (bpal[size].A == 0xFF)) {
                     bpal[size].R = ((long)amap->dstr * amap->tran) >> 8;
                     bpal[size].G = ((long)amap->dstg * amap->tran) >> 8;
                     bpal[size].B = ((long)amap->dstb * amap->tran) >> 8;
                     bpal[size].A = amap->tran;
                     if (amap->tran < 0xFF)
                        retn++;
                     break;
                }

    return retn;
}



long TryUpdatePixTree(ENGD *engd, SAVL* estr) {
    long turn, stat;
    PAVL *epix;

    if (!estr)
        return 0;

    stat = ' ';
    if (estr->epix->anim) {
        /// searching for the appropriate animation
        epix = (PAVL*)TreeFind((TREE*)engd->hpix, estr->epix->hash);
        while (epix) {
            if (CompareAnimStd(epix->anim, estr->epix->anim,
                               epix->ainf.uuid ^ estr->turn))
                break;
            epix = (PAVL*)epix->coll;
        }
        /// not found, the animation is new
        if (!epix) {
            TreeAdd((TREE**)&engd->hpix, (TREE*)estr->epix);
            estr->epix->ainf.uuid = (++engd->uniq << 2) | estr->turn;
        }
        /// found, replacing
        else {
            TreeDel((TREE**)&estr->epix, (ITER)TreeDelAnim);
            estr->epix = epix;
            stat = '#';
        }
    }
    else
        TreeDel((TREE**)&estr->epix, 0);

    if (estr->epix) {
        turn = estr->epix->ainf.uuid ^ estr->turn;
        printf(TXL_UANI" %s\n", estr->epix->ainf.uuid >> 2, (char)stat,
              (turn & 2)? 'D' : 'U', (turn & 1)? 'L' : 'R', estr->path);
    }
    else
        printf(TXL_FAIL" %s\n", estr->path);
    return ~0;
}



void FreeHashTrees(ENGD *engd) {
    TreeDel((TREE**)&engd->hstr, (ITER)TreeDelPath);
    TreeDel((TREE**)&engd->hpix, (ITER)TreeDelAnim);
    engd->uniq = 0;
}



void FillDest(SAVL *root) {
    if (!root)
        return;

    FillDest((SAVL*)root->next[1]);
    FillDest((SAVL*)root->next[0]);
    FillDest((SAVL*)root->coll);
    FlushDest(root, ~0);
}



void UnitArrayFromTree(UNIT *uarr, PAVL *root) {
    if (!root || !uarr)
        return;

    UnitArrayFromTree(uarr, (PAVL*)root->next[1]);
    UnitArrayFromTree(uarr, (PAVL*)root->next[0]);
    UnitArrayFromTree(uarr, (PAVL*)root->coll);

    ASTD *anim = (ASTD*)root->anim;
    long iter = root->ainf.uuid >> 2;

    uarr[iter].anim = root->anim;
    uarr[iter].scal = root->scal;
    uarr[iter].tran = root->tran;
    uarr[iter].offs[0] = root->xoff;
    uarr[iter].offs[2] = root->yoff;
    uarr[iter].offs[1] = root->ainf.dims.x - root->xoff
                       - (anim->xdim << root->scal);
    uarr[iter].offs[3] = root->ainf.dims.y - root->yoff
                       - (anim->ydim << root->scal);
}



void FreeUnitArray(ENGD *engd) {
    free(engd->uarr);
    engd->uarr = 0;
}



void MakeUnitArray(ENGD *engd) {
    FreeUnitArray(engd);
    if (engd->hstr) {
        FillDest(engd->hstr);
        engd->uarr = calloc(engd->uniq + 1, sizeof(*engd->uarr));
        UnitArrayFromTree(engd->uarr, engd->hpix);
    }
}



long SelectUnit(UNIT *uarr, T4FV *data, long size, long xptr, long yptr) {
    long iter, indx, xpos, ypos;
    ASTD *anim;

    for (iter = 0; iter < size; iter++) {
        indx = data[iter].w;
        if (!(ypos = indx >> 2))
            continue;
        anim = uarr[ypos].anim;
        xpos = ( xptr - uarr[ypos].offs[(indx & 1)? 1 : 0]
             - (long)data[iter].x) >> uarr[ypos].scal;
        ypos = (-yptr - uarr[ypos].offs[(indx & 2)? 2 : 3]
             + (long)data[iter].y) >> uarr[ypos].scal;
        if ((xpos >= 0) && (xpos < anim->xdim)
        &&  (ypos >= 0) && (ypos < anim->ydim)) {
            if (indx & 1)
                xpos = anim->xdim - 1 - xpos;
            if (!(indx & 2))
                ypos = anim->ydim - 1 - ypos;
            if (anim->bptr[((long)data[iter].z * anim->ydim + ypos)
                           * anim->xdim + xpos] != 0xFF)
                break;
        }
    }
    return (iter < size)? iter : -1;
}



THR_FUNC ThrdFunc(THRD *data) {
    while (~0) {
        WaitSemaphore(data->orig, 0, data->uuid);
        if (!data->loop)
            break;
        data->func(data);
        if (!PickSemaphore(data->orig, 0, data->uuid))
            return THR_FAIL;
    }
    printf(TXL_EXIT"\n");
    PickSemaphore(data->orig, 0, data->uuid);
    return THR_EXIT;
}



void LTHR(THRD *data) {
    char *name, *path, *file;
    long  indx;
    SAVL *elem;
    ASTD *retn;

    retn = 0;
    path = data->path;
    elem = data->elem;
    if (path) {
        indx = strlen(path);
        if (((path[indx - 3] == 'g') || (path[indx - 3] == 'G'))
        &&  ((path[indx - 2] == 'i') || (path[indx - 2] == 'I'))
        &&  ((path[indx - 1] == 'f') || (path[indx - 1] == 'F'))
        &&  (retn = MakeFileAnimStd(path))) {
            name = strdup(path);
            name[indx - 3] = 'a';
            name[indx - 2] = 'r';
            name[indx - 1] = 't';
            file = LoadFile(name, &indx);
            elem->epix->ainf = (AINF){0, retn->fcnt, retn->time,
                                        {retn->xdim, retn->ydim}};
            elem->epix->tran = RecolorPalette(retn->bpal, file, indx);
            elem->epix->scal = DownsampleAnimStd(retn, &elem->epix->xoff,
                                                       &elem->epix->yoff);
            elem->epix->hash = HashAnimStd(retn, &indx);
            elem->turn = indx & 3;
            free(file);
            free(name);
        }
        elem->epix->anim = retn;
    }
}



void PTHR(THRD *data) {
    long iter, indx, x, y,
         ysrc, ydst, xmin, ymin, xmax, ymax,
         xoff, yoff, xinc, yinc, xpos, ypos;
    BGRA b_r_, _g_a, *bptr;
    UNIT *tail;
    ASTD *anim;

    bptr = data->orig->pict.bptr;
    xoff = data->orig->pict.xdim;
    for (iter = 0; iter < data->orig->size; iter++) {
        xpos = data->orig->data[iter].x;
        ypos = data->orig->data[iter].y;
        indx = data->orig->data[iter].w;
        tail = &data->orig->uarr[indx >> 2];
        anim = tail->anim;
        yoff = anim->xdim * anim->ydim * data->orig->data[iter].z;
        ymax = min(0, data->ymax - ypos);
        ymin = anim->ydim << tail->scal;
        xinc = anim->xdim << tail->scal;
        if (indx & 1) {
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
        for (y = max(-ymin, data->ymin - ypos); y < ymax; y++) {
            ydst = (y + ypos) * xoff + xinc;
            ysrc = (indx & 2)? -y - 1 : y + ymin;
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



void StopThreads(ENGD *engd) {
    ulong iter, loop;

    for (loop = iter = 0; iter < engd->ncpu; iter++)
        loop |= engd->thrd[iter].loop;
    if (loop) {
        WaitSemaphore(engd, 1, SEM_FULL);
        for (iter = 0; iter < engd->ncpu; iter++)
            engd->thrd[iter].loop = 0;
        PickSemaphore(engd, 1, SEM_FULL);
        WaitSemaphore(engd, 1, SEM_FULL);
    }
}



void SwitchThreads(ENGD *engd, long draw) {
    ulong iter, temp;

    if (draw) {
        temp = (engd->pict.ydim / engd->ncpu) + 1;
        for (iter = 0; iter < engd->ncpu; iter++) {
            engd->thrd[iter] = (THRD){1, 1 << iter, engd, PTHR,
                                     {temp * iter}, {temp * (iter + 1)}};
            MakeThread(&engd->thrd[iter]);
        }
        engd->thrd[engd->ncpu - 1].ymax = engd->pict.ydim;
    }
    else
        for (iter = 0; iter < engd->ncpu; iter++) {
            engd->thrd[iter] = (THRD){1, 1 << iter, engd, LTHR};
            MakeThread(&engd->thrd[iter]);
        }
}



BGRA *ExtractRescaleSwizzleAlign(ASTD *anim, uint8_t swiz,
                                 long fram, long xdim, long ydim) {
    BGRA *offs, *retn = 0;
    long x, y, xcoe, ycoe;
    uint8_t *bptr;

    if (anim && anim->xdim && anim->ydim && (fram < anim->fcnt)
    && (anim->xdim <= xdim) && (anim->ydim <= ydim)) {
        xcoe = xdim / anim->xdim;
        ycoe = ydim / anim->ydim;
        offs = (retn = calloc(sizeof(*retn), xdim * ydim))
             +  xdim * ((ydim % anim->ydim) >> 1) + ((xdim % anim->xdim) >> 1);
        bptr = anim->bptr + anim->xdim * anim->ydim * fram;
        for (y = anim->ydim * ycoe - 1; y >= 0; y--)
            for (x = anim->xdim * xcoe - 1; x >= 0; x--) {
                BGRA temp = anim->bpal[bptr[anim->xdim * (y / ycoe)
                                                       + (x / xcoe)]];
                offs[xdim * y + x].B = temp.chan[(swiz >> 0) & 3];
                offs[xdim * y + x].G = temp.chan[(swiz >> 2) & 3];
                offs[xdim * y + x].R = temp.chan[(swiz >> 4) & 3];
                offs[xdim * y + x].A = temp.chan[(swiz >> 6) & 3];
            }
    }
    return retn;
}



void MenuHandler(MENU *item) {
    switch (item->uuid) {
        case MMI_RSTD:
        case MMI_ROGL:
            RestartEngine((ENGD*)item->data,
                          (item->uuid == MMI_RSTD)? SCM_RSTD : SCM_ROGL);
            break;

        case MMI_OPAQ: {
            ENGD *engd = (ENGD*)item->data;

            if (item->flgs & MFL_VCHK)
                engd->flgs |= COM_IOPQ;
            else
                engd->flgs &= ~COM_IOPQ;
            RestartEngine(engd, engd->rscm);
            break;
        }
        case MMI_STOP: {
            ENGD *engd = (ENGD*)item->data;

            engd->draw = !(item->flgs & MFL_VCHK);
            break;
        }
        case MMI_HIDE:
            ShowMainWindow((ENGD*)item->data, !(item->flgs & MFL_VCHK));
            break;

        case MMI_EXIT:
            RestartEngine((ENGD*)item->data, SCM_QUIT);
            break;
    }
}



void ProcessMenuItem(MENU *item) {
    MENU *indx;

    if (item->flgs & MFL_CCHK) {
        if (item->flgs & MFL_RCHK & ~MFL_CCHK) {
            if (item->flgs & MFL_VCHK)
                return;
            item->flgs |= MFL_VCHK;
            indx = item;
            while ((--indx)->flgs & MFL_RCHK & ~MFL_CCHK)
                indx->flgs &= ~MFL_VCHK;
            indx = item;
            while ((++indx)->flgs & MFL_RCHK & ~MFL_CCHK)
                indx->flgs &= ~MFL_VCHK;
        }
        else
            item->flgs ^= MFL_VCHK;
    }
    if (!item->chld && item->func)
        item->func(item);
}



void EngineFreeMenu(MENU **menu) {
    if (!menu || !*menu)
        return;

    MENU *iter = *menu;

    while (iter->text) {
        if (iter->chld)
            EngineFreeMenu(&iter->chld);
        free(iter->text);
        iter++;
    }
    free(*menu - 1);
    *menu = 0;
}



MENU *EngineMenuFromTemplate(MENU *tmpl) {
    MENU *retn, *iter;

    retn = 0;
    if ((iter = tmpl)) {
        while (iter++->text);
        if (iter > tmpl + 1) {
            retn = calloc(iter - tmpl + 1, sizeof(*iter));
            iter = ++retn;
            do {
                *iter = *tmpl;
                if (tmpl->chld)
                    iter->chld = EngineMenuFromTemplate(tmpl->chld);
                iter++->text = (tmpl->text)?
                               (uint8_t*)ConvertUTF8((char*)tmpl->text) : 0;
            } while (tmpl++->text);
        }
    }
    return retn;
}



uint32_t LoadLocalization(uint8_t ***text, uint8_t *data, uint32_t size) {
    long line, nlin, iter, prev;
    uint8_t **retn;

    if (!data)
        return 0;

    /// skipping byte-order mark
    if ((size >= 3) &&
        (data[0] == 0xEF) && (data[1] == 0xBB) && (data[2] == 0xBF)) {
        data += 3;
        size -= 3;
    }
    if (*text)
        for (nlin = 0; (*text)[nlin]; nlin++);
    else {
        for (nlin = 2, iter = 0; iter < size; iter++)
            if (data[iter] == '\n')
                nlin++;
        *text = calloc(nlin, sizeof(*retn));
    }
    retn = *text;
    for (line = iter = prev = 0; (line < nlin) && (iter < size); iter++)
        if ((data[iter] == '\n') || (iter == size - 1)) {
            if (iter - ((data[prev] == '\r')? 1 : 0) > prev) {
                free(retn[line]);
                retn[line] = calloc(iter - prev + 1, sizeof(**retn));
                strncpy((char*)retn[line], (char*)&data[prev],
                        iter - prev - ((data[iter - 1] == '\r')? 1 : 0));
            }
            line++;
            prev = iter + 1;
        }
    return nlin;
}



void FreeLocalization(uint8_t ***text) {
    long iter = -1;

    if (text && *text) {
        while ((*text)[++iter])
            free((*text)[iter]);
        free(*text);
        *text = 0;
    }
}



uintptr_t EngineInitialize() {
    ENGD *retn = calloc(1, sizeof(*retn));

    retn->ncpu = CountCPUs();
    retn->thrd = malloc(retn->ncpu * sizeof(*retn->thrd));
    MakeSemaphore(&retn->isem, retn->ncpu, SEM_NULL);
    MakeSemaphore(&retn->osem, retn->ncpu, SEM_FULL);
    SwitchThreads(retn, 0);
    retn->time = TimeFunc();
    return (uintptr_t)retn;
}



void EngineLoadAnimAsync(uintptr_t engh, uint8_t *path, AINF *ainf) {
    ENGD *engd = (ENGD*)engh;
    SAVL *estr, *retn;
    DEST *dest;
    char *ptrn;

    SEM_TYPE curr;
    uint64_t hash;

    ptrn = ExtractLastDirs((char*)path, 2);
    hash = (path)? HashLine64(ptrn) : 0;
    estr = (path)? (SAVL*)TreeFind((TREE*)engd->hstr, hash) : 0;
    while (estr) {
        if (!strcmp(estr->path, ptrn))
            break;
        estr = (SAVL*)estr->coll;
    }
    if (!estr) {
        if (path) {
            estr = calloc(1, sizeof(*estr));
            estr->hash = hash;
            estr->path = strdup(ptrn);
            estr->epix = calloc(1, sizeof(*estr->epix));
            estr->dest = calloc(1, sizeof(*estr->dest));
            estr->dest->ainf = ainf;
            TreeAdd((TREE**)&engd->hstr, (TREE*)estr);
        }
        curr = FindBit(WaitSemaphore(engd, 1, SEM_NULL));
        retn = engd->thrd[curr].elem;
        engd->thrd[curr].elem = estr;
        free(engd->thrd[curr].path);
        engd->thrd[curr].path = strdup((char*)path);
        PickSemaphore(engd, 1, 1 << curr);
        TryUpdatePixTree(engd, retn);
    }
    else if (estr->epix) {
        dest = calloc(1, sizeof(*dest));
        dest->next = estr->dest;
        estr->dest = dest;
        dest->ainf = ainf;
        printf(TXL_DUPL" %s\n", estr->path);
    }
}



void EngineBeginAddition(uintptr_t engh) {
    ENGD *engd = (ENGD*)engh;

    StopThreads(engd);
    SwitchThreads(engd, 0);
}



void EngineFinishLoading(uintptr_t engh) {
    ENGD *engd = (ENGD*)engh;
    ulong iter;

    engd->draw = 0;
    StopThreads(engd);
    for (iter = 0; iter < engd->ncpu; iter++) {
        TryUpdatePixTree(engd, engd->thrd[iter].elem);
        free(engd->thrd[iter].path);
        engd->thrd[iter].path = 0;
    }
    MakeUnitArray(engd);

    if (engd->uarr) {
        if (engd->data)
            InitRenderer(engd);
        engd->draw = ~0;
    }
}



void EngineRunMainLoop(uintptr_t engh, uint32_t xdim, uint32_t ydim,
                       uint32_t  flgs, uint32_t msec, uint32_t rscm,
                       uintptr_t user, uint8_t *lang, uint32_t size,
                       UFRM func) {
    INCBIN("../core/en.lang", DefaultLanguage);

    ENGD *engd = (ENGD*)engh;
    uint8_t *data;
    long mtmp;

    if (engd->uarr) {
        engd->pict.xdim = xdim;
        engd->pict.ydim = ydim;

        engd->ufrm = func;
        engd->udat = user;
        engd->rscm = rscm;
        engd->flgs = flgs;
        engd->size = size;
        engd->msec = msec;
        engd->data = calloc(size, sizeof(*engd->data));

        mtmp = TimeFunc() - engd->time;
        printf(TXL_AEND" %lu objects, %lu ms: %0.3f ms/obj\n%s\n",
               engd->uniq, mtmp, (float)mtmp / engd->uniq,
              (engd->rscm == SCM_ROGL)? TXL_ROGL : TXL_RSTD);

        FreeLocalization(&engd->tran);
        for (mtmp = 0; DefaultLanguage[mtmp]; mtmp++);
        LoadLocalization(&engd->tran, (uint8_t*)DefaultLanguage, mtmp);
        if (lang) {
            data = (uint8_t*)LoadFile((char*)lang, &mtmp);
            LoadLocalization(&engd->tran, data, mtmp);
            free(data);
        }
        MENU *spec,
        menu[] =
       {{.text = engd->tran[TXT_HEAD], .flgs = MFL_GRAY},
        {.text = (uint8_t*)""},
        {.text = engd->tran[TXT_RSCM]},
        {.text = engd->tran[TXT_SPEC]},
        {.text = engd->tran[TXT_OPAQ], .uuid = MMI_OPAQ, .func = MenuHandler,
         .flgs = MFL_CCHK | ((engd->flgs & COM_IOPQ)? MFL_VCHK : 0),
         .data = (uintptr_t)engd},
        {.text = engd->tran[TXT_STOP], .uuid = MMI_STOP, .func = MenuHandler,
         .flgs = MFL_CCHK,
         .data = (uintptr_t)engd},
        {.text = engd->tran[TXT_HIDE], .uuid = MMI_HIDE, .func = MenuHandler,
         .flgs = MFL_CCHK,
         .data = (uintptr_t)engd},
        {.text = (uint8_t*)""},
        {.text = engd->tran[TXT_EXIT], .uuid = MMI_EXIT, .func = MenuHandler,
         .data = (uintptr_t)engd},
        {}},

        rndr[] =
       {{.text = engd->tran[TXT_RSTD], .uuid = MMI_RSTD, .func = MenuHandler,
         .flgs = MFL_RCHK | ((engd->rscm == SCM_RSTD)? MFL_VCHK : 0),
         .data = (uintptr_t)engd},
        {.text = engd->tran[TXT_ROGL], .uuid = MMI_ROGL, .func = MenuHandler,
         .flgs = MFL_RCHK | ((engd->rscm == SCM_ROGL)? MFL_VCHK : 0),
         .data = (uintptr_t)engd},
        {}},

        none[] =
       {{.text = engd->tran[TXT_NONE], .flgs = MFL_GRAY},
        {}};

        menu[2].chld = rndr;
        engd->menu = EngineMenuFromTemplate(menu);
        engd->menu[3].chld = ((spec = OSSpecificMenu(engd)))?
                               spec : EngineMenuFromTemplate(none);
        do {
            RunMainLoop(engd);
        } while ((engd->rscm = engd->draw) != SCM_QUIT);

        EngineFreeMenu(&engd->menu);
        free(engd->data);
        engd->data = 0;
    }
    else
        printf(TXL_FAIL" No animation base found! Exiting...\n");
}



void EngineDeinitialize(uintptr_t engh) {
    ENGD *engd = (ENGD*)engh;

    if (engd->uarr) {
        FreeUnitArray(engd);
        FreeHashTrees(engd);
    }
    FreeLocalization(&engd->tran);
    FreeSemaphore(&engd->isem, engd->ncpu);
    FreeSemaphore(&engd->osem, engd->ncpu);
    free(engd->thrd);
    free(engd);
}
