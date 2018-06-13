#include "load/ogl_load.h"
#include "oglstd.h"

struct RNDR {
    OGL_FVBO *surf;
    T4FV *temp;
    T4FV  disz;
    T4FV  hitd;
    T4FV  idep;
    long  size, mult;
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

typedef struct {
    GLint size, fcnt, indx;
    GLubyte *bptr;
} TXSZ;



int sizecmp(const void *a, const void *b) {
    return ((TXSZ*)b)->size - ((TXSZ*)a)->size;
}



void FreeRendererOGL(RNDR **rndr) {
    if (rndr && *rndr) {
        OGL_FreeVBO(&(*rndr)->surf);
        free((*rndr)->temp);
        free(*rndr);
        *rndr = 0;
    }
}



long MakeRendererOGL(RNDR **rndr, ulong rgba, UNIT *uarr,
                     ulong uniq, ulong size, ulong xscr, ulong yscr) {
    /** [main vertex shader] (FB = frame bank, CA = current animation)

    ===== dynamic uniforms: changes every frame
    T4FV <data>: x = X position of the sprite
                 y = Y position of the sprite
                 z = current sprite frame
                 w = [base index (22)] [Y inv (1)] [X inv (1)]

    ===== vertex attributes (marked {}) and static uniforms: no or few changes
    T2FV {vert}: x = current quad index, 1-based; sign: X coord, +0.5: Y coord
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
                 z = 1.0 / <apal> depth  (see pixel shader specs)
                 w = 0.0 ##### RESERVED, DO NOT CHANGE #####
    T4FV  idep : x = 1.0 / <dims> and <bank> height
                 y = 1.0 / <data> height
                 z = 1.0 / max texture size
                 w = 1.0 ##### RESERVED, DO NOT CHANGE #####

    ===== parameters for pixel shader: "=" means static, "~" means varying
    T4FV  vtex : x ~ current pixel U-pos (needs to be truncated)
                 y ~ current pixel V-pos (needs to be truncated)
                 z = frame width in pixels
                 w = 0.0 ##### RESERVED, DO NOT CHANGE #####
    T4FV  voff : x = current frame offset in FB
                 y = FB index
                 z = ???                                  ##### UNUSED #####
                 w = ???                                  ##### UNUSED #####
    T4FV  vdep : x = CA palette W-pos in texture, premult ##### UNUSED #####
                 y = CA palette W-pos in texture, premult
                 z = CA palette U-pos in texture + 0.5
                 w = CA palette V-pos in texture + 0.5
    **/
    char *MainVertexShader =
    "attribute float vert;"

    "uniform sampler2D data;"
    "uniform sampler2D dims;"
    "uniform sampler2D bank;"
    "uniform vec4 disz;"
    "uniform vec4 idep;"

    "varying vec4 vtex;"
    "varying vec4 voff;"
    "varying vec4 vdep;"

    "const mat4 coef = mat4(-1.0,   1.0,  0.5, 0.0,"
                           "-2.0,   4.0,  1.0, 0.0,"
                           "-0.125, 0.25, 0.5, 0.0,"
                           " 256.0, 1.0,  0.0, 0.0);"
    "void main() {"
        "vec4 vvec = vec4(abs(vert), vert, coef[0].ww);"
        "vec4 vdat = texture2D(data, idep.zy"
                  "* (floor(vvec.xx * idep.wz - idep.wz) + coef[0].zz));"
        "vec4 indx = vdat.wwww * coef[0].wwzw + idep.wzww * coef[1].zzxw"
                  "* floor(vdat.wwww * coef[2].yyyy + coef[0].xxww);"
        "vec4 iint = floor(indx.xyzy * coef[3].yyyx);"
        "vec4 offs = (iint.xyww * coef[0].yyww + coef[0].zzww) * idep.zxww"
                  "+ step(coef[2].yyyy, vvec.zzyx - floor(vvec.zzzx));"
        "vec4 vdim = texture2D(dims, offs.xy);"
        "vec4 vbnk = texture2D(bank, offs.xy);"
        "vtex = coef[1].yxxw * vdim.zwzw"
             "* ((iint.zzzz * coef[0].yxww + indx.zwww - coef[2].yzww)"
             "* (coef[2].zzww - offs.zwzz) - coef[2].xyzw);"
        "vdim = vdim.xyzw * vdim.zwww * coef[0].yyyw;"
        "indx = vdat.zzzz * vdim.zwww + vbnk.yxww;"
        "voff = (indx.x < vbnk.z)? indx.xyzw : (indx.xyzw"
             "+ floor((indx.xxxx - vbnk.zzzz) / vbnk.wwww)"
             "* (vbnk.wwww * coef[0].xwww + coef[0].wyww)"
             "+ (vbnk.zzzz * coef[0].xwww + coef[0].wyww));"
        "vdep = iint.wwxw * coef[3].wwxy + coef[0].wwzz"
             "+ disz.zzww * (floor(indx.wwww * idep.zzzz) + coef[0].zzzz);"
        "gl_Position = (offs.zwzz * vdim.xyww - coef[0].xyzw * vdat.xyyy)"
                    "*  disz.xyyy + coef[0].xyyy;"
    "}";

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
    T4FV  hitd : x = 1.0 / <atex> depth
                 y = 1.0 / <apal> height
                 z = 1.0 / max texture size
                 w = 1.0 ##### RESERVED, DO NOT CHANGE #####
    **/
    char *MainPixelShader =
    "uniform sampler3D atex;"
    "uniform sampler3D apal;"
    "uniform vec4 hitd;"

    "varying vec4 vtex;"
    "varying vec4 voff;"
    "varying vec4 vdep;"

    "const vec4 coef = vec4(1.0, 0.5, 255.0, 0.0);"

    "void main() {"
        "vec4 pixl = floor(vtex);"
        "pixl.xyz = hitd.wzw * (pixl.zzw * pixl.yyw + pixl.xxw + voff.xxy);"
        "pixl = texture3D(atex, hitd.zzx * (floor(pixl.xyz) + coef.yyy));"
        "if (pixl.x != coef.x)"
            "gl_FragColor = texture3D(apal, hitd.zyw *"
                                          "(pixl.xxx * coef.zww + vdep.zwy));"
        "else discard;"
    "}";

    GLsizei cbnk, fill, curr, mtex, chei, dhei, phei, pdep, fcnt, fend;
    GLubyte *atex, *aptr;
    GLfloat *vert;
    GLuint *indx = 0;
    T4FV *dims, *bank;
    TXSZ *txsz;
    BGRA *apal;
    RNDR *retn;
    OGL_FTEX *test;

    if (!rndr || *rndr)
        return !!rndr;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &mtex);
    mtex = ((mtex < 4096)? mtex : 4096) << 1;
    while (mtex >>= 1)
        if ((test = OGL_MakeTex(mtex, mtex, 1, GL_TEXTURE_3D,
                                GL_REPEAT, GL_NEAREST, GL_NEAREST,
                                GL_UNSIGNED_BYTE, GL_R8, GL_RED, 0)))
            break;
    OGL_FreeTex(&test);

    if (!mtex)
        return 0;

    glCullFace(GL_BACK);
    glEnable(GL_CULL_FACE);
    glDepthFunc(GL_LESS);
    glEnable(GL_DEPTH_TEST);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);
    glViewport(0, 0, xscr, yscr);

    dhei = ceil((GLfloat)size  / mtex) + 1;
    chei = ceil((GLfloat)uniq  / mtex);
    phei = ceil((256.0 * uniq) / mtex);
    phei = ((pdep = ceil((GLfloat)phei  / mtex)) <= 1)? phei : mtex;

    retn = calloc(1, sizeof(*retn));
    retn->disz = (T4FV){{2.0 / xscr, 2.0 / yscr, 1.0 / pdep, 0.0}};
    retn->idep = (T4FV){{1.0 / chei, 1.0 / dhei, 1.0 / mtex, 1.0}};

    /// allocate vertex arrays
//    indx = calloc(mtex * dhei * 6, sizeof(*indx)); /// GL_QUADS if commented
    vert = calloc(mtex * dhei * 4, sizeof(*vert));

    /// 4 * 4 = 16 MB per 2^20 sprites
    for (curr = mtex * dhei - 1; curr >= 0; curr--) {
        vert[curr * 4 + 0] = -1.5 - curr;
        vert[curr * 4 + 1] = -1.0 - curr;
        vert[curr * 4 + 2] = +1.0 + curr;
        vert[curr * 4 + 3] = +1.5 + curr;
    }
    if (indx) /// 6 * 4 = 24 MB per 2^20 sprites
        for (curr = mtex * dhei - 1; curr >= 0; curr--) {
            indx[curr * 6 + 0] = curr * 4 + 0;
            indx[curr * 6 + 1] = curr * 4 + 1;
            indx[curr * 6 + 2] = curr * 4 + 2;
            indx[curr * 6 + 3] = curr * 4 + 0;
            indx[curr * 6 + 4] = curr * 4 + 2;
            indx[curr * 6 + 5] = curr * 4 + 3;
        }
    retn->mult = (indx)? 6 : 4;
    fill = 6 * sizeof(*indx) * !!indx;
    curr = 4 * sizeof(*vert);

    OGL_UNIF satr[] =
        {{.cdat = mtex * dhei * fill, .draw = GL_STATIC_DRAW,
          /** No name and type for indices! **/ .pdat = indx},
         {.cdat = mtex * dhei * curr, .draw = GL_STATIC_DRAW,
          .name = "vert", .type = OGL_UNI_T1FV, .pdat = vert}};
    OGL_UNIF suni[] =
        {{.name = "data", .type = OGL_UNI_T1II, .pdat = (GLvoid*)0},
         {.name = "dims", .type = OGL_UNI_T1II, .pdat = (GLvoid*)1},
         {.name = "bank", .type = OGL_UNI_T1II, .pdat = (GLvoid*)2},
         {.name = "apal", .type = OGL_UNI_T1II, .pdat = (GLvoid*)3},
         {.name = "atex", .type = OGL_UNI_T1II, .pdat = (GLvoid*)4},
         {.name = "disz", .type = OGL_UNI_T4FV, .pdat = &retn->disz},
         {.name = "hitd", .type = OGL_UNI_T4FV, .pdat = &retn->hitd},
         {.name = "idep", .type = OGL_UNI_T4FV, .pdat = &retn->idep}};

    retn->surf = OGL_MakeVBO(5, (indx)? GL_TRIANGLES : GL_QUADS,
                             sizeof(satr) / sizeof(*satr), satr,
                             sizeof(suni) / sizeof(*suni), suni,
                             2, (char*[]){MainVertexShader, MainPixelShader});
    free(vert);
    free(indx);

    /// allocate the sprite data texture (uninitialized)
    *OGL_BindTex(retn->surf, 0, OGL_TEX_NSET) =
        OGL_MakeTex(mtex, dhei, 0, GL_TEXTURE_2D,
                    GL_REPEAT, GL_NEAREST, GL_NEAREST,
                    GL_FLOAT, GL_RGBA32F, GL_RGBA, 0);

    txsz = calloc(uniq, sizeof(*txsz));
    dims = calloc(mtex * chei, sizeof(*dims));
    apal = calloc(mtex * phei * pdep, sizeof(*apal));
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
    *OGL_BindTex(retn->surf, 3, OGL_TEX_NSET) =
        OGL_MakeTex(mtex, phei, pdep, GL_TEXTURE_3D,
                    GL_REPEAT, GL_NEAREST, GL_NEAREST, GL_UNSIGNED_BYTE,
                    GL_RGBA8, (rgba)? GL_RGBA : GL_BGRA, apal);
    free(apal);

    /// allocate the sprite dimension texture
    *OGL_BindTex(retn->surf, 1, OGL_TEX_NSET) =
        OGL_MakeTex(mtex, chei, 0, GL_TEXTURE_2D,
                    GL_REPEAT, GL_NEAREST, GL_NEAREST,
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
    test = *OGL_BindTex(retn->surf, 4, OGL_TEX_NSET) =
        OGL_MakeTex(mtex, mtex, cbnk + 1, GL_TEXTURE_3D,
                    GL_REPEAT, GL_NEAREST, GL_NEAREST,
                    GL_UNSIGNED_BYTE, GL_R8, GL_RED, 0);

    bank = calloc(mtex * chei, sizeof(*bank));
    atex = aptr = calloc(fill = mtex * mtex, sizeof(*atex));
    for (cbnk = fcnt = curr = 0; curr < uniq; fcnt = 0, curr++)
        while (txsz[curr].fcnt) {
            if (aptr - atex + txsz[curr].size > fill) {
                OGL_LoadTex(test, 0, 0, cbnk, mtex, mtex, 1, atex);
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
        OGL_LoadTex(test, 0, 0, cbnk, mtex, mtex, 1, atex);
    free(atex);

    /// allocate the FB control texture
    *OGL_BindTex(retn->surf, 2, OGL_TEX_NSET) =
        OGL_MakeTex(mtex, chei, 0, GL_TEXTURE_2D,
                    GL_REPEAT, GL_NEAREST, GL_NEAREST,
                    GL_FLOAT, GL_RGBA32F, GL_RGBA, bank);
    free(bank);

    retn->hitd = (T4FV){{1.0 / (cbnk + 1), 1.0 / phei, retn->idep.z, 1.0}};
    *rndr = retn;
    return ~0;
}



void DrawRendererOGL(RNDR *rndr, UNIT *uarr, T4FV *data,
                     ulong size, ulong opaq) {
    long iter, indx;
    GLuint xdim, ydim;
    OGL_FTEX *surf;
    T4FV *pfwd, *pbwd;
    UNIT *retn;

    xdim = 0;
    surf = *OGL_BindTex(rndr->surf, 0, OGL_TEX_NSET);
    OGL_EnumTex(surf, 0, 0, 0, &xdim, 0, 0);
    if (!xdim || !size)
        return;
    ydim = ceil((GLfloat)size / (GLfloat)xdim);
    if (rndr->size != (iter = xdim * ydim)) {
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
    OGL_LoadTex(surf, 0, 0, 0, xdim, ydim, 0, rndr->temp);
    OGL_DrawVBO(rndr->surf, 0, size * rndr->mult);
}



FRBO *MakeRBO(long xdim, long ydim) {
    FRBO *retn = calloc(1, sizeof(*retn));
    GLint data;

    retn->xdim = xdim;
    retn->ydim = ydim;
    retn->swiz = 0;

    glGenFramebuffers(1, &retn->fbuf);
    glBindFramebuffer(GL_FRAMEBUFFER, retn->fbuf);

    glGenRenderbuffers(2, retn->rbuf);
    glBindRenderbuffer(GL_RENDERBUFFER, retn->rbuf[0]);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA, retn->xdim, retn->ydim);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                              GL_RENDERBUFFER, retn->rbuf[0]);
    glBindRenderbuffer(GL_RENDERBUFFER, retn->rbuf[1]);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16,
                          retn->xdim, retn->ydim);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                              GL_RENDERBUFFER, retn->rbuf[1]);
    data = retn->xdim * retn->ydim * 4;

    glGenBuffers(2, retn->pbuf);
    glBindBuffer(GL_PIXEL_PACK_BUFFER, retn->pbuf[0]);
    glBufferData(GL_PIXEL_PACK_BUFFER, data, 0, GL_STREAM_READ);
    glBindBuffer(GL_PIXEL_PACK_BUFFER, retn->pbuf[1]);
    glBufferData(GL_PIXEL_PACK_BUFFER, data, 0, GL_STREAM_READ);
    glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, retn->fbuf);
    glViewport(0, 0, retn->xdim, retn->ydim);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    return retn;
}



void BindRBO(FRBO *robj, long bind) {
    GLuint buff = (bind)? robj->fbuf : 0;

    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, buff);
}



void ReadRBO(FRBO *robj, void *pict, ulong flgs) {
    GLvoid *bptr;

    if (flgs & WIN_IPBO)
        glBindBuffer(GL_PIXEL_PACK_BUFFER, robj->pbuf[robj->swiz]);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, robj->fbuf);
    glReadPixels(0, 0, robj->xdim, robj->ydim,
                (flgs & WIN_IBGR)? GL_BGRA : GL_RGBA,
                 GL_UNSIGNED_BYTE, (flgs & WIN_IPBO)? 0 : pict);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);

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
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glDeleteRenderbuffers(2, (*robj)->rbuf);
        glDeleteFramebuffers(1, &(*robj)->fbuf);
        glDeleteBuffers(2, (*robj)->pbuf);
        free(*robj);
        *robj = 0;
    }
}
