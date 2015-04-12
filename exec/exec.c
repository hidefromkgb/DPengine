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
                    *temp = '\0';
            }
            return retn;
        }
    }
    return *tail = 0;
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
    *flgs = ((HashLine(ToLower(text, 0), 0) == hash)? flag : 0)
          |  (*flgs & ~flag);
}



#define GET_TEMP(conf) (temp = SplitLine(conf, DEF_TSEP, 0))
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
                   BHV_ALLM | BHV_EXEC | BHV_____ | BHV_LOOP, 0, 0};

    /// behaviour name......................................................... !def
    retn->name = HashLine(Dequote(GET_TEMP(conf)), 0);

    /// probability of this behaviour..........................................  def = 0
    if (*GET_TEMP(conf))
        retn->prob = StrToFloat(temp) * 1000.0;

    /// maximum duration in sec................................................  def = 15
    if (*GET_TEMP(conf))
        retn->dmax = StrToFloat(temp) * 1000.0;

    /// minimum duration in sec................................................  def = 5
    if (*GET_TEMP(conf))
        retn->dmin = StrToFloat(temp) * 1000.0;

    /// movement speed (*100/3 for pix/sec)....................................  def = 3
    if (*GET_TEMP(conf))
        retn->move = StrToFloat(temp) * FRM_WAIT * 0.1 / 3.0;

    /// right-sided image...................................................... !def
    /// left-sided image....................................................... !def
    MakeSpritePair(engc->engh, retn->unit, engc->libs->path, conf);

    /// possible movement directions...........................................  def = All
    if ((elem = BinarySearch(uBMT, countof(uBMT),
                             HashLine(ToLower(GET_TEMP(conf), 0), 0))))
        retn->flgs = uBHV[elem - 1] | (retn->flgs & ~BHV_MMMM);

    /// linked behaviour name..................................................  def = ""
    if (*GET_TEMP(conf));

    /// speech said on behaviour start.........................................  def = ""
    if (*GET_TEMP(conf));

    /// speech said on behaviour end...........................................  def = ""
    if (*GET_TEMP(conf));

    /// flag to never exec this behaviour at random............................  def = False
    if (*GET_TEMP(conf))
        AdjustFlags(&retn->flgs, temp, VAL_FALS, BHV_EXEC);

    /// X target to follow.....................................................  def = 0
    if (*GET_TEMP(conf));

    /// Y target to follow.....................................................  def = 0
    if (*GET_TEMP(conf));

    /// name of the target.....................................................  def = ""
    if (*GET_TEMP(conf));

    /// [something unintelligible].............................................  def = True
    if (*GET_TEMP(conf))
        AdjustFlags(&retn->flgs, temp, VAL_TRUE, BHV_____);

    /// [something unintelligible].............................................  def = ""
    if (*GET_TEMP(conf));

    /// [something unintelligible].............................................  def = ""
    if (*GET_TEMP(conf));

    /// right image center (natural center if "0,0")...........................  def = "0,0"
    if (*GET_TEMP(conf));

    /// left image center (natural center if "0,0")............................  def = "0,0"
    if (*GET_TEMP(conf));

    /// flag to prevent animation looping......................................  def = False
    if (*GET_TEMP(conf))
        AdjustFlags(&retn->flgs, temp, VAL_FALS, BHV_LOOP);

    /// behaviour group index..................................................  def = 0
    if (*GET_TEMP(conf));

    /// whether target offset shall be mirrored................................  def = Fixed
    if (*GET_TEMP(conf))
        AdjustFlags(&retn->flgs, temp, FOT_MIRR, BHV_MIRR);
}

void ParseEffect(ENGC *engc, BINF *retn, char **conf) {
    char *temp;

    /// defaults
    *retn = (BINF){{}, {}, {}, 0, 0, 0, 0, 0,
                   BHV_EFCT, 0, 0};

    /// effect name............................................................ !def
    retn->name = HashLine(Dequote(GET_TEMP(conf)), 0);

    /// behaviour name......................................................... !def
    if (*GET_TEMP(conf));

    /// right-sided image...................................................... !def
    /// left-sided image....................................................... !def
    MakeSpritePair(engc->engh, retn->unit, engc->libs->path, conf);
}

void AppendLib(ENGC *engc, char *pcnf, char *base, char *path) {
    char *file, *fptr, *conf, *temp;
    long bcnt;

    conf = ConcatPath(fptr = ConcatPath(base, path), pcnf);
    if ((file = LoadFileZ(conf, 0))) {
        free(conf);

        ListAppendTail((LHDR**)&engc->libs, sizeof(*engc->libs));
        engc->libs->path = fptr;

        fptr = file;
        engc->libs->bcnt = 0;
        while ((conf = SplitLine(&fptr, '\n', 1)))
            switch (DetermineType(&conf)) {
                case SVT_BHVR:
                case SVT_EFCT:
                    engc->libs->bcnt++;
                    break;
            }

        if (engc->libs->bcnt)
            engc->libs->barr = calloc(engc->libs->bcnt,
                                      sizeof(*engc->libs->barr));
        bcnt = 0;
        fptr = file;
        while ((conf = GetNextLine(&fptr)))
            switch (DetermineType(&conf)) {
                case SVT_NAME:
                    engc->libs->name = strdup(GET_TEMP(&conf));
                    break;

                case SVT_EFCT:
                    ParseEffect(engc, &engc->libs->barr[bcnt++], &conf);
                    break;

                case SVT_BHVR:
                    ParseBehaviour(engc, &engc->libs->barr[bcnt++], &conf);
                    break;

                case SVT_BGRP:
                    break;

                case SVT_CTGS:
                    break;

                case SVT_PHRS:
                    break;
            }
        free(file);
        conf = 0;
    }
    else
        free(fptr);
    free(conf);
}
#undef GET_TEMP



void FreeLib(LINF *elem) {
    free(elem->path);
    free(elem->name);
    free(elem->barr);
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
        while (~0) {
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
    long move, flag;
    float angl;

    if (!(bhvr->flgs & BHV_CTLM) &&
         (move = ((bhvr->flgs & BHV_HORM)? 1 : 0)
               + ((bhvr->flgs & BHV_DIAM)? 1 : 0)
               + ((bhvr->flgs & BHV_VERM)? 1 : 0))) {
        flag = min(BHV_HORM, min(BHV_DIAM, BHV_VERM));
        for (move = 1 + ((PRNG(&engc->seed) >> 3) % move); move; ) {
            if (bhvr->flgs & flag)
                move--;
            flag <<= 1;
        }
        angl = 0.0;
        switch (flag >> 1) {
            case BHV_VERM:
                angl = 0.5 * M_PI;
                break;

            case BHV_DIAM:
                move = (((bhvr->flgs & BHV_HNVM) == BHV_HNVM) ||
                        !(bhvr->flgs & BHV_HNVM))? 61 : 31;
                angl =  ((bhvr->flgs & BHV_HNVM) == BHV_VERM)? 45.0 : 15.0;
                angl = (angl + ((PRNG(&engc->seed) >> 3) % move)) * DTR_CONV;
                break;
        }
        pict->move = (T2FV){cosf(angl), sinf(angl)};
    }
    else
        pict->move = (T2FV){0.0, 0.0};

    pict->move.x *= bhvr->move;
    pict->move.y *= bhvr->move;

    pict->indx |= ((PRNG(&engc->seed) >> 3) & 1);
    if (!bhvr->unit[pict->indx & 1].uuid)
        pict->indx ^= 1;
    if (pict->indx & 1)
        pict->move.x = -pict->move.x;
    if ((PRNG(&engc->seed) >> 3) & 1)
        pict->move.y = -pict->move.y;
}



void ChooseBehaviour(ENGC *engc, PICT *pict) {
    pict->indx = (PRNG(&engc->seed) % pict->ulib->bcnt) << 1;
    pict->fram = 0;

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
    ListIterateTailToHead((LHDR*)engc->plst, (ITER)free, 0);
    engc->plst = 0;
    engc->pcnt = 0;
    ListIterateTailToHead((LHDR*)engc->libs,
                          (ITER)AppendSpriteArr, (uintptr_t)engc);
    free(engc->parr);
    engc->parr = 0;
    if (engc->pcnt)
        engc->parr = calloc(engc->pcnt, sizeof(*engc->parr));
    SortByY(engc);
    return engc->pcnt;
}



void FreeEmptySprite(LINF *elem, LINF **edge) {
    BINF *iter = elem->barr - 1;
    long indx;

    for (indx = 0; indx < elem->bcnt; indx++)
        if ((elem->barr[indx].unit[0].uuid | elem->barr[indx].unit[1].uuid)
        &&  (++iter != &elem->barr[indx]))
            *iter = elem->barr[indx];

    if ((indx = iter - elem->barr + 1) < elem->bcnt) {
        if ((elem->bcnt = indx))
            elem->barr = realloc(elem->barr, elem->bcnt * sizeof(*elem->barr));
        else {
            if (elem->prev)
                elem->prev->next = elem->next;
            if (elem->next)
                elem->next->prev = elem->prev;
            if (elem == *edge)
                *edge = (LINF*)((elem->prev)? elem->prev : elem->next);
            FreeLib(elem);
        }
    }
}



void FreeEverything(ENGC *engc) {
    EngineFreeMenu(&engc->menu);
    EngineDeinitialize(engc->engh);
    ListIterateTailToHead((LHDR*)engc->plst, (ITER)free, 0);
    ListIterateTailToHead((LHDR*)engc->libs, (ITER)FreeLib, 0);
    free(engc->parr);
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

            engc->flgs |= ENG_QUIT;
            break;
        }
    }
}



void InitMainMenu(ENGC *engc) {
    MENU tmpl[] =
        {{.text = (uint8_t*)"Remove #",               .func = MMH, .uuid = MMI_CDEL},
         {.text = (uint8_t*)"Remove every #",         .func = MMH, .uuid = MMI_ADEL},
         {.text = (uint8_t*)""},
         {.text = (uint8_t*)"Sleep/pause",            .func = MMH, .uuid = MMI_CSLP},
         {.text = (uint8_t*)"Sleep/pause all",        .func = MMH, .uuid = MMI_ASLP},
         {.text = (uint8_t*)""},
         {.text = (uint8_t*)"Add pony",               .func = MMH, .uuid = MMI_PONY},
         {.text = (uint8_t*)"Add house",              .func = MMH, .uuid = MMI_HOUS},
         {.text = (uint8_t*)""},
         {.text = (uint8_t*)"Take control: Player 1", .func = MMH, .uuid = MMI_TPL1},
         {.text = (uint8_t*)"Take control: Player 2", .func = MMH, .uuid = MMI_TPL2},
         {.text = (uint8_t*)""},
         {.text = (uint8_t*)"Show options",           .func = MMH, .uuid = MMI_OPTS},
         {.text = (uint8_t*)"Return to menu",         .func = MMH, .uuid = MMI_RETN},
         {.text = (uint8_t*)"Exit",                   .func = MMH, .uuid = MMI_EXIT,
          .data = (uintptr_t)engc},
         {}};

    engc->menu = EngineMenuFromTemplate(tmpl);
}



uint32_t UpdateFrame(uintptr_t engh, uintptr_t user,
                     T4FV *data, uint64_t *time, uint32_t flgs,
                     int32_t xptr, int32_t yptr, int32_t isel) {
    ENGC *engc = (ENGC*)user;
    PICT *pict = engc->pcur;
    AINF *anim;

    uint64_t curr;
    long indx;

    if (engc->flgs & ENG_QUIT)
        return 0;

//    EngineBeginAddition(engh);
//    /// here you can add new sprites!
//    EngineFinishLoading(engh);

    if ((isel >= 0) || pict) {
        if (!pict && ((engc->flgs ^ flgs) & 1)) {
            pict = engc->pcur = engc->parr[isel];
            printf("[GRABBED] %s\n", pict->ulib->name);
            engc->ppos.x = xptr - pict->offs.x;
            engc->ppos.y = yptr - pict->offs.y;
        }
        if (pict && (flgs & 1)) {
            pict->offs.x = xptr - engc->ppos.x;
            pict->offs.y = yptr - engc->ppos.y;
        }
        else {
            if (pict)
                printf("[DROPPED] %s\n", pict->ulib->name);
            engc->pcur = 0;
        }
        if (!(flgs & 4) && (engc->flgs & 4))
            EngineOpenContextMenu(engc->menu);
        engc->flgs = flgs;
    }
    for (indx = 0; indx < engc->pcnt; indx++) {
        curr = *time;
        pict = engc->parr[indx];
        anim = &pict->ulib->barr[pict->indx >> 1].unit[pict->indx & 1];
        if (curr - pict->tfrm >= anim->time[pict->fram]) {
            pict->fram = (pict->fram + 1 < anim->fcnt)? pict->fram + 1 : 0;
            pict->tfrm = curr;
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
        data[indx] = (T4FV){pict->offs.x, pict->offs.y,
                            pict->fram, anim->uuid};
    }
    return engc->pcnt;
}
