#include "unit.h"



GLchar *name[] = {
    "glUseProgramObjectARB",
    "glGetProgramInfoLog",
    "glGetShaderInfoLog",
    "glGetProgramiv",
    "glGetShaderiv",
    "glCreateProgram",
    "glDeleteProgram",
    "glCreateShader",
    "glDeleteShader",
    "glAttachShader",
    "glShaderSource",
    "glCompileShader",
    "glLinkProgram",
    "glGetAttribLocation",
    "glGetUniformLocation",
    "glUniformMatrix4fv",
    "glUniform1iv",
    "glUniform1fv",
    "glUniform2uiv",
    "glUniform2fv",
    "glUniform3uiv",
    "glUniform3fv",
    "glEnableVertexAttribArray",
    "glVertexAttribPointer",
    "glVertexAttribIPointer",
    "glGenVertexArrays",
    "glBindVertexArray",
    "glDeleteVertexArrays",
    "glActiveTextureARB",
    "glGenBuffersARB",
    "glBindBufferARB",
    "glBufferDataARB",
    "glBufferSubDataARB",
    "glDeleteBuffersARB",
    "glGenFramebuffers",
    "glGenRenderbuffers",
    "glDeleteFramebuffers",
    "glDeleteRenderbuffers",
    "glBindFramebuffer",
    "glBindRenderbuffer",
    "glRenderbufferStorage",
    "glFramebufferTexture2D",
    "glFramebufferRenderbuffer",
    NULL
};
GLvoid *LoadedOpenGLFunctions[carrsz(name)];



long InitRendererOGL() {
    long iter = -1;

    while (name[++iter])
        if (!(LoadedOpenGLFunctions[iter] = GL_GET_PROC_ADDR(name[iter])))
            return 0;
    return 1;
}



FTEX *BindTex(FVBO *vobj, GLuint bind, GLuint mode) {
    GLuint itex, ttex, ktex = bind;
    FVBO *vtex = vobj;

    if (!vtex) return NULL;
    if (vtex->ptex[ktex].orig) {
        do {
            itex = vtex->ptex[ktex].indx;
            ttex = vtex->ptex[ktex].type;
            vtex = vtex->ptex[ktex].orig;
            ktex = itex;
            if (vtex == vobj) {
                printf("Texture circular cross-ref!\n");
                vtex = NULL;
                break;
            }
        } while (vtex->ptex[ktex].orig);
        vobj->ptex[bind].orig = vtex;
        vobj->ptex[bind].type = ttex;
        vobj->ptex[bind].indx = ktex;
    }
    if (mode == TEX_FRMB)
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                               GL_TEXTURE_2D, vtex->ptex[ktex].indx, 0);
    else if (mode == TEX_DFLT) {
        glActiveTexture(GL_TEXTURE0 + bind);
        glBindTexture(vtex->ptex[ktex].type, vtex->ptex[ktex].indx);
    }
    return &vtex->ptex[ktex];
}



GLvoid MakeTex(FTEX  *retn, GLuint xdim, GLuint ydim, GLuint  zdim,
               GLenum trgt, GLenum wrap, GLint  tmag, GLint   tmin,
               GLenum type, GLenum frmt, GLenum mode, GLvoid *data) {
    GLuint iter, iend;

    if (retn) {
        retn->xdim = xdim;
        retn->ydim = ydim;
        retn->zdim = zdim;
        retn->type = trgt;
        glGenTextures(1, &retn->indx);
        glBindTexture(trgt, retn->indx);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

        if (trgt == GL_TEXTURE_CUBE_MAP) {
            iend = 6 + (iter = GL_TEXTURE_CUBE_MAP_POSITIVE_X);
            glTexParameteri(trgt, GL_TEXTURE_WRAP_R, wrap);
        }
        else
            iend = iter = trgt;

        if (trgt == GL_TEXTURE_2D_ARRAY)
            glTexImage3D(trgt, 0, frmt, xdim, ydim, zdim, 0, mode, type, data);
        else
            for (; iter <= iend; iter++)
                glTexImage2D(iter, 0, frmt, xdim, ydim, 0, mode, type, data);

        glTexParameteri(trgt, GL_TEXTURE_MAG_FILTER, tmag);
        glTexParameteri(trgt, GL_TEXTURE_MIN_FILTER, tmin);
        glTexParameteri(trgt, GL_TEXTURE_WRAP_S, wrap);
        glTexParameteri(trgt, GL_TEXTURE_WRAP_T, wrap);
        glBindTexture(trgt, 0);
    }
}



FVBO *MakeVBO(FVBO *prev, GLchar *vshd[], GLchar *pshd[], GLenum elem,
              GLuint catr, UNIF *patr, GLuint cuni, UNIF *puni, GLuint ctex) {
    FVBO *retn = (prev)? prev : calloc(1, sizeof(*retn));
    GLint iter, shdr, ecnt, aloc;

    retn->elem = elem;
    retn->cvbo = catr;

    if (vshd && pshd && cuni && puni)
        retn->pshd = MakeShaderList(vshd, pshd, cuni, puni, &retn->cshd);
    if (ctex) {
        retn->ctex = ctex;
        retn->ptex = calloc(retn->ctex, sizeof(*retn->ptex));
    }

    glGenBuffers(catr, retn->pvbo = malloc(catr * sizeof(*retn->pvbo)));

    retn->cind = patr[0].cdat / sizeof(GLuint);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER_ARB, retn->pvbo[0]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER_ARB,
                 patr[0].cdat, patr[0].pdat, patr[0].draw);

    for (iter = 1; iter < catr; iter++) {
        glBindBuffer(GL_ARRAY_BUFFER_ARB, retn->pvbo[iter]);
        glBufferData(GL_ARRAY_BUFFER_ARB,
                     patr[iter].cdat, patr[iter].pdat, patr[iter].draw);
    }

    for (shdr = 0; shdr < retn->cshd; shdr++) {
        glUseProgramObjectARB(retn->pshd[shdr].prog);
        glBindVertexArray(retn->pshd[shdr].pvao);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER_ARB, retn->pvbo[0]);

        for (iter = 1; iter < catr; iter++) {
            if (patr[iter].name
            && (aloc = glGetAttribLocation(retn->pshd[shdr].prog,
                                           patr[iter].name)) != -1) {
                glBindBuffer(GL_ARRAY_BUFFER_ARB, retn->pvbo[iter]);
                ecnt = 0;
                switch (patr[iter].type) {
                    case UNI_T1IV:
                    case UNI_T1FV: ecnt = 1; break;

                    case UNI_T2UV:
                    case UNI_T2FV: ecnt = 2; break;

                    case UNI_T3UV:
                    case UNI_T3FV: ecnt = 3; break;
                }
                switch (patr[iter].type) {
                    case UNI_T1IV:
                    case UNI_T2UV:
                    case UNI_T3UV:
                        glVertexAttribIPointer(aloc, ecnt,
                                               GL_UNSIGNED_INT, 0, 0);
                        break;

                    case UNI_T1FV:
                    case UNI_T2FV:
                    case UNI_T3FV:
                        glVertexAttribPointer(aloc, ecnt,
                                              GL_FLOAT, GL_FALSE, 0, 0);
                        break;
                }
                glEnableVertexAttribArray(aloc);
            }
        }
    }
    glUseProgramObjectARB(0);
    glBindVertexArray(0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER_ARB, 0);
    glBindBuffer(GL_ARRAY_BUFFER_ARB, 0);
    return retn;
}



GLvoid DrawVBO(FVBO *vobj, GLuint shad) {
    GLuint i;

    if (shad < vobj->cshd) {
        glBindVertexArray(vobj->pshd[shad].pvao);
        glUseProgramObjectARB(vobj->pshd[shad].prog);

        for (i = 0; i < vobj->ctex; i++)
            BindTex(vobj, i, TEX_DFLT);

        for (i = 0; i < vobj->pshd[shad].cuni; i++)
            switch (vobj->pshd[shad].puni[i].type) {
                case UNI_T44F:
                    glUniformMatrix4fv((GLint)vobj->pshd[shad].puni[i].name,
                                          1 + vobj->pshd[shad].puni[i].cdat,
                                                                    GL_TRUE,
                                  (*(GLvoid**)vobj->pshd[shad].puni[i].pdat));
                    break;

                case UNI_T1II:
                    glUniform1iv((GLint)vobj->pshd[shad].puni[i].name,
                                    1 + vobj->pshd[shad].puni[i].cdat,
                              (GLvoid*)&vobj->pshd[shad].puni[i].pdat);
                    break;

                case UNI_T1FI:
                    glUniform1fv((GLint)vobj->pshd[shad].puni[i].name,
                                    1 + vobj->pshd[shad].puni[i].cdat,
                              (GLvoid*)&vobj->pshd[shad].puni[i].pdat);
                    break;

                case UNI_T1IV:
                    glUniform1iv((GLint)vobj->pshd[shad].puni[i].name,
                                    1 + vobj->pshd[shad].puni[i].cdat,
                                        vobj->pshd[shad].puni[i].pdat);
                    break;

                case UNI_T1FV:
                    glUniform1fv((GLint)vobj->pshd[shad].puni[i].name,
                                    1 + vobj->pshd[shad].puni[i].cdat,
                                        vobj->pshd[shad].puni[i].pdat);
                    break;

                case UNI_T2UV:
                    glUniform2uiv((GLint)vobj->pshd[shad].puni[i].name,
                                     1 + vobj->pshd[shad].puni[i].cdat,
                                         vobj->pshd[shad].puni[i].pdat);
                    break;

                case UNI_T2FV:
                    glUniform2fv((GLint)vobj->pshd[shad].puni[i].name,
                                    1 + vobj->pshd[shad].puni[i].cdat,
                                        vobj->pshd[shad].puni[i].pdat);
                    break;

                case UNI_T3UV:
                    glUniform3uiv((GLint)vobj->pshd[shad].puni[i].name,
                                     1 + vobj->pshd[shad].puni[i].cdat,
                                         vobj->pshd[shad].puni[i].pdat);
                    break;

                case UNI_T3FV:
                    glUniform3fv((GLint)vobj->pshd[shad].puni[i].name,
                                    1 + vobj->pshd[shad].puni[i].cdat,
                                        vobj->pshd[shad].puni[i].pdat);
                    break;
            }

        glDrawElements(vobj->elem, vobj->cind, GL_UNSIGNED_INT, NULL);
        glUseProgramObjectARB(0);
        glBindVertexArray(0);
    }
}



GLvoid FreeVBO(FVBO **vobj) {
    if (vobj && *vobj) {
        while ((*vobj)->cshd) {
            glDeleteVertexArrays(1, &(*vobj)->pshd[--(*vobj)->cshd].pvao);
            glDeleteProgram((*vobj)->pshd[(*vobj)->cshd].prog);
            free((*vobj)->pshd[(*vobj)->cshd].puni);
        };
        glDeleteBuffersARB((*vobj)->cvbo, (*vobj)->pvbo);
        free((*vobj)->pvbo);
        free((*vobj)->pshd);
        if ((*vobj)->ctex) {
            while ((*vobj)->ctex)
                if (!(*vobj)->ptex[--(*vobj)->ctex].orig)
                    glDeleteTextures(1, &(*vobj)->ptex[(*vobj)->ctex].indx);
            free((*vobj)->ptex);
        }
        free(*vobj);
        vobj = NULL;
    }
}



FRBO *MakeRBO(FVBO *vobj, GLuint tfrn, GLuint tbak) {
    FTEX *ftex, *btex;
    FRBO *retn;

    if (!vobj || (tfrn >= vobj->ctex) || (tbak >= vobj->ctex))
        return NULL;

    ftex = BindTex(vobj, tfrn, TEX_NSET);
    btex = BindTex(vobj, tbak, TEX_NSET);
    if ((ftex->xdim != btex->xdim) ||
        (ftex->ydim != btex->ydim))
        return NULL;

    retn = malloc(sizeof(*retn));
    retn->vobj = vobj;
    retn->tfrn = tfrn;
    retn->tbak = tbak;

    glGenFramebuffers(1, &retn->frmb);
    glGenRenderbuffers(1, &retn->rndb);
    glBindRenderbuffer(GL_RENDERBUFFER, retn->rndb);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16,
                          ftex->xdim, ftex->ydim);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
    return retn;
}



GLvoid DrawRBO(FRBO *robj, GLuint shad) {
    FTEX ttex, *ftex, *btex;
    GLint view[4];

    glGetIntegerv(GL_VIEWPORT, view);
    glBindFramebuffer(GL_FRAMEBUFFER, robj->frmb);
    glBindRenderbuffer(GL_RENDERBUFFER, robj->rndb);

    ttex = *(btex = BindTex(robj->vobj, robj->tbak, TEX_FRMB));
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                              GL_RENDERBUFFER, robj->rndb);
    glViewport(0, 0, btex->xdim, btex->ydim);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    DrawVBO(robj->vobj, shad);

    glBindRenderbuffer(GL_RENDERBUFFER, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(view[0], view[1], view[2], view[3]);

    *btex = *(ftex = BindTex(robj->vobj, robj->tfrn, TEX_NSET));
    *ftex = ttex;
}



GLvoid SwapRBO(FRBO *robj) {
    FTEX ttex, *ftex, *btex;

     ttex = *(btex = BindTex(robj->vobj, robj->tbak, TEX_NSET));
    *btex = *(ftex = BindTex(robj->vobj, robj->tfrn, TEX_NSET));
    *ftex = ttex;
}



GLvoid FreeRBO(FRBO **robj) {
    if (robj && *robj) {
        glDeleteRenderbuffers(1, &(*robj)->rndb);
        glDeleteFramebuffers(1, &(*robj)->frmb);
        free(*robj);
        *robj = NULL;
    }
}



typedef struct _TXSZ {
    GLint size, indx;
    GLubyte *bptr;
} TXSZ;

FVBO *surf = NULL;
T2FV  disz;



int sizecmp(const void *a, const void *b) {
    return ((TXSZ*)b)->size - ((TXSZ*)a)->size;
}



void MakeRendererOGL(ULIB *ulib, ulong uniq, T2UV *data, ulong size) {
    GLsizei bank, fill, curr, mtex, chei, phei;
    GLubyte *atex, *aptr;
    T4FV *dims, *coef;
    GLuint *indx;
    T3FV *vert;
    BGRA *apal;
    TXSZ *txsz;

    glCullFace(GL_BACK);
    glDisable(GL_CULL_FACE);
    glDepthFunc(GL_LESS);
    glEnable(GL_DEPTH_TEST);
//    glBlendFunc(GL_ONE_MINUS_DST_ALPHA, GL_DST_ALPHA);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);

    glClearColor(0.0, 0.0, 0.0, 0.0);

    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &mtex);
    MakeShaderSrc(log2(mtex = min(mtex, 4096)));

    chei = ceil(  1.0 * uniq / mtex);
    phei = ceil(256.0 * uniq / mtex);

    dims = calloc(mtex * chei, sizeof(*dims));
    coef = calloc(mtex * chei, sizeof(*coef));
    apal = calloc(mtex * phei, sizeof(*apal));

    indx = calloc(size, sizeof(*indx));
    vert = calloc(size, sizeof(*vert));

    txsz = calloc(uniq, sizeof(*txsz));

    while (ulib) {
        for (curr = 0; curr < ulib->ucnt; curr++) {
            ASTD *anim = ulib->uarr[curr]->anim;

            dims[ulib->uarr[curr]->uuid - 1] =
                (T4FV){anim->xdim, anim->ydim, 0, 0};
            coef[ulib->uarr[curr]->uuid - 1] =
                (T4FV){(1 << ulib->uarr[curr]->scal),
                       (1 << ulib->uarr[curr]->scal), 0, 0};
            memcpy(&apal[(ulib->uarr[curr]->uuid - 1) << 8],
                   anim->bpal, 256 * sizeof(BGRA));

            txsz[ulib->uarr[curr]->uuid - 1].indx = ulib->uarr[curr]->uuid;
            txsz[ulib->uarr[curr]->uuid - 1].bptr = anim->bptr;
            txsz[ulib->uarr[curr]->uuid - 1].size =
                anim->xdim * anim->ydim * anim->fcnt;
        }
        ulib = ulib->next;
    }
    qsort(txsz, uniq, sizeof(*txsz), sizecmp);

    for (bank =  1, fill = curr = 0; curr < uniq; curr++) {
        fill += txsz[curr].size;
        if (fill > mtex * mtex) {
            fill = txsz[curr].size;
            bank++;
        }
    }
    atex = aptr = calloc(mtex * mtex * bank, sizeof(*atex));

    for (bank = -1, fill = curr = 0; curr < uniq; curr++) {
        if (aptr + txsz[curr].size - atex > fill) {
            aptr = atex + fill;
            fill += mtex * mtex;
            bank++;
        }
        memcpy(aptr, txsz[curr].bptr, txsz[curr].size);
        dims[txsz[curr].indx - 1].z = bank;
        dims[txsz[curr].indx - 1].w = aptr - atex - fill + mtex * mtex;
        aptr += txsz[curr].size;
    }
    free(txsz);

    for (curr = 0; curr < size; curr++) {
        vert[curr] = (T3FV){(curr >> 1) & 1, ((curr + 1) >> 1) & 1, 0};
        indx[curr] = curr;
    }

    UNIF satr[] = {{/** No name/type for indices! **/ .pdat = indx,
                    .cdat = size * sizeof(*indx), .draw = GL_STATIC_DRAW_ARB},
                   {.name = "vert", .type = UNI_T3FV, .pdat = vert,
                    .cdat = size * sizeof(*vert), .draw = GL_STATIC_DRAW_ARB},
                   {.name = "data", .type = UNI_T2UV, .pdat = data,
                    .cdat = size * sizeof(*data), .draw = GL_STREAM_DRAW_ARB}},

         suni[] = {{.name = "atex", .type = UNI_T1II, .pdat = 0},
                   {.name = "apal", .type = UNI_T1II, .pdat = 1},
                   {.name = "dims", .type = UNI_T1II, .pdat = 2},
                   {.name = "coef", .type = UNI_T1II, .pdat = 3},
                   {.name = "disz", .type = UNI_T2FV, .pdat = &disz}};

    surf = MakeVBO(NULL, sver, spix, GL_QUADS,
                   carrsz(satr), satr, carrsz(suni), suni, 4);

    MakeTex(&surf->ptex[0], mtex, mtex, bank + 1,
            GL_TEXTURE_2D_ARRAY, GL_CLAMP_TO_EDGE, GL_NEAREST, GL_NEAREST,
            GL_UNSIGNED_BYTE, GL_R8UI, GL_RED_INTEGER, atex);

    MakeTex(&surf->ptex[1], mtex, phei, 0,
            GL_TEXTURE_2D, GL_CLAMP_TO_EDGE, GL_NEAREST, GL_NEAREST,
            GL_UNSIGNED_BYTE, GL_RGBA8, GL_BGRA, apal);

    MakeTex(&surf->ptex[2], mtex, chei, 0,
            GL_TEXTURE_2D, GL_CLAMP_TO_EDGE, GL_NEAREST, GL_NEAREST,
            GL_FLOAT, GL_RGBA32F, GL_RGBA, dims);

    MakeTex(&surf->ptex[3], mtex, chei, 0,
            GL_TEXTURE_2D, GL_CLAMP_TO_EDGE, GL_NEAREST, GL_NEAREST,
            GL_FLOAT, GL_RGBA32F, GL_RGBA, coef);

    free(vert);
    free(indx);

    free(atex);
    free(apal);
    free(dims);
    free(coef);

    FreeShaderSrc();
}



void SizeRendererOGL(ulong xscr, ulong yscr) {
    disz = (T2FV){2.0 / xscr, 2.0 / yscr};
    glViewport(0, 0, xscr, yscr);
}



void DrawRendererOGL(T2UV *data, ulong size) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glBindBufferARB(GL_ARRAY_BUFFER_ARB, surf->pvbo[2]);
    glBufferSubDataARB(GL_ARRAY_BUFFER_ARB, 0, size * sizeof(*data), data);
    DrawVBO(surf, 0);
}



void FreeRendererOGL() {
    FreeVBO(&surf);
}
