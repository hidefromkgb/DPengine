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
    "glMapBufferARB",
    "glUnmapBufferARB",
    "glTexImage3D",
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
        glActiveTextureARB(GL_TEXTURE0 + bind);
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

    glGenBuffersARB(catr, retn->pvbo = malloc(catr * sizeof(*retn->pvbo)));

    retn->cind = patr[0].cdat / sizeof(GLuint);
    glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, retn->pvbo[0]);
    glBufferDataARB(GL_ELEMENT_ARRAY_BUFFER_ARB,
                    patr[0].cdat, patr[0].pdat, patr[0].draw);

    for (iter = 1; iter < catr; iter++) {
        glBindBufferARB(GL_ARRAY_BUFFER_ARB, retn->pvbo[iter]);
        glBufferDataARB(GL_ARRAY_BUFFER_ARB,
                        patr[iter].cdat, patr[iter].pdat, patr[iter].draw);
    }

    for (shdr = 0; shdr < retn->cshd; shdr++) {
        glUseProgramObjectARB(retn->pshd[shdr].prog);
        glBindVertexArray(retn->pshd[shdr].pvao);
        glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, retn->pvbo[0]);

        for (iter = 1; iter < catr; iter++) {
            if (patr[iter].name
            && (aloc = glGetAttribLocation(retn->pshd[shdr].prog,
                                           patr[iter].name)) != -1) {
                glBindBufferARB(GL_ARRAY_BUFFER_ARB, retn->pvbo[iter]);
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
    glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, 0);
    glBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
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



typedef struct _TXSZ {
    GLint size, fcnt, indx;
    GLubyte *bptr;
} TXSZ;

FVBO *surf = NULL;
T2FV  disz;



int sizecmp(const void *a, const void *b) {
    return ((TXSZ*)b)->size - ((TXSZ*)a)->size;
}



void MakeRendererOGL(ULIB  *ulib, ulong uniq,
                     T2UV **data, ulong size, ulong rgba) {
    GLsizei cbnk, fill, curr, mtex, chei, phei, dhei, fcnt, fend;
    GLubyte *atex, *aptr;
    T4FV *dims, *bank;
    GLuint *indx;
    TXSZ *txsz;
    BGRA *apal,
          temp;

    glCullFace(GL_BACK);
    glDisable(GL_CULL_FACE);
    glDepthFunc(GL_LESS);
    glEnable(GL_DEPTH_TEST);
//    glBlendFunc(GL_ONE_MINUS_DST_ALPHA, GL_DST_ALPHA);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);

    glClearColor(0.0, 0.0, 0.0, 0.0);

    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &mtex);
    MakeShaderSrc(log2l(mtex = min(mtex, 4096)));

    dhei = ceil((GLfloat)size  / mtex) + 1;
    chei = ceil((GLfloat)uniq  / mtex);
    phei = ceil((256.0 * uniq) / mtex);

    dims = calloc(mtex * chei, sizeof(*dims));
    bank = calloc(mtex * chei, sizeof(*bank));
    apal = calloc(mtex * phei, sizeof(*apal));

    *data = calloc(mtex * dhei, sizeof(**data));
    indx = calloc(mtex * dhei * 4, sizeof(*indx));

    txsz = calloc(uniq, sizeof(*txsz));

    while (ulib) {
        for (curr = 0; curr < ulib->ucnt; curr++)
            if (!ulib->uarr[curr]->orig) {
                ASTD *anim = ulib->uarr[curr]->anim;

                txsz[ulib->uarr[curr]->uuid - 1] =
                    (TXSZ){anim->xdim * anim->ydim, anim->fcnt,
                           ulib->uarr[curr]->uuid,  anim->bptr};
                dims[ulib->uarr[curr]->uuid - 1] =
                    (T4FV){(1 << ulib->uarr[curr]->scal),
                           (1 << ulib->uarr[curr]->scal),
                           anim->xdim, anim->ydim};
                memcpy(&apal[(ulib->uarr[curr]->uuid - 1) << 8],
                       anim->bpal, 256 * sizeof(BGRA));
            }
        ulib = ulib->next;
    }
    if (rgba)
        for (curr = (uniq << 8) - 1; curr >= 0; curr--) {
            temp = apal[curr];
            apal[curr].R = temp.B;
            apal[curr].B = temp.R;
        }
    qsort(txsz, uniq, sizeof(*txsz), sizecmp);

    for (cbnk =  1, fill = curr = 0; curr < uniq; curr++) {
        fcnt = txsz[curr].fcnt;
        while (fcnt) {
            if (fill + txsz[curr].size > mtex * mtex) {
                fill = 0;
                cbnk++;
            }
            fend = min((GLfloat)fcnt,
                       (GLfloat)(mtex * mtex - fill) / txsz[curr].size);
            fill += txsz[curr].size * fend;
            fcnt -= fend;
        }
    }
    atex = aptr = calloc(mtex * mtex * cbnk, sizeof(*atex));

    for (fill = mtex * mtex, cbnk = curr = 0; curr < uniq; curr++) {
        fcnt = GL_FALSE;
        while (txsz[curr].fcnt) {
            if (aptr + txsz[curr].size - atex > fill) {
                cbnk++;
                aptr = atex + fill;
                fill += mtex * mtex;
            }
            fend = min((GLfloat)txsz[curr].fcnt,
                       (GLfloat)(atex + fill - aptr) / txsz[curr].size);
            if (!fcnt) {
                bank[txsz[curr].indx - 1].x = cbnk;
                bank[txsz[curr].indx - 1].y = aptr - atex - fill + mtex * mtex;
                bank[txsz[curr].indx - 1].z =
                    bank[txsz[curr].indx - 1].y + fend * txsz[curr].size;
                bank[txsz[curr].indx - 1].w =
                    mtex * mtex - (mtex * mtex % txsz[curr].size);
                fcnt = GL_TRUE;
            }
            memcpy(aptr, txsz[curr].bptr, txsz[curr].size * fend);
            txsz[curr].bptr += txsz[curr].size * fend;
            aptr += txsz[curr].size * fend;
            txsz[curr].fcnt -= fend;
        }
    }
    free(txsz);

    for (curr = mtex * dhei * 4 - 1; curr >= 0; curr--)
        indx[curr] = curr;

    UNIF satr[] = {{.pdat = indx, .cdat = mtex * dhei * 4 * sizeof(*indx),
                    .draw = GL_STATIC_DRAW_ARB}},
         suni[] = {{.name = "data", .type = UNI_T1II, .pdat = (GLvoid*)0},
                   {.name = "atex", .type = UNI_T1II, .pdat = (GLvoid*)1},
                   {.name = "apal", .type = UNI_T1II, .pdat = (GLvoid*)2},
                   {.name = "dims", .type = UNI_T1II, .pdat = (GLvoid*)3},
                   {.name = "bank", .type = UNI_T1II, .pdat = (GLvoid*)4},
                   {.name = "disz", .type = UNI_T2FV, .pdat = &disz}};

    surf = MakeVBO(NULL, sver, spix, GL_QUADS,
                   carrsz(satr), satr, carrsz(suni), suni, 5);
    surf->cind = size * 4;

    MakeTex(&surf->ptex[0], mtex, dhei, 0,
            GL_TEXTURE_2D, GL_CLAMP_TO_EDGE, GL_NEAREST, GL_NEAREST,
            GL_UNSIGNED_INT, GL_RG32UI, GL_RG_INTEGER, *data);

    MakeTex(&surf->ptex[1], mtex, mtex, cbnk + 1,
            GL_TEXTURE_2D_ARRAY, GL_CLAMP_TO_EDGE, GL_NEAREST, GL_NEAREST,
            GL_UNSIGNED_BYTE, GL_R8UI, GL_RED_INTEGER, atex);

    MakeTex(&surf->ptex[2], mtex, phei, 0,
            GL_TEXTURE_2D, GL_CLAMP_TO_EDGE, GL_NEAREST, GL_NEAREST,
            GL_UNSIGNED_BYTE, GL_RGBA8, GL_BGRA, apal);

    MakeTex(&surf->ptex[3], mtex, chei, 0,
            GL_TEXTURE_2D, GL_CLAMP_TO_EDGE, GL_NEAREST, GL_NEAREST,
            GL_FLOAT, GL_RGBA32F, GL_RGBA, dims);

    MakeTex(&surf->ptex[4], mtex, chei, 0,
            GL_TEXTURE_2D, GL_CLAMP_TO_EDGE, GL_NEAREST, GL_NEAREST,
            GL_FLOAT, GL_RGBA32F, GL_RGBA, bank);

    free(indx);

    free(atex);
    free(apal);
    free(dims);
    free(bank);

    FreeShaderSrc();
}



void SizeRendererOGL(ulong xscr, ulong yscr) {
    disz = (T2FV){2.0 / xscr, 2.0 / yscr};
    glViewport(0, 0, xscr, yscr);
}



void DrawRendererOGL(T2UV *data, ulong size) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    BindTex(surf, 0, TEX_DFLT);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, surf->ptex[0].xdim,
                    ceil((GLfloat)size / surf->ptex[0].xdim),
                    GL_RG_INTEGER, GL_UNSIGNED_INT, data);
    DrawVBO(surf, 0);
}



void FreeRendererOGL(T2UV *data) {
    FreeVBO(&surf);
    free(data);
}
