#ifndef OGL_LOAD_H
#define OGL_LOAD_H

#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <math.h>

#ifdef _WIN32
    typedef char GLchar;
    #include <GL/gl.h>
    #include <windows.h>
    #define GL_COMPILE_STATUS                 0x8B81
    #define GL_LINK_STATUS                    0x8B82
    #define GL_VALIDATE_STATUS                0x8B83
    #define GL_FRAGMENT_SHADER                0x8B30
    #define GL_VERTEX_SHADER                  0x8B31
    #define GL_READ_ONLY                      0x88B8
    #define GL_PIXEL_PACK_BUFFER              0x88EB
    #define GL_READ_FRAMEBUFFER               0x8CA8
    #define GL_DRAW_FRAMEBUFFER               0x8CA9
    #define GL_FRAMEBUFFER                    0x8D40
    #define GL_RENDERBUFFER                   0x8D41
    #define GL_COLOR_ATTACHMENT0              0x8CE0
    #define GL_TEXTURE0                       0x84C0
    #define GL_TEXTURE_CUBE_MAP               0x8513
    #define GL_TEXTURE_CUBE_MAP_POSITIVE_X    0x8515
    #define GL_TEXTURE_WRAP_R                 0x8072
    #define GL_TEXTURE_3D                     0x806F
    #define GL_TEXTURE_2D_ARRAY               0x8C1A
    #define GL_ARRAY_BUFFER                   0x8892
    #define GL_ELEMENT_ARRAY_BUFFER           0x8893
    #define GL_DEPTH_COMPONENT16              0x81A5
    #define GL_DEPTH_COMPONENT24              0x81A6
    #define GL_DEPTH_COMPONENT32              0x81A7
    #define GL_DEPTH_ATTACHMENT               0x8D00
    #define GL_STREAM_READ                    0x88E1
    #define GL_STREAM_DRAW                    0x88E0
    #define GL_STATIC_DRAW                    0x88E4
    #define GL_DYNAMIC_DRAW                   0x88E8
    #define GL_CLAMP_TO_EDGE                  0x812F
    #define GL_R8                             0x8229
    #define GL_RG8                            0x822B
    #define GL_RED                            0x1903
    #define GL_RED_INTEGER                    0x8D94
    #define GL_RG                             0x8227
    #define GL_RG_INTEGER                     0x8228
    #define GL_RGB_INTEGER                    0x8D98
    #define GL_RGBA_INTEGER                   0x8D99
    #define GL_BGR_INTEGER                    0x8D9A
    #define GL_BGRA_INTEGER                   0x8D9B
    #define GL_BGRA                           0x80E1
    #define GL_R32F                           0x822E
    #define GL_RG32F                          0x8230
    #define GL_RGB32F                         0x8815
    #define GL_RGBA32F                        0x8814
    #define GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS 0x8B4C
    #define WGL_CONTEXT_MAJOR_VERSION_ARB     0x2091
    #define WGL_CONTEXT_MINOR_VERSION_ARB     0x2092
    #define WGL_CONTEXT_FLAGS_ARB             0x2094
    #define GL_GET_PROC_ADDR(s)  wglGetProcAddress(s)
#elif __APPLE__
    #include <OpenGL/gl.h>
    #include <dlfcn.h>
    #define APIENTRY
    #define GL_RGBA32F                        0x8814
    #define GL_TEXTURE_2D_ARRAY               0x8C1A
    #define GL_GET_PROC_ADDR(s)  dlsym(RTLD_DEFAULT, s)
#else
    #include <GL/gl.h>
    #include <GL/glx.h>
    #define GL_GET_PROC_ADDR(s)  glXGetProcAddress((GLubyte*)(s))
#endif



#define L(c, ...) \
L4(c,1,0,,,,,,,,,,,,,##__VA_ARGS__) L4(c,0,1,,,,,,,,,##__VA_ARGS__) \
L4(c,0,2,,,,,        ##__VA_ARGS__) L4(c,0,3,        ##__VA_ARGS__)

#define L4(c, f, n, ...) \
L3(c,f,n##0,,,,__VA_ARGS__) L3(c,0,n##1,,,__VA_ARGS__) \
L3(c,0,n##2,,  __VA_ARGS__) L3(c,0,n##3,  __VA_ARGS__)

#define L3(...) L2(__VA_ARGS__, \
1,1,1,1, 1,1,1,1, 1,1,1,1, 1,1,1,1,  0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, )

#define L2(c, f, \
n00,n01,n02,n03, n04,n05,n06,n07, n08,n09,n0A,n0B, n0C,n0D,n0E,n0F, \
a00,a01,a02,a03, a04,a05,a06,a07, a08,a09,a0A,a0B, a0C,a0D,a0E,a0F, \
s, ...) L##s(c, f, n00, a00)

#define L1(c, f, n, a) c##f(n, a)
#define L0(c, f, n, a)

#define P1(n, a)   a _##n
#define P0(n, a) , P1(n, a)
#define A1(n, a)   P1(n,  )
#define A0(n, a)   P0(n,  )

#define F(retn, name, ...) __attribute__((unused))                    \
static retn name(L(P, ##__VA_ARGS__)) {                               \
    static retn APIENTRY (*func)(__VA_ARGS__) = 0;                    \
    if (!(func || (func = (retn APIENTRY (*)(__VA_ARGS__))            \
                           GL_GET_PROC_ADDR(#name))))                 \
        printf("ERROR: cannot load '%s', terminating now!\n", #name); \
    return func(L(A, ##__VA_ARGS__));                                 \
}

#ifndef __APPLE__
F(GLvoid, glUniform1iv, GLint, GLsizei, GLint*);
F(GLvoid, glUniform1fv, GLint, GLsizei, GLfloat*);
F(GLvoid, glUniform2iv, GLint, GLsizei, GLint*);
F(GLvoid, glUniform2fv, GLint, GLsizei, GLfloat*);
F(GLvoid, glUniform3iv, GLint, GLsizei, GLint*);
F(GLvoid, glUniform3fv, GLint, GLsizei, GLfloat*);
F(GLvoid, glUniform4iv, GLint, GLsizei, GLint*);
F(GLvoid, glUniform4fv, GLint, GLsizei, GLfloat*);
F(GLvoid, glUniformMatrix4fv, GLint, GLsizei, GLboolean, GLfloat*);
F(GLuint, glCreateProgram);
F(GLvoid, glDeleteProgram, GLuint);
F(GLvoid, glValidateProgram, GLuint);
F(GLvoid, glLinkProgram, GLuint);
F(GLvoid, glUseProgram, GLuint);
F(GLvoid, glGetProgramInfoLog, GLuint, GLint, GLint*, GLchar*);
F(GLvoid, glGetShaderInfoLog, GLuint, GLint, GLint*, GLchar*);
F(GLvoid, glGetProgramiv, GLuint, GLenum, GLint*);
F(GLvoid, glGetShaderiv, GLuint, GLenum, GLint*);
F(GLuint, glCreateShader, GLenum);
F(GLvoid, glDeleteShader, GLuint);
F(GLvoid, glAttachShader, GLuint, GLuint);
F(GLvoid, glShaderSource, GLuint, GLuint, const GLchar**, GLint*);
F(GLvoid, glCompileShader, GLuint);
F(GLint,  glGetAttribLocation, GLuint, GLchar*);
F(GLint,  glGetUniformLocation, GLuint, GLchar*);
F(GLvoid, glEnableVertexAttribArray, GLint);
F(GLvoid, glDisableVertexAttribArray, GLint);
F(GLvoid, glVertexAttribPointer, GLuint, GLint, GLenum,
                                 GLboolean, GLsizei, GLvoid*);
#ifdef _WIN32
F(GLvoid, glActiveTexture, GLenum);
F(GLvoid, glTexImage3D, GLenum, GLint, GLenum, GLsizei, GLsizei,
                        GLsizei, GLint, GLenum, GLenum, GLvoid*);
F(GLvoid, glTexSubImage3D, GLenum, GLint, GLint, GLint, GLint, GLsizei,
                           GLsizei, GLsizei, GLenum, GLenum, GLvoid*);
#endif /** _WIN32 **/
F(GLvoid, glGenBuffers, GLsizei, GLuint*);
F(GLvoid, glBindBuffer, GLenum, GLuint);
F(GLvoid, glBufferData, GLenum, GLsizei, GLvoid*, GLenum);
F(GLvoid, glBufferSubData, GLenum, GLint, GLsizei, GLvoid*);
F(GLvoid, glDeleteBuffers, GLsizei, GLuint*);
F(GLvoid*,glMapBuffer, GLenum, GLenum);
F(GLvoid, glUnmapBuffer, GLenum);
F(GLvoid, glGenFramebuffersEXT, GLsizei, GLuint*);
F(GLvoid, glGenRenderbuffersEXT, GLsizei, GLuint*);
F(GLvoid, glDeleteFramebuffersEXT, GLsizei, GLuint*);
F(GLvoid, glDeleteRenderbuffersEXT, GLsizei, GLuint*);
F(GLvoid, glBindFramebufferEXT, GLenum, GLuint);
F(GLvoid, glBindRenderbufferEXT, GLenum, GLuint);
F(GLvoid, glRenderbufferStorageEXT, GLenum, GLenum, GLsizei, GLsizei);
F(GLvoid, glFramebufferTexture2DEXT, GLenum, GLenum, GLenum, GLuint, GLint);
F(GLvoid, glFramebufferRenderbufferEXT, GLenum, GLenum, GLenum, GLuint);
#endif /** !__APPLE__ **/

#undef L
#undef L4
#undef L3
#undef L2
#undef L1
#undef L0
#undef P1
#undef P0
#undef A1
#undef A0
#undef F



#ifndef countof
#define countof(a) (sizeof(a) / sizeof(*(a)))
#endif

#define TEX_NSET 0
#define TEX_DFLT 1
#define TEX_FRMB 2

/** types of uniforms **/
#define UNI_T1IV 0
#define UNI_T1FV 1

#define UNI_T2IV 2
#define UNI_T2FV 3

#define UNI_T3IV 4
#define UNI_T3FV 5

#define UNI_T4IV 6
#define UNI_T4FV 7

#define UNI_TMFV 8

/** this flag is to indicate that the uniform value is immediate **/
#define UNI_IIII 0x8000

/** immediate types **/
#define UNI_T1II (UNI_T1IV | UNI_IIII)
#define UNI_T1FI (UNI_T1FV | UNI_IIII)

#define UNI_T2II (UNI_T2IV | UNI_IIII)
#define UNI_T2FI (UNI_T2FV | UNI_IIII)

#define UNI_T3II (UNI_T3IV | UNI_IIII)
#define UNI_T3FI (UNI_T3FV | UNI_IIII)

#define UNI_T4II (UNI_T4IV | UNI_IIII)
#define UNI_T4FI (UNI_T4FV | UNI_IIII)

#define UNI_TMFI (UNI_TMFV | UNI_IIII)



typedef struct {
    GLenum trgt, type, mode;
    GLuint xdim, ydim, zdim, indx;
    struct _FVBO *orig;
} FTEX;

typedef struct {
    GLenum type, draw;
    GLuint indx, cdat;
    GLvoid *pdat;
    GLchar *name;
} UNIF;

typedef struct _FVBO {
    GLenum  elem;
    GLuint  cind;
    GLuint  ctex;
    FTEX   *ptex;
    GLuint  catr;
    GLuint *pbuf;
    struct {
        GLint  aloc;
        GLint  ecnt;
        GLenum elem;
    } *patr;
    GLuint  cshd;
    struct {
        GLuint prog;
        GLuint cuni;
        UNIF  *puni;
    } *pshd;
} FVBO;



__attribute__((unused))
static GLint ShaderProgramStatus(GLuint prog, GLboolean shad, GLenum parm) {
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



__attribute__((unused))
static GLboolean ShaderAdd(const GLchar *fstr, GLuint prog, GLenum type) {
    GLint slen, shad;

    if (!(shad = glCreateShader(type))) {
        printf("Shader allocation failed (0x%X)\n", glGetError());
        glDeleteProgram(prog);
        return GL_FALSE;
    }
    slen = strlen(fstr);
    glShaderSource(shad, 1, &fstr, &slen);
    glCompileShader(shad); /** may emit a ';' to STDOUT, this is normal **/

    if (ShaderProgramStatus(shad, GL_TRUE, GL_COMPILE_STATUS) != GL_TRUE) {
        glDeleteShader(shad);
        glDeleteProgram(prog);
        return GL_FALSE;
    }
    glAttachShader(prog, shad);
    glDeleteShader(shad);
    return GL_TRUE;
}



__attribute__((unused))
static GLvoid MakeShaderSrc(GLchar ***shdr, GLchar **tmpl, va_list list) {
    GLint iter, size;
    GLchar *retn;
    va_list dupl;

    for (iter = 0; tmpl[iter]; iter++);
    *shdr = calloc(iter + 1, sizeof(*tmpl));
    for (iter = 0; tmpl[iter]; iter++)
        if (tmpl[iter] == (typeof(*tmpl))-1)
            (*shdr)[iter] = tmpl[iter];
        else {
            size = strlen(tmpl[iter]);
            va_copy(dupl, list);
            while ((retn = va_arg(dupl, typeof(retn))))
                size += strlen(retn);
            va_end(dupl);
            retn = calloc(sizeof(*retn), size + 1);
            va_copy(dupl, list);
            vsnprintf(retn, size + 1, tmpl[iter], dupl);
            va_end(dupl);
            (*shdr)[iter] = retn;
        }
}



__attribute__((unused))
static GLvoid FreeShaderSrc(GLchar **shdr) {
    GLint iter;

    for (iter = 0; shdr[iter]; iter++)
        if (shdr[iter] != (GLchar*)-1)
            free(shdr[iter]);
    free(shdr);
}



__attribute__((unused))
static FTEX *BindTex(FVBO *vobj, GLuint bind, GLuint mode) {
    GLuint ktex = bind;
    FVBO *vtex = vobj;
    FTEX *ftex;

    if (!vtex)
        return 0;
    if (vtex->ptex[ktex].orig) {
        do {
            ktex = (ftex = &vtex->ptex[ktex])->indx;
            if ((vtex = ftex->orig) == vobj) {
                printf("Texture circular cross-ref!\n");
                return 0;
            }
        } while (vtex);
        vobj->ptex[ktex = bind] = *ftex;
    }
    if (mode == TEX_FRMB)
        glFramebufferTexture2DEXT(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                  GL_TEXTURE_2D, vobj->ptex[ktex].indx, 0);
    else if (mode == TEX_DFLT) {
        glActiveTexture(GL_TEXTURE0 + bind);
        glBindTexture(vobj->ptex[ktex].trgt, vobj->ptex[ktex].indx);
    }
    return &vobj->ptex[ktex];
}



__attribute__((unused))
static GLuint MakeTex(FTEX  *retn, GLuint xdim, GLuint ydim, GLuint  zdim,
                      GLenum trgt, GLenum wrap, GLint  tmag, GLint   tmin,
                      GLenum type, GLenum frmt, GLenum mode, GLvoid *data) {
    GLuint iter, iend;

    iter = (GLuint)GL_INVALID_VALUE;
    if (retn) {
        *retn = (FTEX){trgt, type, mode, xdim, ydim, zdim};
        glGenTextures(1, &retn->indx);
        glBindTexture(retn->trgt, retn->indx);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

        if (retn->trgt != GL_TEXTURE_CUBE_MAP)
            iend = iter = retn->trgt;
        else {
            iend = 6 + (iter = GL_TEXTURE_CUBE_MAP_POSITIVE_X);
            glTexParameteri(retn->trgt, GL_TEXTURE_WRAP_R, wrap);
        }
        if ((retn->trgt != GL_TEXTURE_3D)
        &&  (retn->trgt != GL_TEXTURE_2D_ARRAY))
            for (; iter <= iend; iter++)
                glTexImage2D(iter, 0, frmt, retn->xdim, retn->ydim,
                             0, retn->mode, retn->type, data);
        else {
            glTexParameteri(retn->trgt, GL_TEXTURE_WRAP_R, wrap);
            glTexImage3D(retn->trgt, 0, frmt, retn->xdim, retn->ydim,
                         retn->zdim, 0, retn->mode, retn->type, data);
        }
        iter = glGetError();
        glTexParameteri(retn->trgt, GL_TEXTURE_MAG_FILTER, tmag);
        glTexParameteri(retn->trgt, GL_TEXTURE_MIN_FILTER, tmin);
        glTexParameteri(retn->trgt, GL_TEXTURE_WRAP_S, wrap);
        glTexParameteri(retn->trgt, GL_TEXTURE_WRAP_T, wrap);
        glBindTexture(retn->trgt, 0);
    }
    return iter;
}



__attribute__((unused))
static GLenum LoadTex(FTEX  *retn, GLint  xpos, GLint  ypos, GLint   zpos,
                      GLuint xdim, GLuint ydim, GLuint zdim, GLvoid *data) {
    glBindTexture(retn->trgt, retn->indx);
    switch (retn->trgt) {
        case GL_TEXTURE_2D:
            glTexSubImage2D(retn->trgt, 0, xpos, ypos, xdim, ydim,
                            retn->mode, retn->type, data);
            break;

        case GL_TEXTURE_3D:
        case GL_TEXTURE_2D_ARRAY:
            glTexSubImage3D(retn->trgt, 0, xpos, ypos, zpos, xdim, ydim, zdim,
                            retn->mode, retn->type, data);
            break;
    }
    return glGetError();
}



#define MakeVBO(ctex, elem, catr, patr, cuni, puni, vshd, pshd, ...) \
      __MakeVBO(ctex, elem, catr, patr, cuni, puni, vshd, pshd, ##__VA_ARGS__, 0)
__attribute__((unused))
static FVBO *__MakeVBO(GLuint ctex, GLenum elem, GLuint catr, UNIF *patr,
                       GLuint cuni, UNIF *puni, GLchar *vshd[], GLchar *pshd[],
                       ...) {
    GLchar **vert, **pixl, *curp, *curv;
    GLint iter, indx, ctmp, step;
    GLboolean stop;
    FVBO *retn;
    va_list list;

    retn = calloc(1, sizeof(*retn));
    if (vshd && pshd) {
        retn->cshd = 0;
        curp = curv = 0;
        stop = GL_FALSE;
        va_start(list, pshd);
        MakeShaderSrc(&vert, vshd, list);
        va_end(list);
        va_start(list, pshd);
        MakeShaderSrc(&pixl, pshd, list);
        va_end(list);
        while (pixl[retn->cshd])
            retn->cshd++;
        retn->pshd = calloc(retn->cshd, sizeof(*retn->pshd));
        for (iter = 0; iter < retn->cshd; iter++) {
            if (pixl[iter] != (GLchar*)-1)
                curp = pixl[iter];
            if (!stop) {
                if (!vert[iter])
                    stop = GL_TRUE;
                else if (vert[iter] != (GLchar*)-1)
                    curv = vert[iter];
            }
            retn->pshd[iter].prog = glCreateProgram();
            if (ShaderAdd(curp, retn->pshd[iter].prog, GL_FRAGMENT_SHADER)
            &&  ShaderAdd(curv, retn->pshd[iter].prog, GL_VERTEX_SHADER)) {
                glLinkProgram(retn->pshd[iter].prog);
                if (ShaderProgramStatus(retn->pshd[iter].prog, GL_FALSE,
                                        GL_LINK_STATUS) == GL_TRUE) {
                    glUseProgram(retn->pshd[iter].prog);
                    retn->pshd[iter].cuni = 0;
                    for (ctmp = 0; ctmp < cuni; ctmp++) {
                        indx = glGetUniformLocation(retn->pshd[iter].prog,
                                                    puni[ctmp].name);
                        if (indx != -1)
                            retn->pshd[iter].cuni++;
                    }
                    retn->pshd[iter].puni =
                        (cuni)? malloc(retn->pshd[iter].cuni
                                     * sizeof(*retn->pshd[iter].puni)) : 0;
                    for (step = ctmp = 0; ctmp < cuni; ctmp++) {
                        indx = glGetUniformLocation(retn->pshd[iter].prog,
                                                    puni[ctmp].name);
                        if (indx != -1) {
                            retn->pshd[iter].puni[step] = puni[ctmp];
                            retn->pshd[iter].puni[step++].indx = indx;
                        }
                    }
                    glUseProgram(0);
                    continue;
                }
            }
            /** Only executed if it`s impossible to build the shader **/
            glDeleteProgram(retn->pshd[iter].prog);
        }
        FreeShaderSrc(vert);
        FreeShaderSrc(pixl);
    }
    if (ctex) {
        retn->ctex = ctex;
        retn->ptex = calloc(retn->ctex, sizeof(*retn->ptex));
    }
    retn->elem = elem;
    retn->catr = catr;
    glGenBuffers(catr, retn->pbuf = calloc(catr, sizeof(*retn->pbuf)));
    retn->patr = calloc(catr, sizeof(*retn->patr));

    retn->cind = patr[0].cdat / sizeof(GLuint);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, retn->pbuf[0]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 patr[0].cdat, patr[0].pdat, patr[0].draw);

    for (iter = 1; iter < catr; iter++) {
        glBindBuffer(GL_ARRAY_BUFFER, retn->pbuf[iter]);
        glBufferData(GL_ARRAY_BUFFER,
                     patr[iter].cdat, patr[iter].pdat, patr[iter].draw);
    }
    for (indx = 0; indx < retn->cshd; indx++) {
        glUseProgram(retn->pshd[indx].prog);
        for (iter = 1; iter < catr; iter++) {
            retn->patr[iter].aloc =
                (patr[iter].name)? glGetAttribLocation(retn->pshd[indx].prog,
                                                       patr[iter].name) : -1;
            switch (patr[iter].type) {
                case UNI_T1IV: case UNI_T1FV: retn->patr[iter].ecnt = 1; break;
                case UNI_T2IV: case UNI_T2FV: retn->patr[iter].ecnt = 2; break;
                case UNI_T3IV: case UNI_T3FV: retn->patr[iter].ecnt = 3; break;
                case UNI_T4IV: case UNI_T4FV: retn->patr[iter].ecnt = 4; break;
            }
            switch (patr[iter].type) {
                case UNI_T1IV: case UNI_T2IV:
                case UNI_T3IV: case UNI_T4IV:
                    retn->patr[iter].elem = GL_INT;   break;
                case UNI_T1FV: case UNI_T2FV:
                case UNI_T3FV: case UNI_T4FV:
                    retn->patr[iter].elem = GL_FLOAT; break;
            }
        }
        glUseProgram(0);
    }
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    return retn;
}



__attribute__((unused))
static GLvoid DrawVBO(FVBO *vobj, GLuint shad) {
    GLenum iter;
    UNIF *unif;

    if (shad < vobj->cshd) {
        glUseProgram(vobj->pshd[shad].prog);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vobj->pbuf[0]);
        for (iter = 1; iter < vobj->catr; iter++) {
            if (vobj->patr[iter].aloc == -1)
                continue;
            glBindBuffer(GL_ARRAY_BUFFER, vobj->pbuf[iter]);
            glVertexAttribPointer(vobj->patr[iter].aloc, vobj->patr[iter].ecnt,
                                  vobj->patr[iter].elem, GL_FALSE, 0, 0);
            glEnableVertexAttribArray(vobj->patr[iter].aloc);
        }
        for (iter = 0; iter < vobj->ctex; iter++)
            BindTex(vobj, iter, TEX_DFLT);

        for (iter = 0; iter < vobj->pshd[shad].cuni; iter++)
            switch ((unif = &vobj->pshd[shad].puni[iter])->type & ~UNI_IIII) {
                case UNI_TMFV:
                    glUniformMatrix4fv(unif->indx, 1, GL_TRUE,
                                      (unif->type & UNI_IIII)?
                                      (GLfloat*)unif->pdat :
                                     *(GLfloat**)unif->pdat);
                    continue;

                case UNI_T1IV:
                    glUniform1iv(unif->indx, 1, (unif->type & UNI_IIII)?
                                (GLint*)&unif->pdat : (GLint*)unif->pdat);
                    break;

                case UNI_T1FV:
                    glUniform1fv(unif->indx, 1, (unif->type & UNI_IIII)?
                                (GLfloat*)&unif->pdat : (GLfloat*)unif->pdat);
                    break;

                case UNI_T2IV:
                    glUniform2iv(unif->indx, 1, (unif->type & UNI_IIII)?
                                (GLint*)&unif->pdat : (GLint*)unif->pdat);
                    break;

                case UNI_T2FV:
                    glUniform2fv(unif->indx, 1, (unif->type & UNI_IIII)?
                                (GLfloat*)&unif->pdat : (GLfloat*)unif->pdat);
                    break;

                case UNI_T3IV:
                    glUniform3iv(unif->indx, 1, (unif->type & UNI_IIII)?
                                (GLint*)&unif->pdat : (GLint*)unif->pdat);
                    break;

                case UNI_T3FV:
                    glUniform3fv(unif->indx, 1, (unif->type & UNI_IIII)?
                                (GLfloat*)&unif->pdat : (GLfloat*)unif->pdat);
                    break;

                case UNI_T4IV:
                    glUniform4iv(unif->indx, 1, (unif->type & UNI_IIII)?
                                (GLint*)&unif->pdat : (GLint*)unif->pdat);
                    break;

                case UNI_T4FV:
                    glUniform4fv(unif->indx, 1, (unif->type & UNI_IIII)?
                                (GLfloat*)&unif->pdat : (GLfloat*)unif->pdat);
                    break;

                default: continue;
            }
        glDrawElements(vobj->elem, vobj->cind, GL_UNSIGNED_INT, 0);
        for (iter = 1; iter < vobj->catr; iter++)
            if (vobj->patr[iter].aloc != -1)
                glDisableVertexAttribArray(vobj->patr[iter].aloc);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        glUseProgram(0);
    }
}



__attribute__((unused))
static GLvoid FreeVBO(FVBO **vobj) {
    if (vobj && *vobj) {
        while ((*vobj)->cshd) {
            glDeleteProgram((*vobj)->pshd[--(*vobj)->cshd].prog);
            free((*vobj)->pshd[(*vobj)->cshd].puni);
        }
        glDeleteBuffers((*vobj)->catr, (*vobj)->pbuf);
        free((*vobj)->pbuf);
        free((*vobj)->patr);
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

#endif /** OGL_LOAD_H **/
