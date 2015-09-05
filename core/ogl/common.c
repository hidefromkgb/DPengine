#include <stdarg.h>
#include "common.h"



struct {
    GLchar *name;
    GLuint  mask;
} MaskedStringOpenGLFunctions[] = {MASKED_STRING_OPENGL_FUNCTIONS, {}};
GLvoid *LoadedOpenGLFunctions[countof(MaskedStringOpenGLFunctions)] = {};



GLvoid newerror(GLchar **retn, GLchar *frmt, ...) {
    GLchar buff[2048];
    va_list list;

    va_start(list, frmt);
    vsnprintf(buff, countof(buff), frmt, list);
    va_end(list);
    if (*retn) {
        *retn = realloc(*retn, 1 + strlen(*retn) + strlen(buff));
        strcat(*retn, buff);
    }
    else
        *retn = strdup(buff);
}



GLchar *LoadOpenGLFunctions(GLuint mask) {
    GLchar *retn = 0;
    GLint iter = -1;

    while (MaskedStringOpenGLFunctions[++iter].name) {
        if (LoadedOpenGLFunctions[iter] ||
           (MaskedStringOpenGLFunctions[iter].mask &&
          !(MaskedStringOpenGLFunctions[iter].mask & mask)))
            continue;
        if (!(LoadedOpenGLFunctions[iter] =
              GL_GET_PROC_ADDR(MaskedStringOpenGLFunctions[iter].name)))
            newerror(&retn, "[%d]: %s\n",
                     iter, MaskedStringOpenGLFunctions[iter].name);
    }
    if (mask & NV_vertex_program3) {
        glGetIntegerv(GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS, &iter);
        if (!iter)
            newerror(&retn, "NV_vertex_program3\n");
    }
    return retn;
}



GLint ShaderProgramStatus(GLuint prog, GLboolean shad, GLenum parm) {
    GLchar buff[2048];
    GLint stat, slen;

    buff[countof(buff) - 1] = 0;
    if (shad) {
        glGetShaderiv(prog, parm, &stat);
        if (stat != GL_TRUE) {
            glGetShaderInfoLog(prog, countof(buff) - 1, &slen, buff);
            printf("Shader error!\n%s\n\n", (GLchar*)buff);
        }
    }
    else {
        glGetProgramiv(prog, parm, &stat);
        if (stat != GL_TRUE) {
            glGetProgramInfoLog(prog, countof(buff) - 1, &slen, buff);
            printf("Shader program error!\n%s\n\n", (GLchar*)buff);
        }
    }
    return stat;
}



GLboolean ShaderAdd(GLchar *fstr, GLuint prog, GLenum type) {
    GLuint slen, shad;

    if (!(shad = glCreateShader(type))) {
        printf("Shader allocation failed (0x%X)\n", glGetError());
        glDeleteProgram(prog);
        return GL_FALSE;
    }
    slen = strlen(fstr);
    glShaderSource(shad, 1, &fstr, &slen);
    glCompileShader(shad); /// may emit a semicolon to STDOUT, this is normal

    if (ShaderProgramStatus(shad, GL_TRUE, GL_COMPILE_STATUS) != GL_TRUE) {
        glDeleteShader(shad);
        glDeleteProgram(prog);
        return GL_FALSE;
    }
    glAttachShader(prog, shad);
    glDeleteShader(shad);
    return GL_TRUE;
}



SHDR *MakeShaderList(GLchar *vert[], GLchar *pixl[],
                     GLuint cuni, UNIF *puni, GLuint *cshd) {
    GLchar *curp = 0, *curv = 0;
    GLint ctmp, step, indx, iter = 0;
    GLboolean stop = GL_FALSE;

    while (pixl[iter]) iter++;

    SHDR *retn = calloc(iter, sizeof(*retn));

    *cshd = iter;
    for (iter = 0; iter < *cshd; iter++) {
        if (pixl[iter] != (GLchar*)-1)
            curp = pixl[iter];
        if (!stop) {
            if (!vert[iter])
                stop = GL_TRUE;
            else if (vert[iter] != (GLchar*)-1)
                curv = vert[iter];
        }
        retn[iter].prog = glCreateProgram();
        ShaderAdd(curp, retn[iter].prog, GL_FRAGMENT_SHADER);
        ShaderAdd(curv, retn[iter].prog, GL_VERTEX_SHADER);
        glLinkProgram(retn[iter].prog);
        glUseProgram(retn[iter].prog);

        for (retn[iter].cuni = ctmp = 0; ctmp < cuni; ctmp++)
            if ((indx = glGetUniformLocation(retn[iter].prog,
                                             puni[ctmp].name)) != -1)
                retn[iter].cuni++;

        retn[iter].puni = malloc(retn[iter].cuni * sizeof(*retn[iter].puni));
        for (step = ctmp = 0; ctmp < cuni; ctmp++)
            if ((indx = glGetUniformLocation(retn[iter].prog,
                                             puni[ctmp].name)) != -1) {
                retn[iter].puni[step] = puni[ctmp];
                retn[iter].puni[step++].indx = indx;
            }
    }
    glUseProgram(0);
    return retn;
}



GLchar *shader(GLchar *shdr, va_list list) {
    va_list dupl;
    GLchar *retn;
    GLint size;

    retn = shdr;
    if (retn != (typeof(retn))-1) {
        va_copy(dupl, list);
        size = strlen(shdr);
        while ((retn = va_arg(list, typeof(retn))))
            size += strlen(retn);

        retn = calloc(sizeof(*retn), size + 1);
        vsnprintf(retn, size + 1, shdr, dupl);
        va_end(dupl);
    }
    return retn;
}



GLvoid MakeShaderSrc(GLchar ***sver, GLchar ***spix,
                     GLchar  **tver, GLchar  **tpix, ...) {
    va_list list;
    GLint iter;

    for (iter = 0; tver[iter]; iter++);
    *sver = calloc(iter + 1, sizeof(*tver));

    for (iter = 0; tpix[iter]; iter++);
    *spix = calloc(iter + 1, sizeof(*tpix));

    for (iter = 0; tver[iter]; iter++) {
        va_start(list, tpix);
        (*sver)[iter] = shader(tver[iter], list);
        va_end(list);
    }
    for (iter = 0; tpix[iter]; iter++) {
        va_start(list, tpix);
        (*spix)[iter] = shader(tpix[iter], list);
        va_end(list);
    }
}



GLvoid FreeShaderSrc(GLchar **sver, GLchar **spix) {
    GLint iter;

    for (iter = 0; sver[iter]; iter++)
        if (sver[iter] != (GLchar*)-1)
            free(sver[iter]);

    for (iter = 0; spix[iter]; iter++)
        if (spix[iter] != (GLchar*)-1)
            free(spix[iter]);

    free(sver);
    free(spix);
}



FTEX *BindTex(FVBO *vobj, GLuint bind, GLuint mode) {
    GLuint itex, ttex, ktex = bind;
    FVBO *vtex = vobj;

    if (!vtex) return 0;
    if (vtex->ptex[ktex].orig) {
        do {
            itex = vtex->ptex[ktex].indx;
            ttex = vtex->ptex[ktex].type;
            vtex = vtex->ptex[ktex].orig;
            ktex = itex;
            if (vtex == vobj) {
                printf("Texture circular cross-ref!\n");
                vtex = 0;
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



GLuint MakeTex(FTEX  *retn, GLuint xdim, GLuint ydim, GLuint  zdim,
               GLenum trgt, GLenum wrap, GLint  tmag, GLint   tmin,
               GLenum type, GLenum frmt, GLenum mode, GLvoid *data) {
    GLuint iter, iend;

    iter = (GLuint)-1;
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

        if ((trgt == GL_TEXTURE_2D_ARRAY) || (trgt == GL_TEXTURE_3D)) {
            glTexParameteri(trgt, GL_TEXTURE_WRAP_R, wrap);
            glTexImage3D(trgt, 0, frmt, xdim, ydim, zdim, 0, mode, type, data);
        }
        else
            for (; iter <= iend; iter++)
                glTexImage2D(iter, 0, frmt, xdim, ydim, 0, mode, type, data);

        iter = glGetError();

        glTexParameteri(trgt, GL_TEXTURE_MAG_FILTER, tmag);
        glTexParameteri(trgt, GL_TEXTURE_MIN_FILTER, tmin);
        glTexParameteri(trgt, GL_TEXTURE_WRAP_S, wrap);
        glTexParameteri(trgt, GL_TEXTURE_WRAP_T, wrap);
        glBindTexture(trgt, 0);
    }
    return iter;
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
    glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER, retn->pvbo[0]);
    glBufferDataARB(GL_ELEMENT_ARRAY_BUFFER,
                    patr[0].cdat, patr[0].pdat, patr[0].draw);

    for (iter = 1; iter < catr; iter++) {
        glBindBufferARB(GL_ARRAY_BUFFER, retn->pvbo[iter]);
        glBufferDataARB(GL_ARRAY_BUFFER,
                        patr[iter].cdat, patr[iter].pdat, patr[iter].draw);
    }
    for (shdr = 0; shdr < retn->cshd; shdr++) {
        glUseProgram(retn->pshd[shdr].prog);
        glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER, retn->pvbo[0]);

        for (iter = 1; iter < catr; iter++) {
            if (patr[iter].name
            && (aloc = glGetAttribLocation(retn->pshd[shdr].prog,
                                           patr[iter].name)) != -1) {
                glBindBufferARB(GL_ARRAY_BUFFER, retn->pvbo[iter]);
                ecnt = 0;
                switch (patr[iter].type) {
                    case UNI_T1IV:
                    case UNI_T1FV: ecnt = 1; break;

                    case UNI_T2IV:
                    case UNI_T2FV: ecnt = 2; break;

                    case UNI_T3IV:
                    case UNI_T3FV: ecnt = 3; break;

                    case UNI_T4IV:
                    case UNI_T4FV: ecnt = 4; break;
                }
                switch (patr[iter].type) {
                    case UNI_T1IV:
                    case UNI_T2IV:
                    case UNI_T3IV:
                    case UNI_T4IV:
                        glVertexAttribPointer(aloc, ecnt,
                                              GL_INT, GL_FALSE, 0, 0);
                        break;

                    case UNI_T1FV:
                    case UNI_T2FV:
                    case UNI_T3FV:
                    case UNI_T4FV:
                        glVertexAttribPointer(aloc, ecnt,
                                              GL_FLOAT, GL_FALSE, 0, 0);
                        break;
                }
                glEnableVertexAttribArray(aloc);
            }
        }
    }
    glUseProgram(0);
    glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindBufferARB(GL_ARRAY_BUFFER, 0);
    return retn;
}



GLvoid DrawVBO(FVBO *vobj, GLuint shad) {
    GLenum iter, type;
    UNIF *unif;

    if (shad < vobj->cshd) {
        glUseProgram(vobj->pshd[shad].prog);

        glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER, vobj->pvbo[0]);
        for (iter = 1; iter < vobj->cvbo; iter++)
            glBindBufferARB(GL_ARRAY_BUFFER, vobj->pvbo[0]);

        for (iter = 0; iter < vobj->ctex; iter++)
            BindTex(vobj, iter, TEX_DFLT);

        for (iter = 0; iter < vobj->pshd[shad].cuni; iter++) {
            type = (unif = &vobj->pshd[shad].puni[iter])->type & ~UNI_IIII;
            if ((type < UNI_TMIN) || (type > UNI_TMAX))
                continue;
            if (type == UNI_TMFV)
                glUniformMatrix4fv(unif->indx, 1 + unif->cdat,
                                   GL_TRUE, *(GLvoid**)unif->pdat);
            else
                GL_UNI_FUNC(type, unif->indx, 1 + unif->cdat,
                           (unif->type & UNI_IIII)?
                           (GLvoid*)&unif->pdat : unif->pdat);
        }
        glDrawElements(vobj->elem, vobj->cind, GL_UNSIGNED_INT, 0);
        glBindBufferARB(GL_ARRAY_BUFFER, 0);
        glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER, 0);
        glUseProgram(0);
    }
}



GLvoid FreeVBO(FVBO **vobj) {
    if (vobj && *vobj) {
        while ((*vobj)->cshd) {
            glDeleteProgram((*vobj)->pshd[--(*vobj)->cshd].prog);
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
        *vobj = 0;
    }
}



FRBO *MakeRBO(GLint xdim, GLint ydim) {
    FRBO *retn = calloc(1, sizeof(*retn));
    GLint data;

    retn->xdim = xdim;
    retn->ydim = ydim;
    retn->swiz = 0;

    glGenFramebuffers(1, &retn->fbuf);
    glBindFramebuffer(GL_FRAMEBUFFER, retn->fbuf);

    glGenRenderbuffers(2, retn->rbuf);
    glBindRenderbuffer(GL_RENDERBUFFER, retn->rbuf[0]);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA,
                          retn->xdim, retn->ydim);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                              GL_RENDERBUFFER, retn->rbuf[0]);
    glBindRenderbuffer(GL_RENDERBUFFER, retn->rbuf[1]);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16,
                          retn->xdim, retn->ydim);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                              GL_RENDERBUFFER, retn->rbuf[1]);
    data = retn->xdim * retn->ydim * 4;

    glGenBuffersARB(2, retn->pbuf);
    glBindBufferARB(GL_PIXEL_PACK_BUFFER, retn->pbuf[0]);
    glBufferDataARB(GL_PIXEL_PACK_BUFFER, data, 0, GL_STREAM_READ);
    glBindBufferARB(GL_PIXEL_PACK_BUFFER, retn->pbuf[1]);
    glBufferDataARB(GL_PIXEL_PACK_BUFFER, data, 0, GL_STREAM_READ);
    glBindBufferARB(GL_PIXEL_PACK_BUFFER, 0);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    return retn;
}



GLvoid BindRBO(FRBO *robj, GLboolean bind) {
    GLuint buff = (bind)? robj->fbuf : 0;

    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, buff);
}



GLvoid FreeRBO(FRBO **robj) {
    if (robj && *robj) {
        glBindBufferARB(GL_PIXEL_PACK_BUFFER, 0);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glDeleteRenderbuffers(2, (*robj)->rbuf);
        glDeleteFramebuffers(1, &(*robj)->fbuf);
        glDeleteBuffersARB(2, (*robj)->pbuf);
        free(*robj);
        *robj = 0;
    }
}
