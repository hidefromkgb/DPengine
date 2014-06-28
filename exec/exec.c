#include "exec.h"



UNIT **uarr = 0, *tail = 0, *grab = 0;
uint32_t seed = 0;
long pflg = 0;



char *LoadFile(char *name, long *size) {
    char *retn = 0;
    long file, flen;

    if ((file = open(name, O_RDONLY | O_BINARY)) > 0) {
        flen = lseek(file, 0, SEEK_END);
        lseek(file, 0, SEEK_SET);
        retn = malloc(flen + 1);
        read(file, retn, flen);
        retn[flen] = '\0';
        close(file);
        if (size)
            *size = flen;
    }
    return retn;
}



/// Random number generator modulo
#define RNG_TRIM 0xFFFFFFFB
/// Random number generator multiplier
#define RNG_MULT 0x10A860C1
/// Random number generator shift
#define RNG_PLUS 0x01

uint32_t PRNG() {
    return seed = RNG_PLUS + (seed * RNG_MULT) % RNG_TRIM;
}



/// String-oriented linear hash multiplier
#define SLH_MULT 0xFBC5
/// String-oriented linear hash shift
#define SLH_PLUS 0x11

uint32_t HashLine(char *line) {
    uint32_t hash = 0;

    while (*line)
        hash = SLH_PLUS + SLH_MULT * hash + *line++;
    return hash;
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
            while (iter < *tail) {
                if (!WhitespaceUTF8(iter))
                    temp = iter;
                iter = SkipCharUTF8(iter);
            }
            *tail = (**tail)? *tail + 1 : 0;
            if (*temp) {
                if (*temp != tsep)
                    temp++;
                *temp = '\0';
            }
            return retn;
        }
    }
    return *tail = 0;
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



void MakeEmptyLib(ULIB **head, char *base, char *path) {
    if (!*head)
         *head = calloc(1, sizeof(**head));
    else {
        (*head)->prev = calloc(1, sizeof(**head));
        (*head)->prev->next = *head;
        (*head) = (*head)->prev;
    }
    (*head)->path = ConcatPath(base, path);
}



void MakeEmptyUnit(UNIT **tail) {
    if (!*tail)
         *tail = calloc(1, sizeof(**tail));
    else {
        (*tail)->next = calloc(1, sizeof(**tail));
        (*tail)->next->prev = *tail;
        (*tail) = (*tail)->next;
    }
}



void FreeUnitList(UNIT **tail) {
    UNIT *iter = *tail;

    (*tail) = 0;
    while (iter) {
        if (iter->prev) {
            iter = iter->prev;
            free(iter->next);
        }
        else {
            free(iter);
            iter = 0;
        }
    }
}



void FreeLibList(ULIB **head) {
    ULIB *iter = *head;
    long indx;

    (*head) = 0;
    while (iter) {
        if (iter->uarr) {
            for (indx = 0; indx < iter->ucnt; indx++) {
                free(iter->uarr[indx]->path);
                free(iter->uarr[indx]);
            }
            free(iter->uarr);
        }
        free(iter->path);
        if (iter->next) {
            iter = iter->next;
            free(iter->prev);
        }
        else {
            free(iter);
            iter = 0;
        }
    }
}



void FillLib(ULIB *ulib, char *pcnf, LOAD load) {
    char *file, *fptr, *conf;
    long  ucnt = 0;
    UNIT *tail = 0;

    conf = ConcatPath(ulib->path, pcnf);
    if ((file = fptr = LoadFile(conf, 0))) {
        free(conf);
        while ((conf = GetNextLine(&fptr)))
            switch (DetermineType(&conf)) {
                case AVT_BHVR:
                    SplitLine(&conf, DEF_TSEP);
                    SplitLine(&conf, DEF_TSEP);
                    SplitLine(&conf, DEF_TSEP);
                case AVT_EFCT:
                    SplitLine(&conf, DEF_TSEP);
                    SplitLine(&conf, DEF_TSEP);

                    MakeEmptyUnit(&tail);
                    tail->path =
                        strdup(ConcatPath(ulib->path,
                                          SplitLine(&conf, DEF_TSEP)));
                    load((uint8_t*)tail->path, &tail->uuid,
                         &tail->xdim, &tail->ydim, &tail->fcnt, &tail->time);

                    MakeEmptyUnit(&tail);
                    tail->path =
                        strdup(ConcatPath(ulib->path,
                                          SplitLine(&conf, DEF_TSEP)));
                    load((uint8_t*)tail->path, &tail->uuid,
                         &tail->xdim, &tail->ydim, &tail->fcnt, &tail->time);

                    ucnt += 2;
                    break;
            }

        if (tail) {
            tail->next = 0;
            ulib->ucnt = ucnt;
            ulib->uarr = malloc(ucnt * sizeof(*ulib->uarr));
            for (ucnt--; ucnt >= 0; ucnt--) {
                ulib->uarr[ucnt] = tail;
                tail = tail->prev;
                ulib->uarr[ucnt]->prev = ulib->uarr[ucnt]->next = 0;
            }
        }
        free(file);
        conf = 0;
    }
    free(conf);
}



UNIT *SortByY(UNIT **tail) {
    static struct {
        UNIT *lbgn, *lend;
    } elem[0x2000];
    long ymin, ymax, ytmp;
    UNIT *curr, *retn;

    retn = curr = *tail;
    if (curr->prev) {
        ymin = ymax = curr->ypos;
        while (!0) {
            ytmp = curr->ypos;
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
            ytmp = curr->ypos - ymin;
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



long UnitListFromLib(ULIB *ulib, long uses, long xdim, long ydim) {
    UNIT *elem, *list = 0;
    long iter, retn;

    while (ulib) {
        if (ulib->ucnt)
            for (iter = 0; iter < uses; iter++)
                if (ulib->uarr[retn = PRNG() % ulib->ucnt]->uuid) {
                    elem = malloc(sizeof(*elem));
                    *elem = *ulib->uarr[retn];
                    elem->prev = list;
                    if (list)
                        list->next = elem;
                    list = elem;
                }
        ulib = ulib->next;
    }
    retn = 0;
    if (list) {
        list->next = 0;
        tail = list;
        while (list) {
            list->xpos = PRNG() % (xdim - list->xdim);
            list->ypos = PRNG() % (ydim - list->ydim) + list->ydim;
            list->fram = PRNG() % list->fcnt;
            list = list->prev;
            retn++;
        }
        SortByY(&tail);
    }
    uarr = calloc(retn, sizeof(**uarr));
    return retn;
}



void FreeEverything(ULIB **ulib) {
    if (tail)
        FreeUnitList(&tail);
    if (*ulib)
        FreeLibList(ulib);
    if (uarr)
        free(uarr);
}



void UpdateFrame(T2UV *data, uint64_t *time, uint32_t flgs,
                 int32_t xptr, int32_t yptr, int32_t isel) {
    long indx = 0;
    uint64_t curr;
    UNIT *iter;

    if ((isel >= 0) || grab) {
        if (!grab && ((pflg ^= flgs) & 1)) {
            grab = uarr[isel];
            printf("[GRABBED] %s\n", grab->path);
            grab->xptr = xptr - grab->xpos;
            grab->yptr = yptr - grab->ypos;
        }
        if (grab && (flgs & 1)) {
            grab->xpos = xptr - grab->xptr;
            grab->ypos = yptr - grab->yptr;
            SortByY(&tail);
        }
        else {
            if (grab)
                printf("[DROPPED] %s\n", grab->path);
            grab = 0;
        }
        pflg = flgs;
    }
    iter = tail;
    while (iter) {
        curr = *time;
        if (curr - iter->tstp > iter->time[iter->fram]) {
            iter->fram = (iter->fram + 1 < iter->fcnt)? iter->fram + 1 : 0;
            iter->tstp = curr;
        }
        uarr[indx] = iter;
        data[indx++] =
            (T2UV){(iter->ypos << 16) | (iter->xpos & 0xFFFF),
                   (iter->fram << 18) | (iter->uuid & 0x3FFFF)};
        iter = iter->prev;
    }
}
