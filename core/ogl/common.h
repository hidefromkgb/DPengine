#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#define GL_GEN_VERTEX_ARRAYS "glGenVertexArrays"
#define GL_BIND_VERTEX_ARRAY "glBindVertexArray"
#define GL_DEL_VERTEX_ARRAYS "glDeleteVertexArrays"

#ifdef _WIN32
    typedef char GLchar;
    #include <GL/gl.h>
    #include <windows.h>
    #define GL_COMPILE_STATUS               0x8B81
    #define GL_FRAGMENT_SHADER              0x8B30
    #define GL_VERTEX_SHADER                0x8B31
    #define GL_READ_ONLY                    0x88B8
    #define GL_PIXEL_PACK_BUFFER            0x88EB
    #define GL_READ_FRAMEBUFFER             0x8CA8
    #define GL_DRAW_FRAMEBUFFER             0x8CA9
    #define GL_FRAMEBUFFER                  0x8D40
    #define GL_RENDERBUFFER                 0x8D41
    #define GL_COLOR_ATTACHMENT0            0x8CE0
    #define GL_TEXTURE0                     0x84C0
    #define GL_TEXTURE_CUBE_MAP             0x8513
    #define GL_TEXTURE_CUBE_MAP_POSITIVE_X  0x8515
    #define GL_TEXTURE_WRAP_R               0x8072
    #define GL_TEXTURE_3D                   0x806F
    #define GL_TEXTURE_2D_ARRAY             0x8C1A
    #define GL_ARRAY_BUFFER                 0x8892
    #define GL_ELEMENT_ARRAY_BUFFER         0x8893
    #define GL_DEPTH_COMPONENT16            0x81A5
    #define GL_DEPTH_COMPONENT24            0x81A6
    #define GL_DEPTH_COMPONENT32            0x81A7
    #define GL_DEPTH_ATTACHMENT             0x8D00
    #define GL_STREAM_READ                  0x88E1
    #define GL_STREAM_DRAW                  0x88E0
    #define GL_STATIC_DRAW                  0x88E4
    #define GL_DYNAMIC_DRAW                 0x88E8
    #define GL_CLAMP_TO_EDGE                0x812F
    #define GL_R8                           0x8229
    #define GL_RG8                          0x822B
    #define GL_RED                          0x1903
    #define GL_RED_INTEGER                  0x8D94
    #define GL_RG                           0x8227
    #define GL_RG_INTEGER                   0x8228
    #define GL_RGB_INTEGER                  0x8D98
    #define GL_RGBA_INTEGER                 0x8D99
    #define GL_BGR_INTEGER                  0x8D9A
    #define GL_BGRA_INTEGER                 0x8D9B
    #define GL_BGRA                         0x80E1
    #define GL_R32F                         0x822E
    #define GL_RG32F                        0x8230
    #define GL_RGB32F                       0x8815
    #define GL_RGBA32F                      0x8814
    #define WGL_CONTEXT_MAJOR_VERSION_ARB   0x2091
    #define WGL_CONTEXT_MINOR_VERSION_ARB   0x2092
    #define WGL_CONTEXT_FLAGS_ARB           0x2094
    #define GL_GET_PROC_ADDR(s)  wglGetProcAddress(s)
#elif __APPLE__
    #include <OpenGL/gl3.h>
    #include <dlfcn.h>
    #define APIENTRY
    #define GL_GET_PROC_ADDR(s)  dlsym(RTLD_DEFAULT, s)
    /// wasted a gazillion hours in CGDB tracking this down...
    /// why, Apple, why??? >_<
    #undef  GL_GEN_VERTEX_ARRAYS
    #undef  GL_BIND_VERTEX_ARRAY
    #undef  GL_DEL_VERTEX_ARRAYS
    #define GL_GEN_VERTEX_ARRAYS "glGenVertexArraysAPPLE"
    #define GL_BIND_VERTEX_ARRAY "glBindVertexArrayAPPLE"
    #define GL_DEL_VERTEX_ARRAYS "glDeleteVertexArraysAPPLE"
#else
    #include <GL/gl.h>
    #include <GL/glx.h>
    #define GL_GET_PROC_ADDR(s)  glXGetProcAddress((GLubyte*)s)
#endif



#define STRING_OPENGL_FUNCTIONS  \
    "glUniform1iv",              \
    "glUniform1uiv",             \
    "glUniform1fv",              \
    "glUniform2iv",              \
    "glUniform2uiv",             \
    "glUniform2fv",              \
    "glUniform3iv",              \
    "glUniform3uiv",             \
    "glUniform3fv",              \
    "glUniform4iv",              \
    "glUniform4uiv",             \
    "glUniform4fv",              \
    "glUniformMatrix4fv",        \
    "glUseProgramObjectARB",     \
    "glGetProgramInfoLog",       \
    "glGetShaderInfoLog",        \
    "glGetProgramiv",            \
    "glGetShaderiv",             \
    "glCreateProgram",           \
    "glDeleteProgram",           \
    "glCreateShader",            \
    "glDeleteShader",            \
    "glAttachShader",            \
    "glShaderSource",            \
    "glCompileShader",           \
    "glLinkProgram",             \
    "glGetAttribLocation",       \
    "glGetUniformLocation",      \
    "glEnableVertexAttribArray", \
    "glVertexAttribPointer",     \
    "glVertexAttribIPointer",    \
     GL_GEN_VERTEX_ARRAYS,       \
     GL_BIND_VERTEX_ARRAY,       \
     GL_DEL_VERTEX_ARRAYS,       \
    "glActiveTextureARB",        \
    "glGenBuffersARB",           \
    "glBindBufferARB",           \
    "glBufferDataARB",           \
    "glBufferSubDataARB",        \
    "glDeleteBuffersARB",        \
    "glGenFramebuffers",         \
    "glGenRenderbuffers",        \
    "glDeleteFramebuffers",      \
    "glDeleteRenderbuffers",     \
    "glBindFramebuffer",         \
    "glBindRenderbuffer",        \
    "glRenderbufferStorage",     \
    "glFramebufferTexture2D",    \
    "glFramebufferRenderbuffer", \
    "glMapBufferARB",            \
    "glUnmapBufferARB",          \
    "glTexImage3D"

/// uniform type constants have to match the corresponding
/// uniform loaders in LoadedOpenGLFunctions[] table
#define UNI_T1IV 0
#define UNI_T1UV 1
#define UNI_T1FV 2

#define UNI_T2IV 3
#define UNI_T2UV 4
#define UNI_T2FV 5

#define UNI_T3IV 6
#define UNI_T3UV 7
#define UNI_T3FV 8

#define UNI_T4IV 9
#define UNI_T4UV 10
#define UNI_T4FV 11

#define UNI_TMFV 12

/// minimum and maximum uniform type constants
#define UNI_TMIN 0
#define UNI_TMAX 12

/// this flag is to indicate that the uniform value is immediate
#define UNI_IIII 0x8000

/// immediate uniform types
#define UNI_T1II (UNI_T1IV | UNI_IIII)
#define UNI_T1UI (UNI_T1UV | UNI_IIII)
#define UNI_T1FI (UNI_T1FV | UNI_IIII)

#define UNI_T2II (UNI_T2IV | UNI_IIII)
#define UNI_T2UI (UNI_T2UV | UNI_IIII)
#define UNI_T2FI (UNI_T2FV | UNI_IIII)

#define UNI_T3II (UNI_T3IV | UNI_IIII)
#define UNI_T3UI (UNI_T3UV | UNI_IIII)
#define UNI_T3FI (UNI_T3FV | UNI_IIII)

#define UNI_T4II (UNI_T4IV | UNI_IIII)
#define UNI_T4UI (UNI_T4UV | UNI_IIII)
#define UNI_T4FI (UNI_T4FV | UNI_IIII)

#define UNI_TMFI (UNI_TMFV | UNI_IIII)

#define GL_UNI_FUNC(type, indx, size, vals) (((GLvoid APIENTRY (*)(GLint, GLsizei, GLvoid*))LoadedOpenGLFunctions[type])(indx, size, vals))

#define glUniform1iv               ((GLvoid  APIENTRY (*)(GLint  ,  GLsizei,  GLint*                                                                       ))LoadedOpenGLFunctions[UNI_T1IV])
#define glUniform1uiv              ((GLvoid  APIENTRY (*)(GLint  ,  GLsizei,  GLuint*                                                                      ))LoadedOpenGLFunctions[UNI_T1UV])
#define glUniform1fv               ((GLvoid  APIENTRY (*)(GLint  ,  GLsizei,  GLfloat*                                                                     ))LoadedOpenGLFunctions[UNI_T1FV])
#define glUniform2iv               ((GLvoid  APIENTRY (*)(GLint  ,  GLsizei,  GLint*                                                                       ))LoadedOpenGLFunctions[UNI_T2IV])
#define glUniform2uiv              ((GLvoid  APIENTRY (*)(GLint  ,  GLsizei,  GLuint*                                                                      ))LoadedOpenGLFunctions[UNI_T2UV])
#define glUniform2fv               ((GLvoid  APIENTRY (*)(GLint  ,  GLsizei,  GLfloat*                                                                     ))LoadedOpenGLFunctions[UNI_T2FV])
#define glUniform3iv               ((GLvoid  APIENTRY (*)(GLint  ,  GLsizei,  GLint*                                                                       ))LoadedOpenGLFunctions[UNI_T3IV])
#define glUniform3uiv              ((GLvoid  APIENTRY (*)(GLint  ,  GLsizei,  GLuint*                                                                      ))LoadedOpenGLFunctions[UNI_T3UV])
#define glUniform3fv               ((GLvoid  APIENTRY (*)(GLint  ,  GLsizei,  GLfloat*                                                                     ))LoadedOpenGLFunctions[UNI_T3FV])
#define glUniform4iv               ((GLvoid  APIENTRY (*)(GLint  ,  GLsizei,  GLint*                                                                       ))LoadedOpenGLFunctions[UNI_T4IV])
#define glUniform4uiv              ((GLvoid  APIENTRY (*)(GLint  ,  GLsizei,  GLuint*                                                                      ))LoadedOpenGLFunctions[UNI_T4UV])
#define glUniform4fv               ((GLvoid  APIENTRY (*)(GLint  ,  GLsizei,  GLfloat*                                                                     ))LoadedOpenGLFunctions[UNI_T4FV])
#define glUniformMatrix4fv         ((GLvoid  APIENTRY (*)(GLint  ,  GLsizei,  GLboolean,  GLfloat*                                                         ))LoadedOpenGLFunctions[UNI_TMFV])
/// uniform functions end here
#define glUseProgramObjectARB      ((GLvoid  APIENTRY (*)(GLuint                                                                                           ))LoadedOpenGLFunctions[13])
#define glGetProgramInfoLog        ((GLvoid  APIENTRY (*)(GLuint ,  GLint  ,  GLint*   ,  GLchar*                                                          ))LoadedOpenGLFunctions[14])
#define glGetShaderInfoLog         ((GLvoid  APIENTRY (*)(GLuint ,  GLint  ,  GLint*   ,  GLchar*                                                          ))LoadedOpenGLFunctions[15])
#define glGetProgramiv             ((GLvoid  APIENTRY (*)(GLuint ,  GLenum ,  GLint*                                                                       ))LoadedOpenGLFunctions[16])
#define glGetShaderiv              ((GLvoid  APIENTRY (*)(GLuint ,  GLenum ,  GLint*                                                                       ))LoadedOpenGLFunctions[17])
#define glCreateProgram            ((GLuint  APIENTRY (*)(GLvoid                                                                                           ))LoadedOpenGLFunctions[18])
#define glDeleteProgram            ((GLvoid  APIENTRY (*)(GLuint                                                                                           ))LoadedOpenGLFunctions[19])
#define glCreateShader             ((GLuint  APIENTRY (*)(GLenum                                                                                           ))LoadedOpenGLFunctions[20])
#define glDeleteShader             ((GLvoid  APIENTRY (*)(GLuint                                                                                           ))LoadedOpenGLFunctions[21])
#define glAttachShader             ((GLvoid  APIENTRY (*)(GLuint ,  GLuint                                                                                 ))LoadedOpenGLFunctions[22])
#define glShaderSource             ((GLvoid  APIENTRY (*)(GLuint ,  GLuint ,  GLchar** ,  GLuint*                                                          ))LoadedOpenGLFunctions[23])
#define glCompileShader            ((GLvoid  APIENTRY (*)(GLuint                                                                                           ))LoadedOpenGLFunctions[24])
#define glLinkProgram              ((GLvoid  APIENTRY (*)(GLuint                                                                                           ))LoadedOpenGLFunctions[25])
#define glGetAttribLocation        ((GLint   APIENTRY (*)(GLuint ,  GLchar*                                                                                ))LoadedOpenGLFunctions[26])
#define glGetUniformLocation       ((GLint   APIENTRY (*)(GLuint ,  GLchar*                                                                                ))LoadedOpenGLFunctions[27])
#define glEnableVertexAttribArray  ((GLvoid  APIENTRY (*)(GLint                                                                                            ))LoadedOpenGLFunctions[28])
#define glVertexAttribPointer      ((GLvoid  APIENTRY (*)(GLuint ,  GLint  ,  GLenum   ,  GLboolean,  GLsizei,  GLvoid*                                    ))LoadedOpenGLFunctions[29])
#define glVertexAttribIPointer     ((GLvoid  APIENTRY (*)(GLuint ,  GLint  ,  GLenum   ,  GLsizei  ,  GLvoid*                                              ))LoadedOpenGLFunctions[30])
#define glGenVertexArrays          ((GLvoid  APIENTRY (*)(GLsizei,  GLuint*                                                                                ))LoadedOpenGLFunctions[31])
#define glBindVertexArray          ((GLvoid  APIENTRY (*)(GLuint                                                                                           ))LoadedOpenGLFunctions[32])
#define glDeleteVertexArrays       ((GLvoid  APIENTRY (*)(GLsizei,  GLuint*                                                                                ))LoadedOpenGLFunctions[33])
#define glActiveTextureARB         ((GLvoid  APIENTRY (*)(GLenum                                                                                           ))LoadedOpenGLFunctions[34])
#define glGenBuffersARB            ((GLvoid  APIENTRY (*)(GLsizei,  GLuint*                                                                                ))LoadedOpenGLFunctions[35])
#define glBindBufferARB            ((GLvoid  APIENTRY (*)(GLenum ,  GLuint                                                                                 ))LoadedOpenGLFunctions[36])
#define glBufferDataARB            ((GLvoid  APIENTRY (*)(GLenum ,  GLsizei,  GLvoid*  ,  GLenum                                                           ))LoadedOpenGLFunctions[37])
#define glBufferSubDataARB         ((GLvoid  APIENTRY (*)(GLenum ,  GLint  ,  GLsizei  ,  GLvoid*                                                          ))LoadedOpenGLFunctions[38])
#define glDeleteBuffersARB         ((GLvoid  APIENTRY (*)(GLsizei,  GLuint*                                                                                ))LoadedOpenGLFunctions[39])
#define glGenFramebuffers          ((GLvoid  APIENTRY (*)(GLsizei,  GLuint*                                                                                ))LoadedOpenGLFunctions[40])
#define glGenRenderbuffers         ((GLvoid  APIENTRY (*)(GLsizei,  GLuint*                                                                                ))LoadedOpenGLFunctions[41])
#define glDeleteFramebuffers       ((GLvoid  APIENTRY (*)(GLsizei,  GLuint*                                                                                ))LoadedOpenGLFunctions[42])
#define glDeleteRenderbuffers      ((GLvoid  APIENTRY (*)(GLsizei,  GLuint*                                                                                ))LoadedOpenGLFunctions[43])
#define glBindFramebuffer          ((GLvoid  APIENTRY (*)(GLenum ,  GLuint                                                                                 ))LoadedOpenGLFunctions[44])
#define glBindRenderbuffer         ((GLvoid  APIENTRY (*)(GLenum ,  GLuint                                                                                 ))LoadedOpenGLFunctions[45])
#define glRenderbufferStorage      ((GLvoid  APIENTRY (*)(GLenum ,  GLenum ,  GLsizei  ,  GLsizei                                                          ))LoadedOpenGLFunctions[46])
#define glFramebufferTexture2D     ((GLvoid  APIENTRY (*)(GLenum ,  GLenum ,  GLenum   ,  GLuint   ,  GLint                                                ))LoadedOpenGLFunctions[47])
#define glFramebufferRenderbuffer  ((GLvoid  APIENTRY (*)(GLenum ,  GLenum ,  GLenum   ,  GLuint                                                           ))LoadedOpenGLFunctions[48])
#define glMapBufferARB             ((GLvoid* APIENTRY (*)(GLenum ,  GLenum                                                                                 ))LoadedOpenGLFunctions[49])
#define glUnmapBufferARB           ((GLvoid  APIENTRY (*)(GLenum                                                                                           ))LoadedOpenGLFunctions[50])
#define glTexImage3D               ((GLvoid  APIENTRY (*)(GLenum ,  GLint  ,  GLenum   ,  GLsizei  ,  GLsizei,  GLsizei,  GLint,  GLenum,  GLenum,  GLvoid*))LoadedOpenGLFunctions[51])

#ifndef countof
#define countof(a) (sizeof(a) / sizeof(*(a)))
#endif

#define DTR_CONV (M_PI / 180.0)
#define RTD_CONV (1.0 / DTR_CONV)

#define TEX_NSET 0
#define TEX_DFLT 1
#define TEX_FRMB 2



typedef struct _FTEX {
    GLenum type;
    GLuint indx, xdim, ydim, zdim;
    struct _FVBO *orig;
} FTEX;

typedef struct _UNIF {
    GLenum type, draw;
    GLuint indx, cdat;
    GLvoid *pdat;
    GLchar *name;
} UNIF;

typedef struct _SHDR {
    GLuint prog, pvao, cuni;
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

/// renderbuffer-based framebuffer object
typedef struct _FRBO {
    GLuint fbuf,    /// framebuffer
           rbuf[2], /// renderbuffers for pixel and depth data
           pbuf[2]; /// pixel-transfer buffer array
    GLint  xdim,    /// width
           ydim,    /// height
           swiz;    /// pixel buffer switcher
} FRBO;



GLvoid MakeShaderSrc(GLchar ***sver, GLchar ***spix,
                     GLchar  **tver, GLchar  **tpix, ...);
GLvoid FreeShaderSrc(GLchar  **sver, GLchar  **spix);

FTEX  *BindTex(FVBO  *vobj, GLuint bind, GLuint mode);
GLuint MakeTex(FTEX  *retn, GLuint xdim, GLuint ydim, GLuint  zdim,
               GLenum trgt, GLenum wrap, GLint  tmag, GLint   tmin,
               GLenum type, GLenum frmt, GLenum mode, GLvoid *data);

FVBO  *MakeVBO(FVBO *prev, GLchar *vshd[], GLchar *pshd[], GLenum elem,
               GLuint catr, UNIF *patr, GLuint cuni, UNIF *puni, GLuint ctex);
GLvoid DrawVBO(FVBO *vobj, GLuint shad);
GLvoid FreeVBO(FVBO **vobj);

FRBO  *MakeRBO(GLint xdim, GLint ydim);
GLvoid BindRBO(FRBO *robj, GLboolean bind);
GLvoid FreeRBO(FRBO **robj);

GLint  LoadOpenGLFunctions();

extern GLvoid *LoadedOpenGLFunctions[];