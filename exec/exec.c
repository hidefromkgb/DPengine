#include "exec.h"



/// Random number generator modulo
#define RNG_TRIM 0xFFFFFFFB
/// Random number generator multiplier
#define RNG_MULT 0x10A860C1
/// Random number generator shift
#define RNG_PLUS 0x01

uint32_t PRNG(uint32_t *seed) {
    return *seed = RNG_PLUS + (*seed * RNG_MULT) % RNG_TRIM;
}



/// String-oriented linear hash multiplier
#define SLH_MULT 0xFBC5
/// String-oriented linear hash shift
#define SLH_PLUS 0x11

uint32_t HashLine(char *line, long size) {
    uint32_t hash = 0;

    if (!size)
        size--;
    while (*line && size--)
        hash = SLH_PLUS + SLH_MULT * hash + *line++;
    return hash;
}



int probcmp(const void *a, const void *b) {
    return (*(BINF**)b)->prob - (*(BINF**)a)->prob;
}

int igrpcmp(const void *a, const void *b) {
    return (*(BINF**)b)->igrp - (*(BINF**)a)->igrp;
}

int namecmp(const void *a, const void *b) {
    return ((BINF*)b)->name - ((BINF*)a)->name;
}



/// [TODO] substitute strtod()!
float StrToFloat(char *data) {
    return strtod(data, 0);
}



/// [TODO] make this mess UTF8-compliant
char *ToLower(char *uppr, long size) {
    long iter;

    if (uppr) {
        if (!size)
            size = strlen(uppr);
        for (iter = 0; iter < size; iter++)
            uppr[iter] = tolower(uppr[iter]);
    }
    return uppr;
}



char *Dequote(char *quot) {
    long size;

    if (!quot || !(size = strlen(quot)))
        return 0;

    if (*quot == '"') {
        quot++;
        size--;
    }
    if (quot[size - 1] == '"')
        quot[size - 1] = '\0';

    return quot;
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



char *SplitLine(char **tail, char tsep, long keep) {
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
                if (!keep)
                    *temp = 0;
            }
            return retn;
        }
        *tail = 0;
    }
    return *tail;
}



uint32_t DetermineType(char **tail) {
    char *temp = SplitLine(tail, DEF_TSEP, 1);

    if (temp && (*temp != DEF_CMNT))
        return HashLine(ToLower(temp, *tail - temp - 1), *tail - temp - 1);
    return ERR_HASH;
}



char *ConcatPath(char *base, char *path) {
    char *retn;
    long  iter;

    if (*path == '"')
        path++;
    retn = malloc(strlen(base) + strlen(path) + 2);
    iter = sprintf(retn, "%s"DEF_DSEP"%s", base, path) - 1;
    if (retn[iter] == '"')
        retn[iter] = '\0';
    return retn;
}



char *GetNextLine(char **file) {
    char *retn;
    long  iter;

    if ((retn = SplitLine(file, '\n', 0)))
        if ((iter = strlen(retn)) > 0)
            if (retn[iter - 1] == '\r')
                retn[iter - 1] = '\0';
    return retn;
}



void ListAppendHead(LHDR **list, long size) {
    LHDR *retn = calloc(1, size);

    if (!*list)
         *list = retn;
    else {
        if ((*list)->prev) {
            retn->prev = (*list)->prev;
            retn->prev->next = retn;
        }
        retn->next = *list;
        (*list)->prev = retn;
        *list = retn;
    }
}



void ListAppendTail(LHDR **list, long size) {
    LHDR *retn = calloc(1, size);

    if (!*list)
         *list = retn;
    else {
        if ((*list)->next) {
            retn->next = (*list)->next;
            retn->next->prev = retn;
        }
        retn->prev = *list;
        (*list)->next = retn;
        *list = retn;
    }
}



LHDR *ListIterateHeadToTail(LHDR *list, ITER iter, uintptr_t data) {
    LHDR *elem;

    if (!list)
        return 0;
    do {
        elem = list;
        list = list->next;
        iter(elem, data);
    } while (list);
    return elem;
}



LHDR *ListIterateTailToHead(LHDR *list, ITER iter, uintptr_t data) {
    LHDR *elem;

    if (!list)
        return 0;
    do {
        elem = list;
        list = list->prev;
        iter(elem, data);
    } while (list);
    return elem;
}



void MakeSpritePair(uintptr_t engh, AINF *pair, char *path, char **conf) {
    char *file;
    long iter;

    for (iter = 0; iter <= 1; iter++) {
        file = ConcatPath(path, Dequote(SplitLine(conf, DEF_TSEP, 0)));
        EngineLoadAnimAsync(engh, (uint8_t*)file, &pair[iter]);
        free(file);
    }
}



uint32_t BinarySearch(uint32_t *data, uint32_t size, uint32_t elem) {
    uint32_t *iter = data, fork = size;

    if (!data || !size)
        return 0;

    while (fork > 1) {
        fork = (fork >> 1) + (fork & 1);
        if (iter[fork - 1] < elem)
            iter += fork;
        else if (iter[fork - 1] == elem)
            break;
    }
    return (iter[fork - 1] == elem)? iter + fork - data : 0;
}



void AdjustFlags(uint32_t *flgs, char *text, uint32_t hash, uint32_t flag) {
    uint32_t bred = HashLine(ToLower(text, 0), 0);
    *flgs = ((bred == hash)? flag : 0)
          |  (*flgs & ~flag);
}



#define TRY_TEMP(conf) (GET_TEMP(conf) && *temp)
#define GET_TEMP(conf) (temp = SplitLine(conf, DEF_TSEP, 0))
#define BIN_FIND(list, temp) BinarySearch(list, countof(list), \
                                          HashLine(ToLower(temp, 0), 0))
void ParseBehaviour(ENGC *engc, BINF *retn, char **conf) {
    static uint32_t
        uBMT[] = {BMT_HORM, BMT_ALLM, BMT_VERM, BMT_DRGM, BMT_HNDM, BMT_SLPM,
                  BMT_HNVM, BMT_DIAM, BMT_OVRM, BMT_DNVM, BMT_NONM},
        uBHV[] = {BHV_HORM, BHV_ALLM, BHV_VERM, BHV_DRGM, BHV_HNDM, BHV_SLPM,
                  BHV_HNVM, BHV_DIAM, BHV_OVRM, BHV_DNVM, BHV_NONM};
    uint32_t elem;
    char *temp;

    /// defaults
    *retn = (BINF){{}, {}, {}, 0, 5000, 15000, 0.1 * FRM_WAIT, 0,
                   BHV_ALLM | BHV_EXEC | BHV_____ | FLG_LOOP, 0, 0};

    /// behaviour name......................................................... !def
    retn->name = HashLine(Dequote(GET_TEMP(conf)), 0);

    /// probability of this behaviour..........................................  def = 0
    if (TRY_TEMP(conf)) {
        retn->prob = StrToFloat(temp) * 1000.0;
        retn->prob = min(1000, max(0, retn->prob));
    }
    /// maximum duration in sec................................................  def = 15
    if (TRY_TEMP(conf))
        retn->dmax = StrToFloat(temp) * 1000.0;

    /// minimum duration in sec................................................  def = 5
    if (TRY_TEMP(conf))
        retn->dmin = StrToFloat(temp) * 1000.0;

    /// movement speed (*100/3 for pix/sec)....................................  def = 3
    if (TRY_TEMP(conf))
        retn->move = StrToFloat(temp) * FRM_WAIT * 0.1 / 3.0;

    /// right-sided image...................................................... !def
    /// left-sided image....................................................... !def
    MakeSpritePair(engc->engh, retn->unit, engc->libs->path, conf);

    /// possible movement directions...........................................  def = All
    if ((elem = BIN_FIND(uBMT, GET_TEMP(conf))))
        retn->flgs = uBHV[elem - 1] | (retn->flgs & ~BHV_MMMM);

    /// linked behaviour name..................................................  def = ""
    if (TRY_TEMP(conf))
        retn->link = HashLine(Dequote(temp), 0);

/// [TODO:]
    /// speech said on behaviour start.........................................  def = ""
    if (TRY_TEMP(conf));

/// [TODO:]
    /// speech said on behaviour end...........................................  def = ""
    if (TRY_TEMP(conf));

    /// flag to never exec this behaviour at random............................  def = False
    if (TRY_TEMP(conf))
        AdjustFlags(&retn->flgs, temp, VAL_FALS, BHV_EXEC);

    /// X target to follow.....................................................  def = 0
    if (TRY_TEMP(conf))
        retn->ptgt.x = StrToFloat(temp);

    /// Y target to follow.....................................................  def = 0
    if (TRY_TEMP(conf))
        retn->ptgt.y = StrToFloat(temp);

    /// name of the target.....................................................  def = ""
    if (TRY_TEMP(conf))
        retn->trgt = HashLine(Dequote(temp), 0);

/// [TODO:]
    /// [something unintelligible].............................................  def = True
    if (TRY_TEMP(conf))
        AdjustFlags(&retn->flgs, temp, VAL_TRUE, BHV_____);

/// [TODO:]
    /// [something unintelligible].............................................  def = ""
    if (TRY_TEMP(conf));

/// [TODO:]
    /// [something unintelligible].............................................  def = ""
    if (TRY_TEMP(conf));

    /// right image center (natural center if "0,0")...........................  def = "0,0"
    if (TRY_TEMP(conf)) {
        retn->cntr[0].x = StrToFloat(Dequote(temp));
        retn->cntr[0].y = StrToFloat(Dequote(GET_TEMP(conf)));
    }
    /// left image center (natural center if "0,0")............................  def = "0,0"
    if (TRY_TEMP(conf)) {
        retn->cntr[1].x = StrToFloat(Dequote(temp));
        retn->cntr[1].y = StrToFloat(Dequote(GET_TEMP(conf)));
    }
    /// flag to prevent animation looping......................................  def = False
    if (TRY_TEMP(conf))
        AdjustFlags(&retn->flgs, temp, VAL_FALS, FLG_LOOP);

    /// behaviour group index..................................................  def = 0
    if (TRY_TEMP(conf)) {
        retn->igrp = StrToFloat(temp);
        retn->igrp = max(0, retn->igrp);
    }
    /// whether target offset shall be mirrored................................  def = Fixed
    if (TRY_TEMP(conf))
        AdjustFlags(&retn->flgs, temp, FOT_MIRR, BHV_MIRR);
}

void ParseEffect(ENGC *engc, BINF *retn, char **conf) {
    static uint32_t
        uEMT[] = {EMT_CNRA, EMT_RNDA, EMT_CNTA, EMT_CNLA, EMT_BTMA, EMT_BNLA,
                  EMT_TOPA, EMT_BNRA, EMT_RCLA, EMT_TNLA, EMT_TNRA},
        uEFF[] = {EFF_CNRA, EFF_RNDA, EFF_CNTA, EFF_CNLA, EFF_BTMA, EFF_BNLA,
                  EFF_TOPA, EFF_BNRA, EFF_RCLA, EFF_TNLA, EFF_TNRA};
    uint32_t elem;
    char *temp;

    /// defaults
    *retn = (BINF){{}, {}, {}, 0, 5000, 0, 0, 0,
                   FLG_EFCT | EFF_STAY | (EFF_RNDA * 0x1111), 0, 0};

    /// effect name (skipped intentionally).................................... !def
    if (TRY_TEMP(conf));

    /// behaviour name......................................................... !def
    retn->name = HashLine(Dequote(GET_TEMP(conf)), 0);

    /// right-sided image...................................................... !def
    /// left-sided image....................................................... !def
    MakeSpritePair(engc->engh, retn->unit, engc->libs->path, conf);

    /// duration in sec........................................................  def = 5
    if (TRY_TEMP(conf))
        retn->dmin = StrToFloat(temp) * 1000.0;

    /// cooldown in sec........................................................  def = 0 (no repeating)
    if (TRY_TEMP(conf))
        retn->dmax = StrToFloat(temp) * 1000.0;

    /// possible right placements..............................................  def = Any
    if ((elem = BIN_FIND(uEMT, GET_TEMP(conf))))
        retn->flgs = (uEFF[elem - 1] <<  0) | (retn->flgs & ~(EFF_AAAA <<  0));

    /// possible right centerings..............................................  def = Any
    if ((elem = BIN_FIND(uEMT, GET_TEMP(conf))))
        retn->flgs = (uEFF[elem - 1] <<  4) | (retn->flgs & ~(EFF_AAAA <<  4));

    /// possible left placements...............................................  def = Any
    if ((elem = BIN_FIND(uEMT, GET_TEMP(conf))))
        retn->flgs = (uEFF[elem - 1] <<  8) | (retn->flgs & ~(EFF_AAAA <<  8));

    /// possible left centerings...............................................  def = Any
    if ((elem = BIN_FIND(uEMT, GET_TEMP(conf))))
        retn->flgs = (uEFF[elem - 1] << 12) | (retn->flgs & ~(EFF_AAAA << 12));

    /// flag to keep animation where it is.....................................  def = False
    if (TRY_TEMP(conf))
        AdjustFlags(&retn->flgs, temp, VAL_FALS, EFF_STAY);

    /// flag to prevent animation looping......................................  def = False
    if (TRY_TEMP(conf))
        AdjustFlags(&retn->flgs, temp, VAL_FALS, FLG_LOOP);

    if (!retn->dmax)
        retn->flgs &= ~FLG_LOOP;
}

void AppendLib(ENGC *engc, char *pcnf, char *base, char *path) {
    char *file, *fptr, *conf, *temp;
    long bcnt, ecnt;

    conf = ConcatPath(fptr = ConcatPath(base, path), pcnf);
    if ((file = LoadFileZ(conf, 0))) {
        free(conf);

        ListAppendTail((LHDR**)&engc->libs, sizeof(*engc->libs));
        engc->libs->path = fptr;

        fptr = file;
        engc->libs->bcnt = engc->libs->ecnt = 0;
        while ((conf = SplitLine(&fptr, '\n', 1)))
            switch (DetermineType(&conf)) {
                case SVT_BHVR:
                    engc->libs->bcnt++;
                    break;

                case SVT_EFCT:
                    engc->libs->ecnt++;
                    break;
            }

        bcnt = ecnt = 0;
        if (engc->libs->bcnt)
            engc->libs->barr = calloc(engc->libs->bcnt,
                                      sizeof(*engc->libs->barr));
        if (engc->libs->ecnt)
            engc->libs->earr = calloc(engc->libs->ecnt,
                                      sizeof(*engc->libs->earr));
        fptr = file;
        while ((conf = GetNextLine(&fptr)))
            switch (DetermineType(&conf)) {
                case SVT_NAME:
                    engc->libs->name = strdup(GET_TEMP(&conf));
                    break;

                case SVT_EFCT:
                    ParseEffect(engc, &engc->libs->earr[ecnt++], &conf);
                    break;

                case SVT_BHVR:
                    ParseBehaviour(engc, &engc->libs->barr[bcnt++], &conf);
                    engc->libs->prob += engc->libs->barr[bcnt - 1].prob;
                    break;

                case SVT_BGRP:
                    /// doesn`t help much, skipping
                    break;

                case SVT_CTGS:
                    break;

                case SVT_PHRS:
                    break;
            }
        if (!engc->libs->name)
            engc->libs->name = strdup(path);
        free(file);
        conf = 0;
    }
    else
        free(fptr);
    free(conf);
}
#undef BIN_FIND
#undef GET_TEMP
#undef TRY_TEMP



void FreeLib(LINF *elem) {
    free(elem->path);
    free(elem->name);
    free(elem->barr);
    free(elem->earr);
    free(elem->bgrp);
    free(elem->ngrp);
    free(elem->prob);
    free(elem);
}



#define MAX_YDIM 0x1000
void SortByY(ENGC *engc) {
    LHDR elem[MAX_YDIM + 1] = {};
    PICT *iter;
    long ymin, ymax, ytmp;

    if (!(iter = engc->plst))
        return;

    if (iter->prev) {
        ymin = MAX_YDIM + 1;
        ymax = -1;
        while (!0) {
            ytmp = iter->offs.y;
            if ((ytmp >= 0) && (ytmp < MAX_YDIM)) {
                if (ytmp > ymax)
                    ymax = ytmp;
                else if (ytmp < ymin)
                    ymin = ytmp;
            }
            if (!iter->prev)
                break;
            iter = (PICT*)iter->prev;
        }
        while (iter) {
            ytmp = iter->offs.y - ymin + 1;
            if ((ytmp < 0) || (iter->offs.y >= MAX_YDIM))
                ytmp = 0;
            if (!elem[ytmp].prev)
                elem[ytmp].next = elem[ytmp].prev = (LHDR*)iter;
            else {
                iter->prev = elem[ytmp].next;
                elem[ytmp].next = iter->prev->next = (LHDR*)iter;
            }
            iter = (PICT*)iter->next;
        }
        if ((ymax -= ymin - 1) < 0)
            ymax = 1;
        for (ytmp = 0; ytmp <= ymax; ytmp++)
            if (elem[ytmp].prev) {
                elem[ytmp].prev->prev = (LHDR*)iter;
                if (iter)
                    iter->next = elem[ytmp].prev;
                iter = (PICT*)elem[ytmp].next;
            }

        ytmp = 0;
        iter->next = 0;
        engc->plst = iter;
        while (iter) {
            engc->parr[ytmp++] = iter;
            iter = (PICT*)iter->prev;
        }
    }
    else
        engc->parr[0] = engc->plst;
}
#undef MAX_YDIM



void PlaceRandomly(ENGC *engc, PICT *pict) {
    AINF *anim = &pict->ulib->barr[pict->indx >> 1].unit[pict->indx & 1];

    pict->offs.x = PRNG(&engc->seed) % (engc->dims.x - anim->dims.x);
    pict->offs.y = PRNG(&engc->seed) % (engc->dims.y - anim->dims.y)
                                                     + anim->dims.y;
}



long BoundCrossed(float move, float offs, long bmin, long bmax) {
    if ((move < 0) && (offs + move <= bmin))
        return -1;
    if ((move > 0) && (offs + move >= bmax))
        return 1;
    return 0;
}



void ChooseDirection(ENGC *engc, PICT *pict) {
    BINF *bhvr = &pict->ulib->barr[pict->indx >> 1];
    float angl;
    long flag;

    if (!(bhvr->flgs & BHV_CTLM) && (bhvr->flgs & BHV_ALLM)) {
        flag = PRNG(&engc->seed) >> 3;
        switch (bhvr->flgs & BHV_ALLM) {
            /// horizontal + diagonal + vertical movement
            case BHV_ALLM:
                flag %= 3;
                flag = (flag)? (flag != 1)? BHV_HORM : BHV_DIAM : BHV_VERM;
                break;

            /// horizontal + vertical movement
            case BHV_HNVM:
                flag = (flag & 1)? BHV_HORM : BHV_VERM;
                break;

            /// horizontal + diagonal movement
            case BHV_HNDM:
                flag = (flag & 1)? BHV_HORM : BHV_DIAM;
                break;

            /// diagonal + vertical movement
            case BHV_DNVM:
                flag = (flag & 1)? BHV_DIAM : BHV_VERM;
                break;

            /// separate movement
            default:
                flag = bhvr->flgs;
                break;
        }
        switch (flag) {
            case BHV_DIAM:
                flag = (((bhvr->flgs & BHV_HNVM) == BHV_HNVM) ||
                        !(bhvr->flgs & BHV_HNVM))? 61 : 31;
                angl =  ((bhvr->flgs & BHV_HNVM) == BHV_VERM)? 45.0 : 15.0;
                angl = (angl + ((PRNG(&engc->seed) >> 3) % flag)) * DTR_CONV;
                break;

            case BHV_VERM:
                angl = 0.5 * M_PI;
                break;

            default:
                angl = 0.0;
                break;
        }
        pict->move = (T2FV){cosf(angl), sinf(angl)};
    }
    else
        pict->move = (T2FV){0.0, 0.0};

    pict->move.x *= bhvr->move;
    pict->move.y *= bhvr->move;

    pict->indx = (pict->indx & -2) | ((PRNG(&engc->seed) >> 3) & 1);
    if (!bhvr->unit[pict->indx & 1].uuid)
        pict->indx ^= 1;
    if (pict->indx & 1)
        pict->move.x = -pict->move.x;
    if ((PRNG(&engc->seed) >> 3) & 1)
        pict->move.y = -pict->move.y;
}



uint32_t BinarySearchQ(uint32_t *data, uint32_t size, uint32_t elem) {
    uint32_t *iter = data, fork = size;

    if (!data || !size)
        return 0;

    while (fork > 1) {
        fork = (fork >> 1) + (fork & 1);
        if (iter[fork - 1] < elem)
            iter += fork;
        else if (iter[fork - 1] == elem)
            break;
    }
    return iter + fork - data;
}



void ChooseBehaviour(ENGC *engc, PICT *pict) {
    LINF *ulib = pict->ulib;
//    long igrp, nmin;

    if (ulib->barr[pict->indx >> 1].link)
        pict->indx = (ulib->barr[pict->indx >> 1].link - 1) << 1;
    else {
//        igrp = ulib->barr[pict->indx >> 1].igrp;
//        nmin = (igrp)? ulib->ngrp[igrp - 1] : 0;

//        /// bne - binary search, nonexact
//        pict->indx = (*bne((PRNG(&engc->seed) % ulib->prob[igrp]) + 1,
//                           ulib->bgrp + nmin, ulib->ngrp[igrp] - nmin,
//                           sizeof(*ulib->bgrp), probcmp) - ulib->barr) << 1;

        pict->indx = (PRNG(&engc->seed) % ulib->bcnt) << 1;
    }
    ChooseDirection(engc, pict);
}



void AppendSpriteArr(LINF *elem, ENGC *engc) {
    long icnt;

    if (!elem->bcnt)
        return;

    engc->pcnt += elem->icnt;
    for (icnt = 0; icnt < elem->icnt; icnt++) {
        ListAppendTail((LHDR**)&engc->plst, sizeof(*engc->plst));
        engc->plst->ulib = elem;
        ChooseBehaviour(engc, engc->plst);
        PlaceRandomly(engc, engc->plst);
    }
}



long MakeSpriteArr(ENGC *engc) {
    TTH_ITER(engc->plst, free, 0);
    engc->plst = 0;
    engc->pcnt = 0;
    TTH_ITER(engc->libs, AppendSpriteArr, engc);
    free(engc->parr);
    free(engc->data);
    engc->parr = 0;
    engc->data = 0;
    if (engc->pcnt) {
        engc->parr = calloc(engc->pcnt, sizeof(*engc->parr));
        engc->data = calloc(engc->pcnt, sizeof(*engc->data));
    }
    SortByY(engc);
    return engc->pcnt;
}



long TruncateAndSort(BINF **base, long *rcnt) {
    BINF *root = *base, *iter = root - 1;
    long indx;

    for (indx = 0; indx < *rcnt; indx++)
        if ((root[indx].unit[0].uuid | root[indx].unit[1].uuid)
        &&  (++iter != &root[indx]))
            *iter = root[indx];

    if ((indx = iter - root + 1) < *rcnt)
        *base = realloc(root, (*rcnt = indx) * sizeof(*root));
    if (*rcnt)
        qsort(*base, *rcnt, sizeof(**base), namecmp);
    return *rcnt;
}

void PrepareSpriteArr(LINF *elem, LINF **edge) {
    BINF temp, *iter;
    long indx, turn;

    /// "already prepared" flag
    if (elem->flgs & 1)
        return;
    elem->flgs |= 1;

    TruncateAndSort(&elem->earr, &elem->ecnt);
    if (!TruncateAndSort(&elem->barr, &elem->bcnt)) {
        if (elem->prev)
            elem->prev->next = elem->next;
        if (elem->next)
            elem->next->prev = elem->prev;
        if (elem == *edge)
            *edge = (LINF*)((elem->prev)? elem->prev : elem->next);
        FreeLib(elem);
        return;
    }
    for (indx = 0; indx < elem->ecnt; indx++)
        if ((iter = bsearch(&elem->earr[indx], elem->barr, elem->bcnt,
                            sizeof(*elem->barr), namecmp)) && (!iter->neff++))
            iter->ieff = indx;
    for (indx = 0; indx < elem->bcnt; indx++) {
        if (elem->barr[indx].link) {
            temp.name = elem->barr[indx].link;
            if ((iter = bsearch(&temp, elem->barr, elem->bcnt,
                                sizeof(*elem->barr), namecmp)))
                elem->barr[indx].link = iter - elem->barr + 1;
            else
                elem->barr[indx].link = 0;
        }
        for (iter = &elem->barr[indx], turn = 0; turn <= 1; turn++)
            if (!(iter->cntr[turn].x | iter->cntr[turn].y))
                iter->cntr[turn] = (T2IV){iter->unit[turn].dims.x >> 1,
                                          iter->unit[turn].dims.y >> 1};
    }
    elem->bgrp = malloc(elem->bcnt * sizeof(*elem->bgrp));
    for (elem->zcnt = indx = 0; indx < elem->bcnt; indx++)
        if (elem->barr[indx].prob)
            elem->bgrp[elem->zcnt++] = &elem->barr[indx];
    elem->bgrp = realloc(elem->bgrp, elem->zcnt * sizeof(*elem->bgrp));
    qsort(elem->bgrp, elem->zcnt, sizeof(*elem->bgrp), igrpcmp);
    for (elem->gcnt = turn = indx = 0; indx < elem->zcnt; indx++) {
        if (elem->bgrp[indx]->igrp != turn) {
            turn = elem->bgrp[indx]->igrp;
            elem->gcnt++;
        }
        elem->bgrp[indx]->igrp = elem->gcnt;
    }
    elem->gcnt++;
    elem->ngrp = calloc(elem->gcnt, sizeof(*elem->ngrp));
    elem->prob = calloc(elem->gcnt, sizeof(*elem->prob));
    for (turn = indx = 0; indx < elem->zcnt; indx++)
        elem->ngrp[elem->bgrp[indx]->igrp] = indx + 1;
    for (turn = indx = 0; indx < elem->gcnt; indx++) {
        qsort(elem->bgrp + turn, elem->ngrp[indx] - turn,
              sizeof(*elem->bgrp), probcmp);
        for (++turn; turn < elem->ngrp[indx]; ++turn)
            elem->bgrp[turn]->prob += elem->bgrp[turn - 1]->prob;
        elem->prob[indx] = elem->bgrp[turn - 1]->prob;
        turn = elem->ngrp[indx];
    }
}



void FreeEverything(ENGC *engc) {
    EngineFreeMenu(&engc->menu);
    EngineDeinitialize(engc->engh);
    TTH_ITER(engc->plst, free, 0);
    TTH_ITER(engc->libs, FreeLib, 0);
    free(engc->parr);
    free(engc->data);
    *engc = (ENGC){};
}



#define MMI_CDEL  1
#define MMI_ADEL  2
#define MMI_CSLP  3
#define MMI_ASLP  4
#define MMI_PONY  5
#define MMI_HOUS  6
#define MMI_TPL1  7
#define MMI_TPL2  8
#define MMI_OPTS  9
#define MMI_RETN 10
#define MMI_EXIT 11



void MMH(MENU *item) {
    switch (item->uuid) {
        case MMI_CDEL:
            break;

        case MMI_ADEL:
            break;

        case MMI_CSLP:
            break;

        case MMI_ASLP:
            break;

        case MMI_PONY:
            break;

        case MMI_HOUS:
            break;

        case MMI_TPL1:
            break;

        case MMI_TPL2:
            break;

        case MMI_OPTS:
            break;

        case MMI_RETN:
            break;

        case MMI_EXIT: {
            ENGC *engc = (ENGC*)item->data;

            engc->quit = 1;
            break;
        }
    }
}



void InitMainMenu(ENGC *engc) {
    MENU tmpl[] =
        {{.text = (uint8_t*)"", .flgs = MFL_GRAY},
         {.text = (uint8_t*)""},
         {.text = (uint8_t*)"Remove pony",               .func = MMH, .uuid = MMI_CDEL},
         {.text = (uint8_t*)"Remove all similar ponies", .func = MMH, .uuid = MMI_ADEL},
         {.text = (uint8_t*)""},
         {.text = (uint8_t*)"Sleep/pause",               .func = MMH, .uuid = MMI_CSLP},
         {.text = (uint8_t*)"Sleep/pause all",           .func = MMH, .uuid = MMI_ASLP},
         {.text = (uint8_t*)""},
         {.text = (uint8_t*)"Add pony >>>",              .func = MMH, .uuid = MMI_PONY},
         {.text = (uint8_t*)"Add house >>>",             .func = MMH, .uuid = MMI_HOUS},
         {.text = (uint8_t*)""},
         {.text = (uint8_t*)"Take control: Player 1",    .func = MMH, .uuid = MMI_TPL1},
         {.text = (uint8_t*)"Take control: Player 2",    .func = MMH, .uuid = MMI_TPL2},
         {.text = (uint8_t*)""},
         {.text = (uint8_t*)"Show options...",           .func = MMH, .uuid = MMI_OPTS},
         {.text = (uint8_t*)"Return to menu...",         .func = MMH, .uuid = MMI_RETN},
         {.text = (uint8_t*)"Exit",                      .func = MMH, .uuid = MMI_EXIT,
          .data = (uintptr_t)engc},
         {}};

    engc->menu = EngineMenuFromTemplate(tmpl);
}



uint32_t UpdateFrame(uintptr_t engh, uintptr_t user,
                     T4FV **data, uint64_t *time, uint32_t flgs,
                     int32_t xptr, int32_t yptr, int32_t isel) {
    ENGC *engc = (ENGC*)user;
    PICT *pict = engc->pcur;
    BINF *binf;
    AINF *anim;

    uint8_t *temp;
    uint64_t curr;
    long indx;

    if (engc->quit)
        return 0;

//    EngineBeginAddition(engh);
//    /// here you can add new sprites!
//    EngineFinishLoading(engh);

    if ((flgs & UFR_MOUS) && ((isel >= 0) || pict)) {
        if (!pict && ((engc->flgs ^ flgs) & UFR_LBTN)) {
            pict = engc->pcur = engc->parr[isel];
            printf("[GRABBED] %s\n", pict->ulib->name);
            engc->ppos.x = xptr - pict->offs.x;
            engc->ppos.y = yptr - pict->offs.y;
        }
        if (pict && (flgs & UFR_LBTN)) {
            pict->offs.x = xptr - engc->ppos.x;
            pict->offs.y = yptr - engc->ppos.y;
        }
        else {
            if (pict)
                printf("[DROPPED] %s\n", pict->ulib->name);
            engc->pcur = 0;
        }
        if (~flgs & engc->flgs & UFR_RBTN) {
            if (!pict)
                pict = engc->parr[isel];
            temp = malloc(32 + strlen(pict->ulib->name));
            sprintf((char*)temp, "[ %s ]", pict->ulib->name);
            EngineUpdateMenuItemText(&engc->menu[0], temp);
            free(temp);
            EngineOpenContextMenu(engc->menu);
        }
        engc->flgs = flgs;
    }
    for (indx = 0; indx < engc->pcnt; indx++) {
        curr = *time;
        pict = engc->parr[indx];
        binf = &pict->ulib->barr[pict->indx >> 1];
        anim = &binf->unit[pict->indx & 1];
        if ((curr >= pict->tbhv)
        || ((pict->fram >= anim->fcnt) && !(binf->flgs & FLG_LOOP))) {
            ChooseBehaviour(engc, pict);
            binf = &pict->ulib->barr[pict->indx >> 1];
            anim = &binf->unit[pict->indx & 1];
            pict->fram = -1;
            pict->tfrm = curr;
            pict->tbhv = curr + binf->dmin;
            if (binf->dmax > binf->dmin)
                pict->tbhv += PRNG(&engc->seed) % (binf->dmax - binf->dmin);
        }
        if (curr >= pict->tfrm) {
            pict->fram =
                (pict->fram + 1 >= anim->fcnt)? (binf->flgs & FLG_LOOP)?
                 0 : pict->fram : pict->fram + 1;
            pict->tfrm = curr + anim->time[pict->fram];
        }
        if (curr - pict->tmov >= FRM_WAIT) {
            if (BoundCrossed(pict->move.x, pict->offs.x + anim->dims.x,
                             anim->dims.x, engc->dims.x)) {
                pict->move.x = -pict->move.x;
                pict->indx ^= 1;
            }
            if (BoundCrossed(pict->move.y, pict->offs.y,
                             anim->dims.y, engc->dims.y))
                pict->move.y = -pict->move.y;

            pict->offs.x += pict->move.x;
            pict->offs.y += pict->move.y;
            pict->tmov = curr;
        }
    }
    SortByY(engc);
    for (indx = 0; indx < engc->pcnt; indx++) {
        pict = engc->parr[indx];
        anim = &pict->ulib->barr[pict->indx >> 1].unit[pict->indx & 1];
        engc->data[indx] = (T4FV){pict->offs.x, pict->offs.y,
                                  pict->fram, anim->uuid};
    }
    *data = engc->data;
    return engc->pcnt;
}
