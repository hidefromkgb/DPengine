#include <ogl/oglstd.h>
#include <core.h>



#define SEM_NULL 0
#define SEM_FULL ~SEM_NULL

#define TXL_FAIL "[>>ERR<<]"
#define TXL_DUPL "[--DUP--]"
#define TXL_AEND "[==ANI==]"
#define TXL_RGPU "[##GPU##]"
#define TXL_RSTD "[++CPU++]"

/// internal options (see ENGD::IFLG)
#define IFL_HALT (1 << 0)



/// AINF destination (to put one "physical" animation into multiple AINFs)
typedef struct _DEST {
    struct _DEST *next;
    AINF *ainf;
} DEST;

/// AVL hash tree, opaque outside the module
typedef struct _TREE {
    struct _TREE *next[3]; /// NEXT[2] is used for collisions
    long diff;
    uint64_t hash;
    union {
        struct {
            /// pixel-data tree element
            ASTD *anim;
            AINF  ainf;
            long  xoff, yoff, scal, tran;
        };
        struct {
            /// filename tree element
            typeof(((AINF*)0)->uuid) turn;
            char *path;
            DEST *dest;
            struct _TREE *epix;
        };
    };
} TREE;

/// AVL hash tree iterator
typedef void (*ITER)(TREE*);

/// thread data, opaque outside the module
typedef struct _THRD {
    ulong loop;
    SEM_TYPE uuid;
    ENGD *orig;
    void (*udis)(void*);
    void (*func)(struct _THRD*);
    union {
        struct {
            long ymin, ymax;
        };
        struct {
            TREE *elem;
            void *data;
            long  flgs;
        };
    };
} THRD;

/// engine data, opaque outside the module
struct ENGD {
    uint32_t flgs,    /// options
             dflg,    /// deferred options (those which are set upon restart)
             iflg,    /// internal runtime options (halt flag, etc.)
             size,    /// current length of the main display list
             smax,    /// maximum capacity of the main display list
             fram,    /// FPS counter
             msec,    /// frame delay
             ncpu,    /// number of CPU cores the engine is allowed to occupy
             uniq;    /// number of unique animations
    uint64_t tfrm,    /// timestamp for the previous frame
             tfps;    /// timestamp for the previous FPS count
    UFRM     ufrm;    /// callback to update the state of a frame
    T4IV     dims;    /// drawing area position and dimensions
    BGRA    *bptr;    /// drawing area bits
    UNIT    *uarr;    /// unique animations source in array form
    T4FV    *data;    /// main display list (updated every frame; FOREIGN!)
    THRD    *thrd;    /// thread data array
    FRBO    *surf;    /// supplementary renderbuffer
    RNDR    *rndr;    /// supplementary renderer (the exact type may vary)
    TREE    *hstr,    /// AVL tree for animation filename hashes & HPIX links
            *hpix;    /// AVL tree for animation pixel data (including hashes)
    SEMD    *isem,    /// incoming semaphore
            *osem;    /// outgoing semaphore
    intptr_t udat,    /// user-defined data to be passed to the frame updater
             user[4]; /// user-defined additional values, just in case
};



void cOutputFPS(ENGD *engd, char retn[]) {
    uint64_t time = lTimeFunc();
    float corr = ((engd->tfps > 0) && (time > engd->tfps))?
                 1000.0 / (time - engd->tfps) : 1.0;

    sprintf(retn, "[%7.3f]", corr * engd->fram);
    engd->tfps = time;
    engd->fram = 0;
}



SEM_TYPE cFindBit(SEM_TYPE inpt) {
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
                    if (a->bpal[a->bptr[dpos + x]].bgra !=
                        b->bpal[b->bptr[spos - x]].bgra)
                        return 0;
            }
            else {
                for (x = 0; x < a->xdim; x++)
                    if (a->bpal[a->bptr[dpos + x]].bgra !=
                        b->bpal[b->bptr[spos + x]].bgra)
                        return 0;
            }
        }

    return ~0;
}



/// String-oriented linear hash multiplier
#define SLH_MULT 0xFBC5
/// String-oriented linear hash shift
#define SLH_PLUS 0x11

#define HASH(hash, trgt) hash = SLH_PLUS + SLH_MULT * hash + trgt.bgra;
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



void FlushDest(TREE *elem, long fill) {
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



void TreeDelPath(TREE *elem) {
    FlushDest(elem, 0);
    free(elem->path);
}

void TreeDelAnim(TREE *elem) {
    FreeAnimStd(&elem->anim);
}

void TreeDel(TREE **root, ITER func) {
    if (*root) {
        TreeDel(&(*root)->next[0], func);
        TreeDel(&(*root)->next[1], func);
        TreeDel(&(*root)->next[2], func);
        if (func)
            func(*root);
        free(*root);
        *root = 0;
    }
}



TREE *TreeFind(TREE *root, uint64_t hash) {
    while (root && (root->hash != hash))
        root = root->next[(root->hash > hash)? 0 : 1];
    return root;
}



void TreeAdd(TREE **root, TREE *elem) {
    TREE *btop, *iter, *temp, *prev;
    long diff, indx;

    diff = indx = 0;
    temp = prev = 0;
    if (!(btop = iter = *root)) {
        *root = elem;
        return;
    }
    while (iter) {
        if (elem->hash == iter->hash) {
            elem->next[2] = iter->next[2];
            iter->next[2] = elem;
            return;          /// collision found; add collision and exit
        }
        if (iter->diff) {
            btop = iter;     /// ITER is unbalanced: potential turn site
            prev = temp;     /// saving ITER`s direct parent
            indx = diff = 0; /// purging node path, as it begins at BTOP
        }
        temp = iter;         /// max 2^32 nodes in the tree, see below
        diff |= (elem->hash > iter->hash)? 1 << indx : 0;
        iter = iter->next[(diff >> indx++) & 1];
    }
    /// add ELEM to the tree and rebalance all nodes from BTOP to ELEM
    iter = btop;
    temp->next[(diff >> --indx) & 1] = elem;
    while (iter != elem) {
        iter->diff += ((diff & 1) << 1) - 1;
        iter = iter->next[diff & 1];
        diff >>= 1;
    }
    if ((diff = (btop->diff < 0)? -1 : +1) << 1 == btop->diff) {
        /// absolute branch disbalance exceeds 1, turn needed
        indx = (diff + 1) >> 1;
        temp = btop->next[indx];
        if (temp->diff == diff) {
            /// small turn:  BTOP > TEMP  ~  TEMP v BTOP
            btop->next[indx] = temp->next[indx ^ 1];
            temp->next[indx ^ 1] = btop;
            temp->diff = btop->diff = 0;
            iter = temp;
        }
        else {
            /// big turn:  BTOP > TEMP v ITER  ~  (ITER v BTOP) > TEMP
            iter = temp->next[indx ^ 1];
            temp->next[indx ^ 1] = iter->next[indx];
            iter->next[indx] = temp;
            btop->next[indx] = iter->next[indx ^ 1];
            iter->next[indx ^ 1] = btop;
            temp->diff = (iter->diff == -diff)? -iter->diff : 0;
            btop->diff = (iter->diff == +diff)? -iter->diff : 0;
            iter->diff = 0;
        }
        /// *ROOT == BTOP: replacing the tree root
        /// *ROOT != BTOP: replacing BTOP`s site in its parent
        if (*root != btop)
            root = &prev->next[(prev->next[0] == btop)? 0 : 1];
        *root = iter;
    }
}



long TryUpdatePixTree(ENGD *engd, TREE *estr) {
    long turn, stat;
    TREE *epix;

    if (!estr)
        return 0;

    stat = ' ';
    if (!estr->epix->anim)
        TreeDel(&estr->epix, 0);
    else {
        /// searching for the appropriate animation
        epix = TreeFind(engd->hpix, estr->epix->hash);
        while (epix) {
            if (CompareAnimStd(epix->anim, estr->epix->anim,
                               epix->ainf.uuid ^ estr->turn))
                break;
            epix = epix->next[2];
        }
        if (!epix) {
            /// not found, the animation is new
            TreeAdd(&engd->hpix, estr->epix);
            estr->epix->ainf.uuid = (++engd->uniq << 2) | estr->turn;
        }
        else {
            /// found, replacing
            TreeDel(&estr->epix, TreeDelAnim);
            estr->epix = epix;
            stat = '#';
        }
    }
    if (!estr->epix)
        printf(TXL_FAIL" %s\n", estr->path);
    else {
        turn = estr->epix->ainf.uuid ^ estr->turn;
        printf("[%4ld%c%c%c] %s\n", estr->epix->ainf.uuid >> 2, (char)stat,
              (turn & 2)? 'D' : 'U', (turn & 1)? 'L' : 'R', estr->path);
    }
    return ~0;
}



void FillDest(TREE *root) {
    if (!root)
        return;

    FillDest(root->next[0]);
    FillDest(root->next[1]);
    FillDest(root->next[2]);
    FlushDest(root, ~0);
}



void UnitArrayFromTree(UNIT *uarr, TREE *root) {
    if (!root || !uarr)
        return;

    UnitArrayFromTree(uarr, root->next[0]);
    UnitArrayFromTree(uarr, root->next[1]);
    UnitArrayFromTree(uarr, root->next[2]);

    ASTD *anim = (ASTD*)root->anim;
    long iter = root->ainf.uuid >> 2;

    uarr[iter].anim = root->anim;
    uarr[iter].scal = root->scal;
    uarr[iter].tran = root->tran;
    uarr[iter].offs[0] = root->xoff;
    uarr[iter].offs[2] = root->yoff;
    uarr[iter].offs[1] = root->ainf.xdim - root->xoff
                       - (anim->xdim << root->scal);
    uarr[iter].offs[3] = root->ainf.ydim - root->yoff
                       - (anim->ydim << root->scal);
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



THR_FUNC cThrdFunc(void *user) {
    THRD *data = (THRD*)user;

    while (!0) {
        lWaitSemaphore(data->orig->isem, data->uuid);
        if (!data->loop)
            break;
        data->func(data);
        if (!lPickSemaphore(data->orig->isem, data->orig->osem, data->uuid))
            return THR_FAIL;
    }
    lPickSemaphore(data->orig->isem, data->orig->osem, data->uuid);
    return THR_EXIT;
}



int pixlcmp(const void *a, const void *b) {
    return (((BGRA*)a)->bgra  > ((BGRA*)b)->bgra)? 1 :
           (((BGRA*)a)->bgra == ((BGRA*)b)->bgra)? 0 : -1;
}

ASTD *ConvertAnim(AINF *asrc) {
    ASTD *retn = 0;
    uint64_t *temp;
    uint32_t iter, indx;

    if (!asrc || !asrc->xdim || !asrc->ydim
    ||  !asrc->fcnt || !asrc->time || !asrc->uuid)
        return 0;

    retn = calloc(1, sizeof(*retn));
    *retn = (ASTD){calloc(asrc->fcnt, asrc->xdim * asrc->ydim),
                   asrc->xdim, asrc->ydim, asrc->fcnt,
                   calloc(asrc->fcnt, sizeof(uint32_t)),
                   calloc(256, sizeof(BGRA))};
    memcpy(retn->time, asrc->time, asrc->fcnt * sizeof(uint32_t));
    temp = calloc(asrc->fcnt * sizeof(uint64_t), asrc->xdim * asrc->ydim);
    iter = asrc->fcnt * asrc->xdim * asrc->ydim;
    while (iter--)
        temp[iter] = ((uint64_t)iter << 32) | ((BGRA*)asrc->uuid)[iter].bgra;
    qsort(temp, asrc->fcnt * asrc->xdim * asrc->ydim,
          sizeof(uint64_t), pixlcmp);

    /// assuming that the picture contains no more than 256 colors.
    /// [TODO:] add dithering to fix this!

    indx = 0;
    iter = asrc->fcnt * asrc->xdim * asrc->ydim - 1;
    /// the first index is zero; we don`t explicitly set
    /// it, as retn->bptr is already filled with zeroes
    retn->bpal[indx].bgra = temp[iter];
    while (iter--) {
        if ((uint32_t)temp[iter + 1] != (uint32_t)temp[iter])
            retn->bpal[++indx].bgra = temp[iter];
        retn->bptr[temp[iter] >> 32] = indx;
    }
    free(temp);
    return retn;
}



void LTHR(THRD *data) {
    char *file;
    long  size;
    ASTD *retn;
    TREE *elem;

    if (!data->data)
        return;

    retn = 0;
    elem = data->elem;
    switch (data->flgs) {
        case ELA_AINF: retn = ConvertAnim((AINF*)data->data);      break;
        case ELA_LOAD: retn = MakeAnimStd(data->data, LONG_MAX);   break;
        case ELA_DISK: file = lLoadFile((char*)data->data, &size);
                       retn = MakeAnimStd(file, size); free(file); break;
    }
    if (retn) {
        elem->epix->ainf = (AINF){0, retn->xdim, retn->ydim,
                                     retn->fcnt, retn->time};
        elem->epix->scal = DownsampleAnimStd(retn, &elem->epix->xoff,
                                                   &elem->epix->yoff);
        elem->epix->hash = HashAnimStd(retn, &size);
        elem->turn = size & 3;
    }
    elem->epix->anim = retn;
}



void PTHR(THRD *data) {
    long iter, indx, x, y,
         ysrc, ydst, xmin, ymin, xmax, ymax,
         xoff, yoff, xinc, yinc, xpos, ypos;
    BGRA b_r_, _g_a, *bptr;
    UNIT *tail;
    ASTD *anim;

    bptr = data->orig->bptr;
    xoff = data->orig->dims.xdim;
    /// manual zeroing
//    memset(bptr + xoff * data->ymin, 0,
//           xoff * (data->ymax - data->ymin) << 2);

    for (iter = 0; iter < data->orig->size; iter++) {
        indx = data->orig->data[iter].w;
        tail = &data->orig->uarr[indx >> 2];
        xpos = data->orig->data[iter].x + tail->offs[indx & 1];
        ypos = data->orig->data[iter].y - tail->offs[3 - ((indx >> 1) & 1)];
        anim = tail->anim;
        yoff = anim->xdim * anim->ydim * data->orig->data[iter].z;
        ymax = (ypos > data->ymax)? data->ymax - ypos : 0;
        ymin = anim->ydim << tail->scal;
        xinc = anim->xdim << tail->scal;
        if (indx & 1) {
            xmax = (0 <        xpos       )? xinc : xinc + xpos;
            xmin = (0 > xinc + xpos - xoff)?    0 : xinc + xpos - xoff;
            yinc = -1;
        }
        else {
            xmax = (xinc < xoff - xpos)? xinc : xoff - xpos;
            xmin = (   0 >    0 - xpos)?    0 :    0 - xpos;
            yinc = 1;
        }
        xinc = ((yinc < 0)? xinc - 1 - xmin : xmin) + xpos;

        /// alpha blending here
        y = data->ymin - ypos;
        for (y = (y > -ymin)? y : -ymin; y < ymax; y++) {
            ydst = (y + ypos) * xoff + xinc;
            ysrc = (indx & 2)? -y - 1 : y + ymin;
            ysrc = (ysrc >> tail->scal) * anim->xdim + yoff;
            for (x = xmin; x < xmax; x++, ydst += yinc)
                if (bptr[ydst].chnl[3] != 0xFF) {
                    b_r_ = anim->bpal[anim->bptr[ysrc + (x >> tail->scal)]];
                    if (bptr[ydst].chnl[3] == 0x00)
                        bptr[ydst] = b_r_;
                    else {
                        _g_a.bgra = ((b_r_.bgra >> 8) & 0x00FF00FF)
                                  * (0xFF - bptr[ydst].chnl[3]);
                        b_r_.bgra = ((b_r_.bgra     ) & 0x00FF00FF)
                                  * (0xFF - bptr[ydst].chnl[3]);
                        bptr[ydst].bgra += ((b_r_.bgra >> 8) & 0x00FF00FF)
                                        +  ((_g_a.bgra     ) & 0xFF00FF00);
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
        lWaitSemaphore(engd->osem, SEM_FULL);
        for (iter = 0; iter < engd->ncpu; iter++)
            engd->thrd[iter].loop = 0;
        lPickSemaphore(engd->osem, engd->isem, SEM_FULL);
        lWaitSemaphore(engd->osem, SEM_FULL);
    }
}



long SwitchThreads(ENGD *engd, long draw) {
    long iter, temp;

    if (draw) {
        for (iter = 0; iter < engd->ncpu; iter++)
            if (engd->thrd[iter].loop)
                return -1;

        temp = (engd->dims.ydim / engd->ncpu) + 1;
        for (iter = 0; iter < engd->ncpu; iter++) {
            engd->thrd[iter] = (THRD){1, 1 << iter, engd, 0, PTHR,
                                    {{temp * iter, temp * (iter + 1)}}};
            lMakeThread(&engd->thrd[iter]);
        }
        engd->thrd[engd->ncpu - 1].ymax = engd->dims.ydim;
    }
    else
        for (iter = 0; iter < engd->ncpu; iter++) {
            engd->thrd[iter] = (THRD){1, 1 << iter, engd, 0, LTHR};
            lMakeThread(&engd->thrd[iter]);
        }
    return 0;
}



uint32_t cPrepareFrame(ENGD *engd, long xptr, long yptr, uint32_t attr) {
    uint64_t time = lTimeFunc();
    long pick;

    if (time - engd->tfrm < engd->msec)
        return PFR_HALT | PFR_SKIP;
    engd->tfrm = time;
    if (~engd->flgs & COM_DRAW)
        return PFR_HALT;
    pick = SelectUnit(engd->uarr, engd->data, engd->size, xptr, yptr);
    engd->smax = 0;
    engd->size = engd->ufrm(engd, &engd->data, &engd->smax, lTimeFunc(),
                            engd->udat, attr, xptr, yptr, pick);
    if (!engd->smax) {
        cEngineCallback(engd, ECB_QUIT, ~0);
        return PFR_HALT;
    }
    return ((pick >= 0) || (engd->flgs & COM_OPAQ))? PFR_PICK : 0;
}



void cOutputFrame(ENGD *engd, long frbo) {
    if (engd->flgs & COM_RGPU) {
        if (!MakeRendererOGL(&engd->rndr, !!frbo && !(engd->flgs & WIN_IBGR),
                              engd->uarr, engd->uniq, engd->smax,
                              engd->dims.xdim - engd->dims.xpos,
                              engd->dims.ydim - engd->dims.ypos)) {
            cEngineCallback(engd, ECB_SFLG, engd->flgs & ~COM_RGPU);
            return;
        }
        if (!!frbo) {
            if (!engd->surf)
                engd->surf = MakeRBO(engd->dims.xdim - engd->dims.xpos,
                                     engd->dims.ydim - engd->dims.ypos);
            ReadRBO(engd->surf, engd->bptr, engd->flgs);
            BindRBO(engd->surf, 1);
        }
        DrawRendererOGL(engd->rndr, engd->uarr, engd->data,
                        engd->size, engd->flgs & COM_OPAQ);
        if (!!frbo)
            BindRBO(engd->surf, 0);
    }
    else {
        SwitchThreads(engd, 1);
        lPickSemaphore(engd->osem, engd->isem, SEM_FULL);
        lWaitSemaphore(engd->osem, SEM_FULL);
    }
    engd->fram++;
}



void cDeallocFrame(ENGD *engd, long frbo) {
    if (~engd->flgs & COM_RGPU)
        StopThreads(engd);
    else {
        FreeRendererOGL(&engd->rndr);
        if (!!frbo)
            FreeRBO(&engd->surf);
    }
}



void cEngineLoadAnimAsync(ENGD *engd, AINF *ainf, uint8_t *name,
                          void *data, uint32_t flgs, void (*udis)(void*)) {
    TREE *estr, *retn;
    DEST *dest;

    SEM_TYPE curr;
    uint64_t hash;

    if (!engd || !ainf || !name)
        return;
    estr = TreeFind(engd->hstr, hash = HashLine64((char*)name));
    while (estr) {
        if (!strcmp(estr->path, (char*)name))
            break;
        estr = estr->next[2];
    }
    if (!estr) {
        estr = calloc(1, sizeof(*estr));
        estr->hash = hash;
        estr->path = strdup((char*)name);
        estr->epix = calloc(1, sizeof(*estr->epix));
        estr->dest = calloc(1, sizeof(*estr->dest));
        estr->dest->ainf = ainf;
        TreeAdd(&engd->hstr, estr);
        curr = cFindBit(lWaitSemaphore(engd->osem, SEM_NULL));
        retn = engd->thrd[curr].elem;
        engd->thrd[curr].elem = estr;
        if (engd->thrd[curr].udis)
            engd->thrd[curr].udis(engd->thrd[curr].data);
        engd->thrd[curr].udis = udis;
        engd->thrd[curr].data = data;
        engd->thrd[curr].flgs = flgs;
        lPickSemaphore(engd->osem, engd->isem, 1 << curr);
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



void cEngineRunMainLoop(ENGD *engd, int32_t xpos, int32_t ypos,
                        uint32_t xdim, uint32_t ydim, uint32_t flgs,
                        uint32_t msec, intptr_t user, UFRM ufrm, UFLG uflg) {
    long mtmp;

    if (engd->uarr) {
        engd->dims = (T4IV){{xpos, ypos, xdim, ydim}};
        engd->dflg = engd->flgs = flgs;
        engd->ufrm = ufrm;
        engd->udat = user;
        engd->msec = msec;
        engd->size = 0;

        mtmp = lTimeFunc() - engd->tfrm;
        printf(TXL_AEND" %u threads, %u objects, %ld ms: %0.3f ms/obj\n",
               engd->ncpu, engd->uniq, mtmp,
              (double)mtmp * engd->ncpu / engd->uniq);
        do {
            engd->flgs = (engd->flgs & ~COM_DDDD) | (engd->dflg & COM_DDDD);
            /// done setting deferred flags (e.g., ones like rendering scheme)
            if (uflg)
                engd->flgs = uflg(engd, engd->udat, engd->flgs);
            printf("%s\n", (engd->flgs & COM_RGPU)? TXL_RGPU : TXL_RSTD);
            lRunMainLoop(engd, engd->dims.xpos, engd->dims.ypos,
                         engd->dims.xdim, engd->dims.ydim,
                        &engd->bptr, engd->user, engd->flgs);
        } while (~engd->iflg & IFL_HALT);
    }
    else
        printf(TXL_FAIL" No animation base found! Exiting...\n");
}



void cEngineCallback(ENGD *engd, uint32_t ecba, intptr_t data) {
    if (!engd && (ecba != ECB_INIT))
        return;
    switch (ecba) {
        case ECB_INIT:
            engd = calloc(1, sizeof(*engd));
            engd->ncpu = lCountCPUs();
            engd->thrd = malloc(engd->ncpu * sizeof(*engd->thrd));
            lMakeSemaphore(&engd->isem, engd->ncpu, SEM_NULL);
            lMakeSemaphore(&engd->osem, engd->ncpu, SEM_FULL);
            SwitchThreads(engd, 0);
            engd->tfrm = lTimeFunc();
            *(intptr_t*)data = (intptr_t)engd;
            break;

        case ECB_GUSR:
            if (data)
                *(intptr_t**)data = engd->user;
            break;

        case ECB_GFLG:
            if (data)
                *(uint32_t*)data = engd->flgs;
            break;

        case ECB_SFLG: {
            uint32_t temp = engd->flgs;

            if ((data ^ temp) & COM_DDDD) {
                /// we have received a deferred flag, restart needed!
                engd->dflg = data;
                engd->iflg &= ~IFL_HALT; /// do not halt on restart!
                lRestartEngine(engd);
            }
            /// deferred flags are not to be set until restart!
            engd->flgs = (engd->flgs & COM_DDDD) | (data & ~COM_DDDD);
            lShowMainWindow(engd, engd->flgs & COM_SHOW);
            break;
        }
        /// [TODO:] change to PTHR()
        case ECB_DRAW: {
            AINF *ainf = (AINF*)data;
            BGRA *retn = (BGRA*)ainf->time, pixl;
            ASTD *anim = engd->uarr[ainf->uuid >> 2].anim;
            long x, y, xcoe, ycoe, xdim = ainf->xdim, ydim = ainf->ydim;
            uint8_t *bptr;

            if ((!anim) || (ainf->fcnt >= anim->fcnt)
            || (anim->xdim > xdim) || (anim->ydim > ydim))
                break;
            xcoe = xdim / anim->xdim;
            ycoe = ydim / anim->ydim;
            xcoe = ycoe = (xcoe < ycoe)? xcoe : ycoe;
            retn += ((ydim - ycoe * anim->ydim) >> 1) * xdim
                 +  ((xdim - xcoe * anim->xdim) >> 1);
            bptr = anim->bptr + anim->xdim * anim->ydim * ainf->fcnt;
            for (y = anim->ydim * ycoe - 1; y >= 0; y--)
                for (x = anim->xdim * xcoe - 1; x >= 0; x--) {
                    /// division, OMFG! [TODO:] get rid of this
                    pixl = anim->bpal[bptr[anim->xdim * (y / ycoe)
                                                      + (x / xcoe)]];
                    if (pixl.chnl[3] != 0x00)
                        retn[xdim * y + x] = pixl;
                }
            break;
        }
        case ECB_TEST: {
            AINF *ainf = (AINF*)data;
            T4FV test = {{-(int32_t)ainf->xdim, -(int32_t)ainf->ydim,
                          ainf->fcnt, ainf->uuid}};

            ainf->fcnt = SelectUnit(engd->uarr, &test, 1, 0, 0) + 1;
            break;
        }
        case ECB_LOAD:
            StopThreads(engd);
            if (data) {
                SwitchThreads(engd, 0);
                engd->tfrm = lTimeFunc();
            }
            else {
                data = engd->flgs;
                engd->flgs &= ~COM_DRAW;
                for (ecba = 0; ecba < engd->ncpu; ecba++) {
                    TryUpdatePixTree(engd, engd->thrd[ecba].elem);
                    if (engd->thrd[ecba].udis)
                        engd->thrd[ecba].udis(engd->thrd[ecba].data);
                    engd->thrd[ecba].udis = 0;
                    engd->thrd[ecba].data = 0;
                }
                if (engd->hstr) {
                    FillDest(engd->hstr);
                    free(engd->uarr);
                    engd->uarr = calloc(engd->uniq + 1, sizeof(*engd->uarr));
                    UnitArrayFromTree(engd->uarr, engd->hpix);
                }
                if (engd->uarr)
                    engd->flgs = data;
            }
            break;

        case ECB_QUIT:
            engd->iflg |= IFL_HALT;
            lRestartEngine(engd);
            if (data)
                break;
            if (engd->uarr) {
                free(engd->uarr);
                engd->uarr = 0;
                TreeDel(&engd->hstr, TreeDelPath);
                TreeDel(&engd->hpix, TreeDelAnim);
                engd->uniq = 0;
            }
            lFreeSemaphore(&engd->isem, engd->ncpu);
            lFreeSemaphore(&engd->osem, engd->ncpu);
            free(engd->thrd);
            free(engd);
            break;
    }
}
