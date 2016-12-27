#include "load/ogl_load.h"
#include "oglstd.h"

struct RNDR {
    FVBO *surf;
    T4FV *temp;
    T4FV  disz;
    T4FV  hitd;
    long  size;
};

/// renderbuffer-based framebuffer object, opaque outside the module
struct FRBO {
    GLuint fbuf,    /// framebuffer
           rbuf[2], /// renderbuffers for pixel and depth data
           pbuf[2]; /// pixel-transfer buffer array
    GLint  xdim,    /// width
           ydim,    /// height
           swiz;    /// pixel buffer switcher
};



/** [main vertex shader] (FB = frame bank, CA = current animation)

    ===== dynamic uniforms: changes every frame
    T4FV <data>: x = X position of the sprite
                 y = Y position of the sprite
                 z = current sprite frame
                 w = [base index (22)] [Y inv (1)] [X inv (1)]

    ===== vertex attributes (marked {}) and static uniforms: no or few changes
    T3FV {vert}: x = X quad coordinates, either 0 or 1  -->  / (0, 1) (1, 1) \
                 y = Y quad coordinates, either 0 or 1  -->  \ (0, 0) (1, 0) /
                 z = index of the current quad (0-based)
    T4FV <dims>: x = frame X scale
                 y = frame Y scale
                 z = frame width
                 w = frame height
    T4FV <bank>: x = FB index
                 y = CA`s first frame offset in FB
                 z = end of CA block in FB
                 w = end of FB as if it all consisted of CA frames
    T4FV  disz : x = 2.0 / screen width
                 y = 2.0 / screen height
                 z = 1.0 / <data> height
                 w = 1.0 / <dims> and <bank> height
    T4FV  hitd : x = 1.0 / <atex> depth  (see pixel shader specs)
                 y = 1.0 / <apal> height (see pixel shader specs)
                 z = 1.0 / max texture size
                 w = 1.0 ##### RESERVED, DO NOT CHANGE #####

    ===== parameters for pixel shader: "=" means static, "~" means varying
    T4FV  vtex : x ~ current pixel U-pos (needs to be truncated)
                 y ~ current pixel V-pos (needs to be truncated)
                 z = frame width in pixels
                 w = 0.0 ##### RESERVED, DO NOT CHANGE #####
    T4FV  voff : x = current frame offset in FB
                 y = FB index
                 z = CA palette U-pos in texture + 0.5
                 w = CA palette V-pos in texture + 0.5
 **/
INCBIN("../core/ogl/shad/vert.glsl", MainVertexShader);
char *tver[] = {MainVertexShader, 0};



/** [main pixel shader] (FB = frame bank, CA = current animation)

    ===== basic concepts and theory
               _________    __________________   This is a frame bank array on
             .|       . |  |_#K_|             |  the left & a single FB on the
           .  |     .   |  | animation #(K+1) |  right. Each FB is composed of
         .____|___.     |  |             _____|  animations, which in turn are
       _|_______  |    n|  |____________|     |  made of consecutive frames in
     _|_______  | |_____|  | animation #(K+2) |  linear form, unlike textures.
    |         | | |   .    |                __|  Some of the animations do not
    |         | |2| .      |_______________|  |  fit in a single FB, occupying
    |         |1|_|------->| animation #(K+3) |  space in several FBs. In this
    |        0|_|          |       ___________|  case, they are split frame by
    |_________|            |______|///EMPTY///|  frame in several parts. As an
                                                 illustration, animation #K is
    only partially contained in the second FB: say, one or two last frames. FB
    dimensions are chosen to match GPU capabilities, but they cannot be larger
    than 4096*4096. This restriction comes from many factors like inability of
    GLfloat to store consecutive integers larger than 2^24, or excess unneeded
    space consumption when only a part of the last FB is actually used.

    ===== vertex attributes (marked {}) and static uniforms: no or few changes
    T1FV <atex>: FB array, each layer is an FB with palette indices; see above
    T4FV <apal>: palette texture
 **/
INCBIN("../core/ogl/shad/pixl.glsl", MainPixelShader);
char *tpix[] = {MainPixelShader, 0};



typedef struct _TXSZ {
    GLint size, fcnt, indx;
    GLubyte *bptr;
} TXSZ;



int sizecmp(const void *a, const void *b) {
    return ((TXSZ*)b)->size - ((TXSZ*)a)->size;
}



void FreeRendererOGL(RNDR **rndr) {
    if (rndr && *rndr) {
        FreeVBO(&(*rndr)->surf);
        free((*rndr)->temp);
        free(*rndr);
        *rndr = 0;
    }
}



long MakeRendererOGL(RNDR **rndr, ulong rgba, UNIT *uarr,
                     ulong uniq, ulong size, ulong xscr, ulong yscr) {
    GLsizei cbnk, fill, curr, mtex, chei, phei, dhei, fcnt, fend;
    GLubyte *atex, *aptr;
    T4FV *dims, *bank;
    GLuint *indx;
    T3FV *vert;
    TXSZ *txsz;
    BGRA *apal;
    FTEX *test;
    RNDR *retn;

    if (!rndr || *rndr)
        return !!rndr;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &mtex);
    test = calloc(1, sizeof(*test));
    mtex = (mtex < 4096)? mtex : 4096;
    while (mtex) {
        curr = MakeTex(test, mtex, mtex, 1,
                       GL_TEXTURE_3D, GL_CLAMP_TO_EDGE, GL_NEAREST, GL_NEAREST,
                       GL_UNSIGNED_BYTE, GL_R8, GL_RED, 0);
        glDeleteTextures(1, &test->indx);
        if (curr == GL_NO_ERROR)
            break;
        mtex >>= 1;
    }
    free(test);

    if (!mtex)
        return 0;

    glCullFace(GL_BACK);
    glEnable(GL_CULL_FACE);
    glDepthFunc(GL_LESS);
    glEnable(GL_DEPTH_TEST);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);

    retn = calloc(1, sizeof(*retn));

    retn->disz.x = 2.0 / xscr;
    retn->disz.y = 2.0 / yscr;
    glViewport(0, 0, xscr, yscr);

    dhei = ceil((GLfloat)size  / mtex) + 1;
    chei = ceil((GLfloat)uniq  / mtex);
    phei = ceil((256.0 * uniq) / mtex);

    /// allocate vertex arrays
    indx = calloc(mtex * dhei * 6, sizeof(*indx));
    vert = calloc(mtex * dhei * 4, sizeof(*vert));
    for (curr = mtex * dhei - 1; curr >= 0; curr--) {
        vert[curr * 4 + 0] = (T3FV){{0, 1, curr}};
        vert[curr * 4 + 1] = (T3FV){{0, 0, curr}};
        vert[curr * 4 + 2] = (T3FV){{1, 0, curr}};
        vert[curr * 4 + 3] = (T3FV){{1, 1, curr}};
        indx[curr * 6 + 0] = curr * 4 + 0;
        indx[curr * 6 + 1] = curr * 4 + 1;
        indx[curr * 6 + 2] = curr * 4 + 2;
        indx[curr * 6 + 3] = curr * 4 + 0;
        indx[curr * 6 + 4] = curr * 4 + 2;
        indx[curr * 6 + 5] = curr * 4 + 3;
    }
    fill = 6 * sizeof(*indx);
    curr = 4 * sizeof(*vert);

    UNIF satr[] = {{.cdat = mtex * dhei * fill, .draw = GL_STATIC_DRAW,
                    /** No name/type for indices! **/ .pdat = indx},
                   {.cdat = mtex * dhei * curr, .draw = GL_STATIC_DRAW,
                    .name = "vert", .type = UNI_T3FV, .pdat = vert}},
         suni[] = {{.name = "data", .type = UNI_T1II, .pdat = (GLvoid*)0},
                   {.name = "dims", .type = UNI_T1II, .pdat = (GLvoid*)1},
                   {.name = "bank", .type = UNI_T1II, .pdat = (GLvoid*)2},
                   {.name = "apal", .type = UNI_T1II, .pdat = (GLvoid*)3},
                   {.name = "atex", .type = UNI_T1II, .pdat = (GLvoid*)4},
                   {.name = "disz", .type = UNI_T4FV, .pdat = &retn->disz},
                   {.name = "hitd", .type = UNI_T4FV, .pdat = &retn->hitd}};

    retn->surf = MakeVBO(5, GL_TRIANGLES, countof(satr), satr,
                         countof(suni), suni, tver, tpix);
    retn->surf->cind = 0;
    retn->disz.z = 1.0 / dhei;
    retn->disz.w = 1.0 / chei;
    free(vert);
    free(indx);

    /// allocate the sprite data texture (uninitialized)
    MakeTex(&retn->surf->ptex[0], mtex, dhei, 0,
            GL_TEXTURE_2D, GL_REPEAT, GL_NEAREST, GL_NEAREST,
            GL_FLOAT, GL_RGBA32F, GL_RGBA, 0);

    txsz = calloc(uniq, sizeof(*txsz));
    dims = calloc(mtex * chei, sizeof(*dims));
    apal = calloc(mtex * phei, sizeof(*apal));
    for (curr = 1; curr <= uniq; curr++)
        if (uarr[curr].anim) {
            ASTD *anim = uarr[curr].anim;
            txsz[curr - 1] =
                (TXSZ){anim->xdim * anim->ydim, anim->fcnt, curr, anim->bptr};
            dims[curr - 1] =
                (T4FV){{1 << uarr[curr].scal, 1 << uarr[curr].scal,
                        anim->xdim, anim->ydim}};
            memcpy(&apal[(curr - 1) << 8], anim->bpal, 256 * sizeof(*apal));
        }

    /// allocate the palette texture
    MakeTex(&retn->surf->ptex[3], mtex, phei, 0,
            GL_TEXTURE_2D, GL_REPEAT, GL_NEAREST, GL_NEAREST,
            GL_UNSIGNED_BYTE, GL_RGBA8, (rgba)? GL_RGBA : GL_BGRA, apal);
    free(apal);

    /// allocate the sprite dimension texture
    MakeTex(&retn->surf->ptex[1], mtex, chei, 0,
            GL_TEXTURE_2D, GL_REPEAT, GL_NEAREST, GL_NEAREST,
            GL_FLOAT, GL_RGBA32F, GL_RGBA, dims);
    free(dims);

    qsort(txsz, uniq, sizeof(*txsz), sizecmp);
    for (cbnk = fill = curr = 0; curr < uniq; curr++) {
        fcnt = txsz[curr].fcnt;
        while (fcnt) {
            if (fill + txsz[curr].size > mtex * mtex) {
                fill = 0;
                cbnk++;
            }
            fend = (mtex * mtex - fill) / txsz[curr].size;
            fend = (fcnt < fend)? fcnt : fend;
            fill += txsz[curr].size * fend;
            fcnt -= fend;
        }
    }
    /// allocate the main FB array texture (uninitialized)
    MakeTex(&retn->surf->ptex[4], mtex, mtex, cbnk + 1,
            GL_TEXTURE_3D, GL_REPEAT, GL_NEAREST, GL_NEAREST,
            GL_UNSIGNED_BYTE, GL_R8, GL_RED, 0);

    bank = calloc(mtex * chei, sizeof(*bank));
    atex = aptr = calloc(fill = mtex * mtex, sizeof(*atex));
    for (cbnk = fcnt = curr = 0; curr < uniq; fcnt = 0, curr++)
        while (txsz[curr].fcnt) {
            if (aptr - atex + txsz[curr].size > fill) {
                LoadTex(&retn->surf->ptex[4], 0, 0, cbnk, mtex, mtex, 1, atex);
                aptr = atex;
                cbnk++;
            }
            /// final frame that may be safely allocated in the current FB
            fend = (atex + fill - aptr) / txsz[curr].size;
            fend = (txsz[curr].fcnt < fend)? txsz[curr].fcnt : fend;
            if (!fcnt) {
                /// this is the first time we allocate frames for the CA,
                /// so let`s add its header to the frame header bank
                bank[txsz[curr].indx - 1].x = cbnk;
                bank[txsz[curr].indx - 1].y = aptr - atex;
                bank[txsz[curr].indx - 1].z =
                    bank[txsz[curr].indx - 1].y + fend * txsz[curr].size;
                bank[txsz[curr].indx - 1].w = fill - (fill % txsz[curr].size);
                fcnt = ~0;
            }
            memcpy(aptr, txsz[curr].bptr, txsz[curr].size * fend);
            txsz[curr].bptr += txsz[curr].size * fend;
            aptr += txsz[curr].size * fend;
            txsz[curr].fcnt -= fend;
        }
    free(txsz);
    if (aptr > atex)
        LoadTex(&retn->surf->ptex[4], 0, 0, cbnk, mtex, mtex, 1, atex);
    free(atex);

    /// allocate the FB control texture
    MakeTex(&retn->surf->ptex[2], mtex, chei, 0,
            GL_TEXTURE_2D, GL_REPEAT, GL_NEAREST, GL_NEAREST,
            GL_FLOAT, GL_RGBA32F, GL_RGBA, bank);
    free(bank);

    retn->hitd = (T4FV){{1.0 / (cbnk + 1), 1.0 / phei, 1.0 / mtex, 1.0}};
    *rndr = retn;
    return ~0;
}



void DrawRendererOGL(RNDR *rndr, UNIT *uarr, T4FV *data,
                     ulong size, ulong opaq) {
    long iter, indx, ydim = ceil((GLfloat)size / rndr->surf->ptex[0].xdim);
    T4FV *pfwd, *pbwd;
    UNIT *retn;

    if (!(iter = rndr->surf->ptex[0].xdim * ydim))
        return;
    if (rndr->size != iter) {
        free(rndr->temp);
        rndr->size = iter;
        rndr->temp = malloc(rndr->size * sizeof(*rndr->temp));
    }
    pbwd = (pfwd = rndr->temp) + size;
    for (iter = 0; iter < size; iter++) {
        retn = &uarr[(indx = (long)data[iter].w) >> 2];
        /// opaque sprites go to the beginning, translucent ones to the end,
        /// applying offsets if a sprite is larger than its actual content
        /// [TODO:] somehow overcome the depth priority bug when OFFS.y > 0
        *((retn->tran)? --pbwd : pfwd++) =
            (T4FV){{data[iter].x + retn->offs[indx & 1],
                    data[iter].y - retn->offs[3 - ((indx >> 1) & 1)],
                    data[iter].z, data[iter].w}};
    }
    glClearColor(0.0, 0.0, 0.0, (opaq)? 1.0 : 0.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    LoadTex(&rndr->surf->ptex[0], 0, 0, 0,
            rndr->surf->ptex[0].xdim, ydim, 0, rndr->temp);
    rndr->surf->cind = size * 6;
    DrawVBO(rndr->surf, 0);
}



FRBO *MakeRBO(long xdim, long ydim) {
    FRBO *retn = calloc(1, sizeof(*retn));
    GLint data;

    retn->xdim = xdim;
    retn->ydim = ydim;
    retn->swiz = 0;

    glGenFramebuffersEXT(1, &retn->fbuf);
    glBindFramebufferEXT(GL_FRAMEBUFFER, retn->fbuf);

    glGenRenderbuffersEXT(2, retn->rbuf);
    glBindRenderbufferEXT(GL_RENDERBUFFER, retn->rbuf[0]);
    glRenderbufferStorageEXT(GL_RENDERBUFFER, GL_RGBA,
                             retn->xdim, retn->ydim);
    glFramebufferRenderbufferEXT(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                 GL_RENDERBUFFER, retn->rbuf[0]);
    glBindRenderbufferEXT(GL_RENDERBUFFER, retn->rbuf[1]);
    glRenderbufferStorageEXT(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16,
                             retn->xdim, retn->ydim);
    glFramebufferRenderbufferEXT(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                                 GL_RENDERBUFFER, retn->rbuf[1]);
    data = retn->xdim * retn->ydim * 4;

    glGenBuffers(2, retn->pbuf);
    glBindBuffer(GL_PIXEL_PACK_BUFFER, retn->pbuf[0]);
    glBufferData(GL_PIXEL_PACK_BUFFER, data, 0, GL_STREAM_READ);
    glBindBuffer(GL_PIXEL_PACK_BUFFER, retn->pbuf[1]);
    glBufferData(GL_PIXEL_PACK_BUFFER, data, 0, GL_STREAM_READ);
    glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
    glBindFramebufferEXT(GL_FRAMEBUFFER, 0);

    glBindFramebufferEXT(GL_DRAW_FRAMEBUFFER, retn->fbuf);
    glViewport(0, 0, retn->xdim, retn->ydim);
    glBindFramebufferEXT(GL_FRAMEBUFFER, 0);
    return retn;
}



void BindRBO(FRBO *robj, long bind) {
    GLuint buff = (bind)? robj->fbuf : 0;

    glBindFramebufferEXT(GL_DRAW_FRAMEBUFFER, buff);
}



void ReadRBO(FRBO *robj, void *pict, ulong flgs) {
    GLvoid *bptr;

    if (flgs & WIN_IPBO)
        glBindBuffer(GL_PIXEL_PACK_BUFFER, robj->pbuf[robj->swiz]);
    glBindFramebufferEXT(GL_READ_FRAMEBUFFER, robj->fbuf);
    glReadPixels(0, 0, robj->xdim, robj->ydim,
                (flgs & WIN_IBGR)? GL_BGRA : GL_RGBA,
                 GL_UNSIGNED_BYTE, (flgs & WIN_IPBO)? 0 : pict);
    glBindFramebufferEXT(GL_READ_FRAMEBUFFER, 0);

    if (flgs & WIN_IPBO) {
        robj->swiz ^= 1;
        glBindBuffer(GL_PIXEL_PACK_BUFFER, robj->pbuf[robj->swiz]);
        bptr = glMapBuffer(GL_PIXEL_PACK_BUFFER, GL_READ_ONLY);
        if (bptr) {
            memcpy(pict, bptr, robj->xdim * robj->ydim * sizeof(BGRA));
            glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
        }
        glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
    }
}



void FreeRBO(FRBO **robj) {
    if (robj && *robj) {
        glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
        glBindFramebufferEXT(GL_FRAMEBUFFER, 0);
        glDeleteRenderbuffersEXT(2, (*robj)->rbuf);
        glDeleteFramebuffersEXT(1, &(*robj)->fbuf);
        glDeleteBuffers(2, (*robj)->pbuf);
        free(*robj);
        *robj = 0;
    }
}
