#include "core.h"
#include <string.h>

#define max(a, b) (((a) > (b))? (a) : (b))
#define min(a, b) (((a) < (b))? (a) : (b))



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
#define AVT_CATS 0xC831B868
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
    uint32_t hash;
    char *temp;

    temp = SplitLine(tail, DEF_TSEP);
    if (temp && (*temp != DEF_CMNT)) {
        hash = 0;
        do {
            hash = SLH_PLUS + SLH_MULT * hash + *temp++;
        } while (*temp);
        return hash;
    }
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



char *LoadConfig(char *conf) {
    char *retn = NULL;
    long file, size;

    if ((file = open(conf, O_RDONLY)) > 0) {
        size = lseek(file, 0, SEEK_END);
        lseek(file, 0, SEEK_SET);
        retn = malloc(size + 1);
        read(file, retn, size);
        retn[size] = '\0';
        close(file);
    }
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
    UNIT *iter, *temp;
    ASTD *anim;
    long ytmp;

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
                     anim->ydim * temp->fcur)] != 0xFF)) {
//                &&  (anim->bptr.bgra[((cptr.x - temp->cpos.x) >> temp->scal) +
//                     anim->xdim * (((cptr.y - ytmp) >> temp->scal) +
//                     anim->ydim * temp->fcur)].A)) {

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
    long x, y, xmin, ymin, xmax, ymax, xoff, yoff, ysrc, ydst;
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
        bptr = draw->pict->bptr;

        /// alpha blending here
        for (y = ymin; y < ymax; y++) {
            ydst = (y + tail->cpos.y - (anim->ydim << tail->scal))
                 * xoff + tail->cpos.x + xmin;
            ysrc = (y >> tail->scal) * anim->xdim + yoff;
            for (x = xmin; x < xmax; x++, ydst++) {
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
                    bptr[ydst].BGRA += (0x00FF00FF & (b_r_.BGRA >> 8))
                                    +  (0xFF00FF00 & (_g_a.BGRA     ));
                }
            }
        }
        tail = tail->prev;
    }
}



void FillLibStdThrd(FILL *fill) {
    #define anih ((ASTD*)tail->anim)
    char *file, *fptr, *conf;
    UNIT *tail;

    tail = NULL;
    fill->curr = 0;
    conf = ConcatPath(fill->ulib->path, DEF_CONF);
    if ((file = fptr = LoadConfig(conf))) {
        free(conf);
        while ((conf = GetNextLine(&fptr))) {
            switch (DetermineType(&conf)) {
                case AVT_BHVR:
                    if (!tail) {
                        tail = malloc(sizeof(*tail));
                        tail->prev = NULL;
                    }
                    else {
                        tail->next = malloc(sizeof(*tail));
                        tail->next->prev = tail;
                        tail = tail->next;
                    }
                    SplitLine(&conf, DEF_TSEP);
                    SplitLine(&conf, DEF_TSEP);
                    SplitLine(&conf, DEF_TSEP);
                    SplitLine(&conf, DEF_TSEP);
                    SplitLine(&conf, DEF_TSEP);
                    tail->path = ConcatPath(fill->ulib->path,
                                            SplitLine(&conf, DEF_TSEP));
                    tail->anim = NULL;
                    if ((tail->anim = MakeAnimStd(tail->path, ~0))) {
                        printf("DONE:\t%s\n", tail->path);
                        tail->scal = 1;
                        tail->cpos.x = PRNG(&seed) % (fill->scrn.x
                                     - (anih->xdim << tail->scal));
                        tail->cpos.y = PRNG(&seed) % (fill->scrn.y
                                     - (anih->ydim << tail->scal))
                                     + (anih->ydim << tail->scal);
                        tail->fcur = PRNG(&seed) % anih->fcnt;
                        tail->ulib = fill->ulib;
                        fill->curr++;
                    }
                    else {
                        printf("FAILED:\t%s\n", tail->path);
                        free(tail->path);
                        if (tail->prev) {
                            tail = tail->prev;
                            free(tail->next);
                        }
                        else {
                            free(tail);
                            tail = NULL;
                        }
                    }
                    break;
            }
        }
        if (tail) {
            fill->load += fill->curr;
            printf(" --- %ld objects loaded!\n\n", fill->curr);
            tail->next = NULL;
            fill->ulib->uses = 5;
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
    #undef anih
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
    (*head)->prev = NULL;
    (*head)->uarr = NULL;
    (*head)->uses = 0;
    (*head)->path = ConcatPath(base, path);
}



void FreeUnitList(UNIT **tail, void (*adel)(void**)) {
    UNIT *iter = *tail;

    while (iter) {
        if (adel) {
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
