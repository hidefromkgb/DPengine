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
    #define GL_RGBA32F                      0x8814
    #define GL_TEXTURE_2D_ARRAY             0x8C1A
    #define GL_GET_PROC_ADDR(s)  dlsym(RTLD_DEFAULT, s)
#else
    #include <GL/gl.h>
    #include <GL/glx.h>
    #define GL_GET_PROC_ADDR(s)  glXGetProcAddress((GLubyte*)(s))
#endif



#define _ARGS(name, ...) \
    _ARGN(name, ##__VA_ARGS__, 31,  30,  29,  28,  27,  26,  25,  24, \
           23,  22,  21,  20,  19,  18,  17,  16,  15,  14,  13,  12, \
           11,  10,   9,   8,   7,   6,   5,   4,   3,   2,   1,   0)
#define _ARGN(name,  _1,  _2,  _3,  _4,  _5,  _6,  _7,  _8,  _9, _10, \
          _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, \
          _23, _24, _25, _26, _27, _28, _29, _30, _31, indx, ...) name ##indx

#define _SET(cond, what) _SET ##cond(what)
#define _SET1(what) what
#define _SET0(what)

#define _VAR0(...)
#define _VAR1(cond, type, ...)  _SET(cond, type) p
#define _VAR2(cond, type, ...)  _SET(cond, type) _ARGS(p, __VA_ARGS__),  \
                                _ARGS(_VAR, __VA_ARGS__)(cond, __VA_ARGS__)
#define _VAR3(cond, type, ...)  _SET(cond, type) _ARGS(p, __VA_ARGS__),  \
                                _ARGS(_VAR, __VA_ARGS__)(cond, __VA_ARGS__)
#define _VAR4(cond, type, ...)  _SET(cond, type) _ARGS(p, __VA_ARGS__),  \
                                _ARGS(_VAR, __VA_ARGS__)(cond, __VA_ARGS__)
#define _VAR5(cond, type, ...)  _SET(cond, type) _ARGS(p, __VA_ARGS__),  \
                                _ARGS(_VAR, __VA_ARGS__)(cond, __VA_ARGS__)
#define _VAR6(cond, type, ...)  _SET(cond, type) _ARGS(p, __VA_ARGS__),  \
                                _ARGS(_VAR, __VA_ARGS__)(cond, __VA_ARGS__)
#define _VAR7(cond, type, ...)  _SET(cond, type) _ARGS(p, __VA_ARGS__),  \
                                _ARGS(_VAR, __VA_ARGS__)(cond, __VA_ARGS__)
#define _VAR8(cond, type, ...)  _SET(cond, type) _ARGS(p, __VA_ARGS__),  \
                                _ARGS(_VAR, __VA_ARGS__)(cond, __VA_ARGS__)
#define _VAR9(cond, type, ...)  _SET(cond, type) _ARGS(p, __VA_ARGS__),  \
                                _ARGS(_VAR, __VA_ARGS__)(cond, __VA_ARGS__)
#define _VAR10(cond, type, ...) _SET(cond, type) _ARGS(p, __VA_ARGS__),  \
                                _ARGS(_VAR, __VA_ARGS__)(cond, __VA_ARGS__)
#define _VAR11(cond, type, ...) _SET(cond, type) _ARGS(p, __VA_ARGS__),  \
                                _ARGS(_VAR, __VA_ARGS__)(cond, __VA_ARGS__)
#define _VAR12(cond, type, ...) _SET(cond, type) _ARGS(p, __VA_ARGS__),  \
                                _ARGS(_VAR, __VA_ARGS__)(cond, __VA_ARGS__)
#define _VAR13(cond, type, ...) _SET(cond, type) _ARGS(p, __VA_ARGS__),  \
                                _ARGS(_VAR, __VA_ARGS__)(cond, __VA_ARGS__)
#define _VAR14(cond, type, ...) _SET(cond, type) _ARGS(p, __VA_ARGS__),  \
                                _ARGS(_VAR, __VA_ARGS__)(cond, __VA_ARGS__)
#define _VAR15(cond, type, ...) _SET(cond, type) _ARGS(p, __VA_ARGS__),  \
                                _ARGS(_VAR, __VA_ARGS__)(cond, __VA_ARGS__)
#define _VAR16(cond, type, ...) _SET(cond, type) _ARGS(p, __VA_ARGS__),  \
                                _ARGS(_VAR, __VA_ARGS__)(cond, __VA_ARGS__)
#define _VAR17(cond, type, ...) _SET(cond, type) _ARGS(p, __VA_ARGS__),  \
                                _ARGS(_VAR, __VA_ARGS__)(cond, __VA_ARGS__)
#define _VAR18(cond, type, ...) _SET(cond, type) _ARGS(p, __VA_ARGS__),  \
                                _ARGS(_VAR, __VA_ARGS__)(cond, __VA_ARGS__)
#define _VAR19(cond, type, ...) _SET(cond, type) _ARGS(p, __VA_ARGS__),  \
                                _ARGS(_VAR, __VA_ARGS__)(cond, __VA_ARGS__)
#define _VAR20(cond, type, ...) _SET(cond, type) _ARGS(p, __VA_ARGS__),  \
                                _ARGS(_VAR, __VA_ARGS__)(cond, __VA_ARGS__)
#define _VAR21(cond, type, ...) _SET(cond, type) _ARGS(p, __VA_ARGS__),  \
                                _ARGS(_VAR, __VA_ARGS__)(cond, __VA_ARGS__)
#define _VAR22(cond, type, ...) _SET(cond, type) _ARGS(p, __VA_ARGS__),  \
                                _ARGS(_VAR, __VA_ARGS__)(cond, __VA_ARGS__)
#define _VAR23(cond, type, ...) _SET(cond, type) _ARGS(p, __VA_ARGS__),  \
                                _ARGS(_VAR, __VA_ARGS__)(cond, __VA_ARGS__)
#define _VAR24(cond, type, ...) _SET(cond, type) _ARGS(p, __VA_ARGS__),  \
                                _ARGS(_VAR, __VA_ARGS__)(cond, __VA_ARGS__)
#define _VAR25(cond, type, ...) _SET(cond, type) _ARGS(p, __VA_ARGS__),  \
                                _ARGS(_VAR, __VA_ARGS__)(cond, __VA_ARGS__)
#define _VAR26(cond, type, ...) _SET(cond, type) _ARGS(p, __VA_ARGS__),  \
                                _ARGS(_VAR, __VA_ARGS__)(cond, __VA_ARGS__)
#define _VAR27(cond, type, ...) _SET(cond, type) _ARGS(p, __VA_ARGS__),  \
                                _ARGS(_VAR, __VA_ARGS__)(cond, __VA_ARGS__)
#define _VAR28(cond, type, ...) _SET(cond, type) _ARGS(p, __VA_ARGS__),  \
                                _ARGS(_VAR, __VA_ARGS__)(cond, __VA_ARGS__)
#define _VAR29(cond, type, ...) _SET(cond, type) _ARGS(p, __VA_ARGS__),  \
                                _ARGS(_VAR, __VA_ARGS__)(cond, __VA_ARGS__)
#define _VAR30(cond, type, ...) _SET(cond, type) _ARGS(p, __VA_ARGS__),  \
                                _ARGS(_VAR, __VA_ARGS__)(cond, __VA_ARGS__)
#define _VAR31(cond, type, ...) _SET(cond, type) _ARGS(p, __VA_ARGS__),  \
                                _ARGS(_VAR, __VA_ARGS__)(cond, __VA_ARGS__)

#define _FUNC(text, retn, name, ...) __attribute__((unused))          \
static retn name(_ARGS(_VAR, ##__VA_ARGS__)(1, ##__VA_ARGS__)) {      \
    static retn APIENTRY (*func)(__VA_ARGS__) = 0;                    \
    if (!(func || (func = (retn APIENTRY (*)(__VA_ARGS__))            \
                           GL_GET_PROC_ADDR((text)? text : #name))))  \
        printf("ERROR: cannot load '%s', terminating now!\n",         \
              (text)? text : #name);                                  \
    return func(_ARGS(_VAR, ##__VA_ARGS__)(0, ##__VA_ARGS__));        \
}

_FUNC(0, GLvoid, glUniform1iv, GLint, GLsizei, GLint*);
_FUNC(0, GLvoid, glUniform1fv, GLint, GLsizei, GLfloat*);
_FUNC(0, GLvoid, glUniform2iv, GLint, GLsizei, GLint*);
_FUNC(0, GLvoid, glUniform2fv, GLint, GLsizei, GLfloat*);
_FUNC(0, GLvoid, glUniform3iv, GLint, GLsizei, GLint*);
_FUNC(0, GLvoid, glUniform3fv, GLint, GLsizei, GLfloat*);
_FUNC(0, GLvoid, glUniform4iv, GLint, GLsizei, GLint*);
_FUNC(0, GLvoid, glUniform4fv, GLint, GLsizei, GLfloat*);
_FUNC(0, GLvoid, glUniformMatrix4fv, GLint, GLsizei, GLboolean, GLfloat*);
_FUNC(0, GLuint, glCreateProgram);
_FUNC(0, GLvoid, glDeleteProgram, GLuint);
_FUNC(0, GLvoid, glValidateProgram, GLuint);
_FUNC(0, GLvoid, glLinkProgram, GLuint);
_FUNC(0, GLvoid, glUseProgram, GLuint);
_FUNC(0, GLvoid, glGetProgramInfoLog, GLuint, GLint, GLint*, GLchar*);
_FUNC(0, GLvoid, glGetShaderInfoLog, GLuint, GLint, GLint*, GLchar*);
_FUNC(0, GLvoid, glGetProgramiv, GLuint, GLenum, GLint*);
_FUNC(0, GLvoid, glGetShaderiv, GLuint, GLenum, GLint*);
_FUNC(0, GLuint, glCreateShader, GLenum);
_FUNC(0, GLvoid, glDeleteShader, GLuint);
_FUNC(0, GLvoid, glAttachShader, GLuint, GLuint);
_FUNC(0, GLvoid, glShaderSource, GLuint, GLuint, GLchar**, GLuint*);
_FUNC(0, GLvoid, glCompileShader, GLuint);
_FUNC(0, GLint,  glGetAttribLocation, GLuint, GLchar*);
_FUNC(0, GLint,  glGetUniformLocation, GLuint, GLchar*);
_FUNC(0, GLvoid, glEnableVertexAttribArray, GLint);
_FUNC(0, GLvoid, glVertexAttribPointer, GLuint, GLint, GLenum,
                                        GLboolean, GLsizei, GLvoid*);
#ifdef _WIN32
_FUNC(0, GLvoid, glActiveTexture, GLenum);
_FUNC(0, GLvoid, glTexImage3D, GLenum, GLint, GLenum, GLsizei, GLsizei,
                               GLsizei, GLint, GLenum, GLenum, GLvoid*);
_FUNC(0, GLvoid, glTexSubImage3D, GLenum, GLint, GLint, GLint, GLint,
                                  GLsizei, GLsizei, GLsizei, GLenum,
                                  GLenum, GLvoid*);
#endif
_FUNC(0, GLvoid, glGenBuffers, GLsizei, GLuint*);
_FUNC(0, GLvoid, glBindBuffer, GLenum, GLuint);
_FUNC(0, GLvoid, glBufferData, GLenum, GLsizei, GLvoid*, GLenum);
_FUNC(0, GLvoid, glBufferSubData, GLenum, GLint, GLsizei, GLvoid*);
_FUNC(0, GLvoid, glDeleteBuffers, GLsizei, GLuint*);
_FUNC(0, GLvoid*,glMapBuffer, GLenum, GLenum);
_FUNC(0, GLvoid, glUnmapBuffer, GLenum);
_FUNC("glGenFramebuffersEXT",
         GLvoid, glGenFramebuffers, GLsizei, GLuint*);
_FUNC("glGenRenderbuffersEXT",
         GLvoid, glGenRenderbuffers, GLsizei, GLuint*);
_FUNC("glDeleteFramebuffersEXT",
         GLvoid, glDeleteFramebuffers, GLsizei, GLuint*);
_FUNC("glDeleteRenderbuffersEXT",
         GLvoid, glDeleteRenderbuffers, GLsizei, GLuint*);
_FUNC("glBindFramebufferEXT",
         GLvoid, glBindFramebuffer, GLenum, GLuint);
_FUNC("glBindRenderbufferEXT",
         GLvoid, glBindRenderbuffer, GLenum, GLuint);
_FUNC("glRenderbufferStorageEXT",
         GLvoid, glRenderbufferStorage, GLenum, GLenum, GLsizei, GLsizei);
_FUNC("glFramebufferTexture2DEXT",
         GLvoid, glFramebufferTexture2D, GLenum, GLenum, GLenum,
                                         GLuint, GLint);
_FUNC("glFramebufferRenderbufferEXT",
         GLvoid, glFramebufferRenderbuffer, GLenum, GLenum, GLenum, GLuint);

#undef _ARGS
#undef _ARGN

#undef _SET
#undef _SET1
#undef _SET0

#undef _VAR0
#undef _VAR1
#undef _VAR2
#undef _VAR3
#undef _VAR4
#undef _VAR5
#undef _VAR6
#undef _VAR7
#undef _VAR8
#undef _VAR9
#undef _VAR10
#undef _VAR11
#undef _VAR12
#undef _VAR13
#undef _VAR14
#undef _VAR15
#undef _VAR16
#undef _VAR17
#undef _VAR18
#undef _VAR19
#undef _VAR20
#undef _VAR21
#undef _VAR22
#undef _VAR23
#undef _VAR24
#undef _VAR25
#undef _VAR26
#undef _VAR27
#undef _VAR28
#undef _VAR29
#undef _VAR30
#undef _VAR31

#undef _FUNC



#ifndef countof
#define countof(a) (sizeof(a) / sizeof(*(a)))
#endif

#define DTR_CONV (M_PI / 180.0)
#define RTD_CONV (1.0 / DTR_CONV)

#define TEX_NSET 0
#define TEX_DFLT 1
#define TEX_FRMB 2

/// types of uniforms
#define UNI_T1IV 0
#define UNI_T1FV 1

#define UNI_T2IV 2
#define UNI_T2FV 3

#define UNI_T3IV 4
#define UNI_T3FV 5

#define UNI_T4IV 6
#define UNI_T4FV 7

#define UNI_TMFV 8

/// this flag is to indicate that the uniform value is immediate
#define UNI_IIII 0x8000

/// immediate types
#define UNI_T1II (UNI_T1IV | UNI_IIII)
#define UNI_T1FI (UNI_T1FV | UNI_IIII)

#define UNI_T2II (UNI_T2IV | UNI_IIII)
#define UNI_T2FI (UNI_T2FV | UNI_IIII)

#define UNI_T3II (UNI_T3IV | UNI_IIII)
#define UNI_T3FI (UNI_T3FV | UNI_IIII)

#define UNI_T4II (UNI_T4IV | UNI_IIII)
#define UNI_T4FI (UNI_T4FV | UNI_IIII)

#define UNI_TMFI (UNI_TMFV | UNI_IIII)



typedef struct _FTEX {
    GLenum trgt, type, mode;
    GLuint xdim, ydim, zdim, indx;
    struct _FVBO *orig;
} FTEX;

typedef struct _UNIF {
    GLenum type, draw;
    GLuint indx, cdat;
    GLvoid *pdat;
    GLchar *name;
} UNIF;

typedef struct _SHDR {
    GLuint prog, cuni;
    UNIF *puni;
} SHDR;

typedef struct _FVBO {
    GLenum  elem;
    GLuint  cind;
    GLuint  cvbo;
    GLuint *pvbo;
    GLuint  ctex;
    FTEX   *ptex;
    GLuint  cshd;
    SHDR   *pshd;
} FVBO;



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



static GLboolean ShaderAdd(GLchar *fstr, GLuint prog, GLenum type) {
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



static SHDR *MakeShaderList(GLchar *vert[], GLchar *pixl[],
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
        if (ShaderAdd(curp, retn[iter].prog, GL_FRAGMENT_SHADER)
        &&  ShaderAdd(curv, retn[iter].prog, GL_VERTEX_SHADER)) {
            glLinkProgram(retn[iter].prog);
            if (ShaderProgramStatus(retn[iter].prog, GL_FALSE,
                                    GL_LINK_STATUS) == GL_TRUE) {
                glUseProgram(retn[iter].prog);

                for (retn[iter].cuni = ctmp = 0; ctmp < cuni; ctmp++)
                    if ((indx = glGetUniformLocation(retn[iter].prog,
                                                     puni[ctmp].name)) != -1)
                        retn[iter].cuni++;

                retn[iter].puni = malloc(retn[iter].cuni *
                                         sizeof(*retn[iter].puni));
                for (step = ctmp = 0; ctmp < cuni; ctmp++)
                    if ((indx = glGetUniformLocation(retn[iter].prog,
                                                     puni[ctmp].name)) != -1) {
                        retn[iter].puni[step] = puni[ctmp];
                        retn[iter].puni[step++].indx = indx;
                    }
                glUseProgram(0);
                continue;
            }
        }
        glDeleteProgram(retn[iter].prog);
    }
    return retn;
}



static GLchar *shader(GLchar *shdr, va_list list) {
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



__attribute__((unused))
static GLvoid MakeShaderSrc(GLchar ***sver, GLchar ***spix,
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



__attribute__((unused))
static GLvoid FreeShaderSrc(GLchar **sver, GLchar **spix) {
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



static FTEX *BindTex(FVBO *vobj, GLuint bind, GLuint mode) {
    GLuint ktex = bind;
    FVBO *vtex = vobj;
    FTEX *ftex;

    if (!vtex) return 0;
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
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                               GL_TEXTURE_2D, vtex->ptex[ktex].indx, 0);
    else if (mode == TEX_DFLT) {
        glActiveTexture(GL_TEXTURE0 + bind);
        glBindTexture(vtex->ptex[ktex].trgt, vtex->ptex[ktex].indx);
    }
    return &vtex->ptex[ktex];
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



__attribute__((unused))
static FVBO *MakeVBO(FVBO *prev, GLchar *vshd[], GLchar *pshd[],
                     GLenum elem, GLuint catr, UNIF *patr,
                     GLuint cuni, UNIF *puni, GLuint ctex) {
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
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, retn->pvbo[0]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 patr[0].cdat, patr[0].pdat, patr[0].draw);

    for (iter = 1; iter < catr; iter++) {
        glBindBuffer(GL_ARRAY_BUFFER, retn->pvbo[iter]);
        glBufferData(GL_ARRAY_BUFFER,
                     patr[iter].cdat, patr[iter].pdat, patr[iter].draw);
    }
    for (shdr = 0; shdr < retn->cshd; shdr++) {
        glUseProgram(retn->pshd[shdr].prog);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, retn->pvbo[0]);

        for (iter = 1; iter < catr; iter++) {
            if (patr[iter].name
            && (aloc = glGetAttribLocation(retn->pshd[shdr].prog,
                                           patr[iter].name)) != -1) {
                glBindBuffer(GL_ARRAY_BUFFER, retn->pvbo[iter]);
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

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vobj->pvbo[0]);
        for (iter = 1; iter < vobj->cvbo; iter++)
            glBindBuffer(GL_ARRAY_BUFFER, vobj->pvbo[0]);

        for (iter = 0; iter < vobj->ctex; iter++)
            BindTex(vobj, iter, TEX_DFLT);

        for (iter = 0; iter < vobj->pshd[shad].cuni; iter++)
            switch ((unif = &vobj->pshd[shad].puni[iter])->type & ~UNI_IIII) {
                case UNI_TMFV:
                    glUniformMatrix4fv(unif->indx, 1 + unif->cdat,
                                       GL_TRUE, *(GLfloat**)unif->pdat);
                    break;

                case UNI_T1IV:
                    glUniform1iv(unif->indx, 1 + unif->cdat,
                                (unif->type & UNI_IIII)?
                                (GLint*)&unif->pdat : (GLint*)unif->pdat);
                    break;

                case UNI_T1FV:
                    glUniform1fv(unif->indx, 1 + unif->cdat,
                                (unif->type & UNI_IIII)?
                                (GLfloat*)&unif->pdat : (GLfloat*)unif->pdat);
                    break;

                case UNI_T2IV:
                    glUniform2iv(unif->indx, 1 + unif->cdat,
                                (unif->type & UNI_IIII)?
                                (GLint*)&unif->pdat : (GLint*)unif->pdat);
                    break;

                case UNI_T2FV:
                    glUniform2fv(unif->indx, 1 + unif->cdat,
                                (unif->type & UNI_IIII)?
                                (GLfloat*)&unif->pdat : (GLfloat*)unif->pdat);
                    break;

                case UNI_T3IV:
                    glUniform3iv(unif->indx, 1 + unif->cdat,
                                (unif->type & UNI_IIII)?
                                (GLint*)&unif->pdat : (GLint*)unif->pdat);
                    break;

                case UNI_T3FV:
                    glUniform3fv(unif->indx, 1 + unif->cdat,
                                (unif->type & UNI_IIII)?
                                (GLfloat*)&unif->pdat : (GLfloat*)unif->pdat);
                    break;

                case UNI_T4IV:
                    glUniform4iv(unif->indx, 1 + unif->cdat,
                                (unif->type & UNI_IIII)?
                                (GLint*)&unif->pdat : (GLint*)unif->pdat);
                    break;

                case UNI_T4FV:
                    glUniform4fv(unif->indx, 1 + unif->cdat,
                                (unif->type & UNI_IIII)?
                                (GLfloat*)&unif->pdat : (GLfloat*)unif->pdat);
                    break;

                default: continue;
            }
        glDrawElements(vobj->elem, vobj->cind, GL_UNSIGNED_INT, 0);
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
        };
        glDeleteBuffers((*vobj)->cvbo, (*vobj)->pvbo);
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
