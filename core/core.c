#include "core.h"



/// Default config file
#define DEF_CONF "anim.conf"
/// Default comment character
#define DEF_CMNT '#'
/// Default token separator
#define DEF_TSEP ','

/// Random number generator modulo
#define RNG_TRIM 0xFFFFFFFB
/// Random number generator multiplier
#define RNG_MULT 0x10A860C1
/// Random number generator shift
#define RNG_PLUS 0x01

/// String-oriented linear hash multiplier
#define SLH_MULT 0xFBC5
/// String-oriented linear hash shift
#define SLH_PLUS 0x11

/// [not recognized / comment / invalid token]
#define AVT_NONE 0x00000000
/// 'Name'
#define AVT_NAME 0xC6961EB1
/// 'Effect'
#define AVT_EFCT 0xE708AEFF
/// 'Behavior'
#define AVT_BHVR 0xB6AB6234
/// 'behaviorgroup'
#define AVT_BGRP 0xA40004B2
/// 'Categories'
#define AVT_CTGS 0xC831B868
/// 'Speak'
#define AVT_PHRS 0x90E5E31D



uint32_t seed;



uint32_t PRNG(uint32_t *seed) {
    return *seed = RNG_PLUS + (*seed * RNG_MULT) % RNG_TRIM;
}



char *SkipCharUTF8(char *line) {
    static char skip[] = {2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 5, 6};
    return line + ((*line & 0x80)? skip[((*line) >> 2) & 0x0F] : 1);
}



long WhitespaceUTF8(char *line) {
    switch (line[0]) {
        case '\x09':
        case '\x20':
            return ~0;
        case '\xC2':
            if (line[1] == '\xA0') return ~0; break;
        case '\xE1':
            if (line[1] == '\x9A' && line[2] == '\x80') return ~0; break;
        case '\xE3':
            if (line[1] == '\x80' && line[2] == '\x80') return ~0; break;
        case '\xEF':
            if (line[1] == '\xBB' && line[2] == '\xBF') return ~0; break;
        case '\xE2':
            switch (line[1]) {
                case '\x81': if (line[2] == '\x9F') return ~0; break;
                case '\x80':
                    switch (line[2]) {
                        case '\x80':
                        case '\x81':
                        case '\x82':
                        case '\x83':
                        case '\x84':
                        case '\x85':
                        case '\x86':
                        case '\x87':
                        case '\x88':
                        case '\x89':
                        case '\x8A':
                        case '\x8B':
                        case '\xAF': return ~0;
                    }
            }
    }
    return 0;
}



uint32_t HashLine(char *line) {
    uint32_t hash = 0;

    while (*line)
        hash = SLH_PLUS + SLH_MULT * hash + *line++;
    return hash;
}



char *SplitLine(char **tail, char tsep) {
    char *retn, *temp, *iter = *tail;

    if (*tail) {
        while (WhitespaceUTF8(iter))
            iter = SkipCharUTF8(iter);
        if (*iter) {
            if (!(*tail = strchr(iter, tsep)))
                *tail = iter + strlen(iter);
            temp = retn = iter;
            do {
                if (!WhitespaceUTF8(iter))
                    temp = iter;
                iter = SkipCharUTF8(iter);
            } while (iter < *tail);
            *tail = (**tail)? *tail + 1 : NULL;
            *++temp = '\0';
            return retn;
        }
    }
    *tail = NULL;
    return NULL;
}



uint32_t DetermineType(char **tail) {
    char *temp = SplitLine(tail, DEF_TSEP);
    if (temp && (*temp != DEF_CMNT))
        return HashLine(temp);
    return AVT_NONE;
}



char *ConcatPath(char *base, char *path) {
    char *retn;
    long  iter;

    if (*path == '"')
        path++;
    retn = malloc(strlen(base) + strlen(path) + 2);
    iter = sprintf(retn, "%s/%s", base, path) - 1;
    if (retn[iter] == '"')
        retn[iter] = '\0';
    return retn;
}



char *GetNextLine(char **file) {
    char *retn;
    long  iter;

    if ((retn = SplitLine(file, '\n')))
        if ((iter = strlen(retn)) > 0)
            if (retn[iter - 1] == '\r')
                retn[iter - 1] = '\0';
    return retn;
}



UNIT *SortByY(UNIT **tail) {
    long ymin, ymax, ytmp;
    UNIT *curr, *retn;

    static struct {
        UNIT *lbgn, *lend;
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



UNIT *UpdateFrameStd(UNIT **tail, UNIT **pick, ulong *time, VEC2 cptr) {
    long xpos, ypos, ytmp;
    UNIT *iter, *temp;
    ASTD *anim;

    if (*tail) {
        iter = temp = *tail;
        if (*pick == EMP_PICK) {
            while (temp) {
                anim = temp->anim;
                ytmp = temp->cpos.y - (anim->ydim << temp->scal);
                xpos = (cptr.x - temp->cpos.x) >> temp->scal;
                ypos = (cptr.y - ytmp) >> temp->scal;
                if (temp->flgs & UCF_REVX)
                    xpos = anim->xdim - 1 - xpos;
                if (temp->flgs & UCF_REVY)
                    ypos = anim->ydim - 1 - ypos;
                if ((cptr.x <  temp->cpos.x + (anim->xdim << temp->scal))
                &&  (cptr.x >= temp->cpos.x)
                &&  (cptr.y <  temp->cpos.y)
                &&  (cptr.y >= ytmp)

                &&  (anim->bptr.indx[xpos + anim->xdim *
                                    (ypos + anim->ydim * temp->fcur)] < 255)) {
//                &&  (anim->bptr.bgra[xpos + anim->xdim *
//                                    (ypos + anim->ydim * temp->fcur)].A)) {

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
        while (iter) {
            anim = iter->anim;
            if ((*time - iter->time) > anim->time[iter->fcur]) {
                iter->time = *time;
                if (++iter->fcur >= anim->fcnt)
                    iter->fcur = 0;
            }
            iter = iter->prev;
        }
        if (*pick && (*pick != EMP_PICK)) {
            (*pick)->cpos.x = cptr.x - (*pick)->cptr.x;
            (*pick)->cpos.y = cptr.y - (*pick)->cptr.y;
            SortByY(tail);
        }
    }
    return *tail;
}



void DrawPixStdThrd(DRAW *draw) {
    long x, y, xmin, ymin, xmax, ymax, xoff, yoff, ysrc, ydst, yinc;
    BGRA b_r_, _g_a, *bptr;
    UNIT *tail;
    ASTD *anim;

    tail = draw->tail;
    while (tail) {
        anim = tail->anim;

        xoff = draw->pict->size.x;
        yoff = tail->cpos.y - (anim->ydim << tail->scal);
        xmin = max(0, -tail->cpos.x);
        ymin = max(0, draw->ymin - yoff);
        xmax = min(anim->xdim << tail->scal, xoff - tail->cpos.x);
        ymax = min(anim->ydim << tail->scal, draw->ymax - yoff);
        yoff = anim->xdim * anim->ydim * tail->fcur;
        yinc = (tail->flgs & UCF_REVX)? -1 : 1;
        bptr = draw->pict->bptr;

        /// alpha blending here
        for (y = ymin, ymin = (anim->ydim << tail->scal) - 1; y < ymax; y++) {
            ydst = (y + tail->cpos.y - (anim->ydim << tail->scal))
                 * xoff + tail->cpos.x + xmin + ((yinc < 0)? xmax - 1 : 0);
            ysrc = (tail->flgs & UCF_REVY)? ymin - y : y;
            ysrc = (ysrc >> tail->scal) * anim->xdim + yoff;
            for (x = xmin; x < xmax; x++, ydst += yinc) {
                if (bptr[ydst].A == 0xFF)
                    continue;

                b_r_ = anim->bpal[anim->bptr.indx[ysrc + (x >> tail->scal)]];
//                b_r_ = anim->bptr.bgra[ysrc + (x >> tail->scal)];

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
        tail = tail->prev;
    }
}



long MakeUnitStd(UNIT **tail, UNIT *info) {
    #pragma pack(push, 1)
    struct {
        RGBX csrc;
        uint8_t tran;
        RGBX cdst;
    } *amap;
    #pragma pack(pop)

    char *apal, *file;
    long  indx;
    BGRA *bpal;
    UNIT *prev;

    if (!(*tail)) {
        (*tail) = malloc(sizeof(**tail));
        (*tail)->prev = NULL;
    }
    else {
        (*tail)->next = malloc(sizeof(**tail));
        (*tail)->next->prev = (*tail);
        (*tail) = (*tail)->next;
    }
    prev = (*tail)->prev;
    *(*tail) = *info;
    (*tail)->prev = prev;
    prev = (*tail);

    prev->hash = HashLine(prev->path);
    info = prev->prev;
    while (info) {
        if (info->hash == prev->hash)
            break;
        info = info->prev;
    }
    if (info && strcmp(info->path, prev->path))
        info = NULL;
    if (info) {
        free(prev->path);
        prev->anim = info->anim;
        prev->path = info->path;
        prev->flgs |= UCF_COPY;
    }
    else {
        prev->anim = MakeAnimStd(prev->path, ~0);
        prev->flgs &= ~UCF_COPY;
    }

    if (prev->anim) {
//        if (prev->flgs & UCF_CANX)
            prev->flgs = (prev->flgs & ~UCF_REVX) | (PRNG(&seed) & UCF_REVX);
//        if (prev->flgs & UCF_CANY)
            prev->flgs = (prev->flgs & ~UCF_REVY) | (PRNG(&seed) & UCF_REVY);
        prev->cpos.x = PRNG(&seed) % (prev->cpos.x
                     - (((ASTD*)prev->anim)->xdim << prev->scal));
        prev->cpos.y = PRNG(&seed) % (prev->cpos.y
                     - (((ASTD*)prev->anim)->ydim << prev->scal))
                     + (((ASTD*)prev->anim)->ydim << prev->scal);
        prev->fcur = PRNG(&seed) % ((ASTD*)prev->anim)->fcnt;
        if ((bpal = ((ASTD*)prev->anim)->bpal)) {
            apal = strdup(prev->path);
            indx = strlen(apal);
            apal[indx - 3] = 'a';
            apal[indx - 2] = 'r';
            apal[indx - 1] = 't';
            if (bpal && (file = LoadFile(apal, &indx))) {
                free(apal);
                for (apal  = file + indx -  sizeof(*amap);
                     apal >= file;  apal -= sizeof(*amap))
                    for (amap = (typeof(amap))apal,
                         indx = 0; indx < 256; indx++)
                        if ((bpal[indx].R == amap->csrc.R)
                        &&  (bpal[indx].G == amap->csrc.G)
                        &&  (bpal[indx].B == amap->csrc.B)) {
                             bpal[indx].R = ((long)amap->cdst.R
                                          * amap->tran) >> 8;
                             bpal[indx].G = ((long)amap->cdst.G
                                          * amap->tran) >> 8;
                             bpal[indx].B = ((long)amap->cdst.B
                                          * amap->tran) >> 8;
                             bpal[indx].A = amap->tran;
                             break;
                        }
                free(file);
                apal = NULL;
            }
            free(apal);
        }
        return ~0;
    }
    if ((*tail)->prev) {
        (*tail) = (*tail)->prev;
        free((*tail)->next);
    }
    else {
        free(*tail);
        *tail = NULL;
    }
    return 0;
}



void FillLibStdThrd(FILL *fill) {
    char *file, *fptr, *conf;
    UNIT *tail,  info;

    tail = NULL;
    fill->curr = 0;
    info.cpos = fill->scrn;
    info.ulib = fill->ulib;
    conf = ConcatPath(fill->ulib->path, DEF_CONF);
    if ((file = fptr = LoadFile(conf, NULL))) {
        free(conf);
        while ((conf = GetNextLine(&fptr))) {
            switch (DetermineType(&conf)) {
                case AVT_BHVR:
                    SplitLine(&conf, DEF_TSEP);
                    SplitLine(&conf, DEF_TSEP);
                    SplitLine(&conf, DEF_TSEP);
                    SplitLine(&conf, DEF_TSEP);
                    SplitLine(&conf, DEF_TSEP);
                    info.path = ConcatPath(fill->ulib->path,
                                           SplitLine(&conf, DEF_TSEP));
                    SplitLine(&conf, DEF_TSEP);
                    info.flgs = strtol(SplitLine(&conf, DEF_TSEP), NULL, 16);
                    info.scal = (info.flgs >> 24) & 3;
                    if (MakeUnitStd(&tail, &info)) {
                        printf("[%c] %s\n",
                              (tail->flgs & UCF_COPY)? 'C' : ' ', tail->path);
                        fill->curr++;
                    }
                    else {
                        printf("[!] %s\n", info.path);
                        free(info.path);
                    }
                    break;

                case AVT_EFCT:
                    SplitLine(&conf, DEF_TSEP);
                    SplitLine(&conf, DEF_TSEP);
                    info.path = ConcatPath(fill->ulib->path,
                                           SplitLine(&conf, DEF_TSEP));
                    info.scal = 0;
                    if (MakeUnitStd(&tail, &info)) {
                        printf("[%c] %s\n",
                              (tail->flgs & UCF_COPY)? 'C' : ' ', tail->path);
                        fill->curr++;
                    }
                    else {
                        printf("[!] %s\n", info.path);
                        free(info.path);
                    }
                    break;
            }
        }
        if (tail) {
            fill->load += fill->curr;
            printf("--- %s: %ld objects\n\n",
                   fill->ulib->path, fill->curr);
            tail->next = NULL;
            fill->ulib->uses = 25;
            fill->ulib->ucnt = fill->curr;
            fill->ulib->uarr = malloc(fill->curr * sizeof(*fill->ulib->uarr));
            for (fill->curr--; fill->curr >= 0; fill->curr--) {
                fill->ulib->uarr[fill->curr] = tail;
                tail = tail->prev;
            }
        }
        free(file);
        conf = NULL;
    }
    free(conf);
}



void MakeEmptyLib(ULIB **head, char *base, char *path) {
    if (!*head) {
         *head = malloc(sizeof(**head));
        (*head)->next = NULL;
    }
    else {
        (*head)->prev = malloc(sizeof(**head));
        (*head)->prev->next = *head;
        (*head) = (*head)->prev;
    }
    (*head)->uses = 0;
    (*head)->prev = NULL;
    (*head)->uarr = NULL;
    (*head)->path = ConcatPath(base, path);
}



void FreeUnitList(UNIT **tail, void (*adel)(void**)) {
    UNIT *iter = *tail;

    while (iter) {
        if (adel && !(iter->flgs & UCF_COPY)) {
            adel(&iter->anim);
            free(iter->path);
        }
        if (iter->prev) {
            iter = iter->prev;
            free(iter->next);
        }
        else {
            free(iter);
            iter = NULL;
        }
    }
    (*tail) = NULL;
}



void FreeLibList(ULIB **list, void (*adel)(void**)) {
    ULIB *iter = *list;

    while (iter) {
        if (iter->uarr) {
            FreeUnitList(&iter->uarr[iter->ucnt - 1], adel);
            free(iter->uarr);
        }
        free(iter->path);
        if (iter->next) {
            iter = iter->next;
            free(iter->prev);
        }
        else {
            free(iter);
            iter = NULL;
        }
    }
    (*list) = NULL;
}



void UnitListFromLib(ULIB *ulib, UNIT **tail) {
    UNIT *elem, *list = NULL;
    ulong iter;

    while (ulib) {
        for (iter = 0; iter < ulib->uses; iter++) {
            elem = malloc(sizeof(*list));
            *elem = *ulib->uarr[PRNG(&seed) % ulib->ucnt];
            elem->prev = list;
            if (list)
                list->next = elem;
            list = elem;
        }
        ulib = ulib->next;
    }
    if (list) {
        list->next = NULL;
        SortByY(&list);
        FreeUnitList(tail, NULL);
        *tail = list;
    }
}
