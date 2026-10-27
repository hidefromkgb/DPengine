#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "exec_parser.hpp"
#include "exec_web.hpp"
#include "zip/zip_load.h"



// parallel execution unit data
struct para_t {
    intptr_t desc;
    char *hash, *file, *hreq;
};

#define MAKE_GET_QUERY(uenc, ...) \
        make_get_query(uenc, ##__VA_ARGS__, (const char*)nullptr)
// concatenates the strings; UENC: URL-encode all but DEF_DSEP
static char *make_get_query(long uenc, ...) {
    const char *indx;
    char *temp, *retn;
    long size;
    va_list list;

    size = 1;
    va_start(list, uenc);
    while ((indx = va_arg(list, const char*)))
        size += strlen(indx) * 3 * 6;
    va_end(list);

    va_start(list, uenc);
    temp = retn = (char*)realloc(NULL, size);
    while ((indx = va_arg(list, const char*))) {
        for (indx -= (size = 1 - (long)strlen(indx)); size <= 0; size++)
            if (!uenc || (indx[size] == *DEF_DSEP)) {
                *temp++ = indx[size];
            } else {
                temp[2] = "0123456789ABCDEF"[(indx[size] >> 0) & 0xF];
                temp[1] = "0123456789ABCDEF"[(indx[size] >> 4) & 0xF];
                temp[0] = '%';
                temp += 3;
            }
    }
    *temp = 0;
    va_end(list);
    return retn;
}

static void make_sha1(uint32_t *curr, const char *text, uint32_t size,
        uint32_t base, uint32_t trim) {
    #define SHA_R(v, b) (((v) << (b)) | ((v) >> (32 - (b))))
    #define SHA_I(i) (data.l[i] = (SHA_R(data.l[i], 24) & 0xFF00FF00)         \
                                | (SHA_R(data.l[i],  8) & 0x00FF00FF))
    #define SHA_U(i) (data.l[i & 15] = SHA_R(data.l[(i + 13) & 0x0F]          \
                                           ^ data.l[(i +  8) & 0x0F]          \
                                           ^ data.l[(i +  2) & 0x0F]          \
                                           ^ data.l[(i +  0) & 0x0F], 1))
    #define SHA_1(n) do {head[0] = ((head[2] & (head[3] ^ head[4])) ^ head[4])\
                                 + n + head[5] + SHA_R(head[1], 5);           \
                         head[2] = SHA_R(head[2], 30); head--;} while (0)
    #define SHA_2(n) do {head[0] = (head[2] ^ head[3] ^ head[4])              \
                                 + n + head[5] + SHA_R(head[1], 5);           \
                         head[2] = SHA_R(head[2], 30); head--;} while (0)
    #define SHA_3(n) do {head[0] = (((head[2] | head[3]) & head[4])           \
                                   | (head[2] & head[3]))                     \
                                 + n + head[5] + SHA_R(head[1], 5);           \
                         head[2] = SHA_R(head[2], 30); head--;} while (0)

    uint32_t iter, indx, pack, *head, temp[5];
    #pragma pack(push, 1)
    union {
        uint32_t l[16];
        uint8_t b[64];
    } data;
    #pragma pack(pop)

    pack = size - ((size + 1) & 0x3F) +
                ((((size + 1) & 0x3F) > 0x38)? 0x80 : 0x40);
    trim = (trim)? trim : pack;
    for (iter = base; iter <= trim; iter++) {
        data.b[iter & 0x3F] = (iter <= size)? (iter == size)? 0x80 : text[iter]
                 : (pack - iter < 4)? (size << 3) >> ((~iter << 3) & 0x1F) : 0;
        if ((iter & 0x3F) == 0x3F) {
            head = curr + 5 * 4 * 4 - 1;
            memcpy(temp, curr, sizeof(temp));
            memcpy(head + 1, curr, sizeof(temp));
            for (indx =  0; indx < 16; indx++) SHA_1(0x5A827999 + SHA_I(indx));
            for (indx = 16; indx < 20; indx++) SHA_1(0x5A827999 + SHA_U(indx));
            for (indx = 20; indx < 40; indx++) SHA_2(0x6ED9EBA1 + SHA_U(indx));
            for (indx = 40; indx < 60; indx++) SHA_3(0x8F1BBCDC + SHA_U(indx));
            for (indx = 60; indx < 80; indx++) SHA_2(0xCA62C1D6 + SHA_U(indx));
            curr[0] += temp[0]; curr[1] += temp[1]; curr[2] += temp[2];
            curr[3] += temp[3]; curr[4] += temp[4];
        }
    }
    #undef SHA_3
    #undef SHA_2
    #undef SHA_1
    #undef SHA_U
    #undef SHA_I
    #undef SHA_R
}

static void check_hash_and_download(intptr_t user, uint64_t zero) {
    auto para = (para_t*)user;
    char shas[5 * 4 * 2 + 1], *blob, *load, *text;
    uint32_t curr[5 * 4 * 4 + 4 + 1], retn;
    uint8_t shan[5 * 4];
    long size, flen;

    flen = sprintf(text = (char*)realloc(NULL, 0x100 + strlen(para->file)),
                   "\"%s\"... ", para->file);
    size = 0;
    shan[0] = 1;
    shas[1] = 0;
    shas[0] = '0';
    load = rLoadFile(para->file, &size);
    if (strlen(para->hash) == sizeof(shan) << 1) {
        retn = sprintf(blob = (char*)realloc(NULL, 32), "blob %ld", size);
        curr[0] = 0x67452301; curr[1] = 0xEFCDAB89; curr[2] = 0x98BADCFE;
        curr[3] = 0x10325476; curr[4] = 0xC3D2E1F0;
        make_sha1(curr, blob, retn + 1 + size, 0, retn);
        make_sha1(curr, load - (retn + 1), retn + 1 + size, retn + 1, 0);
        for (retn = 0; retn < sizeof(shan); retn++) {
            // [TODO:] fix little-endian dependency
            shan[retn] = curr[retn >> 2] >> ((~retn & 3) << 3);
            sprintf(shas + (retn << 1), "%02x", shan[retn]);
        }
        for (retn = 0; retn < (sizeof(shan)) << 1; retn++) {
            // [TODO:] fix little-endian dependency
            shan[retn >> 1] = (shan[retn >> 1] << 4) | (shan[retn >> 1] >> 4);
            shan[retn >> 1] ^= ((para->hash[retn] & 0x0F)
                             + ((para->hash[retn] & 0x40)? 9 : 0)) & 0x0F;
        }
        for (retn = 0; retn < sizeof(shan); retn++)
            shan[0] |= shan[retn];
        free(blob);
    }
    free(load);
    if (!shan[0]) {
        sprintf(text + flen, "already updated");
    } else {
        size = rLoadHTTPS(para->desc, para->hreq, &load, 0, 0);
        if (!load) {
            sprintf(text + flen, "error!");
        } else {
            sprintf(text + flen, "%s vs. %s", shas, para->hash);
            rSaveFile(para->file, load, size);
            free(load);
        }
        free(para->hreq);
    }
    free(para->file);
    free(para);
    printf("[**WEB**] %s\n", text);
    free(text);
}

static void zip_callback(char *name, char *data, long size, void *user) {
    if ((name = strstr(name, DEF_FLDR))
    && ((name += sizeof(DEF_FLDR) - 1)[0] == DEF_DSEP[0])) {
        name = MAKE_GET_QUERY(0, (char*)user, name);
        if (!size)
            rMakeDir(name, 0);
        else
            rSaveFile(name, data, size);
        free(name);
    }
}

static void load_callback(long size, intptr_t user) {
    ((main_window_t*)user)->set_progress_load(size >> 10, 0);
}

void try_get_from_github(main_window_t &mctl, const char *user,
        const char *auth, const char *proj, const char *bran,
        const char *repo, const char *disk) {
    #define GIT_SFIN "\""
    #define GIT_SURL GIT_SFIN "url"  GIT_SFIN ":" GIT_SFIN
    #define GIT_SSHA GIT_SFIN "sha"  GIT_SFIN ":" GIT_SFIN
    #define GIT_STYP GIT_SFIN "type" GIT_SFIN ":" GIT_SFIN
    #define GIT_SPTH GIT_SFIN "path" GIT_SFIN ":" GIT_SFIN
    #define GIT_SRAW "raw.githubusercontent.com"
    #define GIT_SAPI "api.github.com"

    char *temp, *file = 0, *path = 0, *text = 0, *tail = 0;
    long size, rlen, iter = 0, full = 0;
    intptr_t para, desc = 0;
    para_t *tmpl;

    if (!mctl.confirm_update()) return;
    if ((desc = rMakeHTTPS((char*)user, (char*)GIT_SAPI))
    &&  (iter = rMakeDir((char*)disk, 1))) {
        // no target directory present, need to populate it first
        mctl.set_progress_load(0, 0);
        temp = MAKE_GET_QUERY(0, "repos" DEF_DSEP, auth, DEF_DSEP,
                proj, DEF_DSEP "zipball" DEF_DSEP, bran);
        size = rLoadHTTPS(desc, temp, &path, load_callback, (intptr_t)&mctl);
        free(temp);
        if (path) {
            ZIP_Load(path, size, (void*)disk, zip_callback);
            free(path);
        }
    }
    rFreeHTTPS(desc);
    if (iter || (!path && !desc && !iter)) {
        if (!path && (!desc || iter)) mctl.report_update_failure(nullptr);
        return;
    }
    if ((desc = rMakeHTTPS((char*)user, (char*)GIT_SAPI))) {
        temp = MAKE_GET_QUERY(0, "repos" DEF_DSEP, auth, DEF_DSEP, proj,
                         DEF_DSEP "git" DEF_DSEP "trees" DEF_DSEP, bran);
        rLoadHTTPS(desc, temp, &text, 0, 0);
        free(temp);
        if (text) {
            size = sizeof(GIT_SPTH) + 1 + (rlen = strlen(repo));
            memmove(path = (char*)realloc(NULL, size),
                    GIT_SPTH, sizeof(GIT_SPTH) - 1);
            memmove(path + sizeof(GIT_SPTH) - 1, repo, rlen);
            path[size - 2] = *GIT_SFIN;
            path[size - 1] = 0;
            if ((temp = strstr(text, path)) && (temp = strstr(temp, GIT_SURL)))
                tail = strstr(temp += sizeof(GIT_SURL) - 1, GIT_SFIN);
            free(path);
            if (tail) {
                tail[0] = 0;
                tail = 0;
                if ((temp = strstr(temp, GIT_SAPI))) {
                    temp += sizeof(GIT_SAPI); // no -1: skipping the slash
                    temp = MAKE_GET_QUERY(0, temp, "?recursive=1");
                    rLoadHTTPS(desc, temp, &tail, 0, 0);
                    free(temp);
                }
            }
            free(text);
            text = (tail)? tail-- : 0;
        }
    }
    rFreeHTTPS(desc);
    if (text && (desc = rMakeHTTPS((char*)user, (char*)GIT_SRAW))) {
        while ((temp = strstr(tail + 1, GIT_SPTH))) {
            if ((tail = strstr(temp + sizeof(GIT_SPTH) - 1, GIT_SFIN))
            &&  (temp = strstr(tail + 1, GIT_STYP))
            &&  (tail = strstr(temp + sizeof(GIT_STYP) - 1, GIT_SFIN))
            &&  (temp = strstr(tail + 1, GIT_SSHA))
            &&  (tail = strstr(temp + sizeof(GIT_SSHA) - 1, GIT_SFIN)))
                full++;
            else
                break;
        }
        tail = text - 1;
        para = rMakeParallel(check_hash_and_download, 4);
        for (iter = 0; iter < full; iter++) {
            temp = strstr(tail + 1, GIT_SPTH);
            tail = strstr(temp += sizeof(GIT_SPTH) - 1, GIT_SFIN);
            tail[0] = 0;
            path = temp;

            temp = strstr(tail + 1, GIT_STYP);
            tail = strstr(temp += sizeof(GIT_STYP) - 1, GIT_SFIN);
            tail[0] = 0;
            if (strcmp(temp, "blob"))
                path = 0;

            if (path) {
                file = (char*)realloc(strdup(path), 2 + (rlen = strlen(disk))
                                                      + (size = strlen(path)));
                file[rlen] = *DEF_DSEP;
                memmove(file, disk, rlen);
                memmove(file + rlen + 1, path, size + 1);
                for (temp = file + 1; (temp = strstr(temp, DEF_DSEP)); ) {
                    *temp = 0;
                    if (!rMakeDir(file, 0)) {
                        mctl.report_update_failure(file);
                        free(file);
                        path = 0;
                        break;
                    }
                    *temp++ = *DEF_DSEP;
                }
            }
            temp = strstr(tail + 1, GIT_SSHA);
            tail = strstr(temp += sizeof(GIT_SSHA) - 1, GIT_SFIN);
            tail[0] = 0;
            if (path) {
                *(tmpl = (para_t*)realloc(NULL, sizeof(*tmpl))) =
                    {desc, temp, file,
                     MAKE_GET_QUERY(1, auth, DEF_DSEP, proj, DEF_DSEP,
                                       bran, DEF_DSEP, repo, DEF_DSEP, path)};
                rLoadParallel(para, (intptr_t)tmpl);
            }
            mctl.set_progress_update(iter, full);
        }
        rFreeParallel(para);
        free(text);
        rFreeHTTPS(desc);
    } else { // do NOT invert if/else, since conditions have side-effects
        mctl.report_update_failure(nullptr);
    }
    #undef GIT_SAPI
    #undef GIT_SRAW
    #undef GIT_SPTH
    #undef GIT_STYP
    #undef GIT_SSHA
    #undef GIT_SURL
    #undef GIT_SFIN
}
