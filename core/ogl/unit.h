#include "core.h"

#if defined(_WIN32)
    #include <gl/gl.h>
    #define GL_GET_PROC_ADDR wglGetProcAddress
#else
    #include <GL/gl.h>
    #include <GL/glx.h>
    #define GL_GET_PROC_ADDR glXGetProcAddress
#endif



#define glUseProgramObjectARB      ((GLvoid APIENTRY (*)(GLuint                                                       ))LoadedOpenGLFunctions[ 0])
#define glGetProgramInfoLog        ((GLvoid APIENTRY (*)(GLuint ,  GLint  ,  GLint*   ,  GLchar*                      ))LoadedOpenGLFunctions[ 1])
#define glGetShaderInfoLog         ((GLvoid APIENTRY (*)(GLuint ,  GLint  ,  GLint*   ,  GLchar*                      ))LoadedOpenGLFunctions[ 2])
#define glGetProgramiv             ((GLvoid APIENTRY (*)(GLuint ,  GLenum ,  GLint*                                   ))LoadedOpenGLFunctions[ 3])
#define glGetShaderiv              ((GLvoid APIENTRY (*)(GLuint ,  GLenum ,  GLint*                                   ))LoadedOpenGLFunctions[ 4])
#define glCreateProgram            ((GLuint APIENTRY (*)(GLvoid                                                       ))LoadedOpenGLFunctions[ 5])
#define glDeleteProgram            ((GLvoid APIENTRY (*)(GLuint                                                       ))LoadedOpenGLFunctions[ 6])
#define glCreateShader             ((GLuint APIENTRY (*)(GLenum                                                       ))LoadedOpenGLFunctions[ 7])
#define glDeleteShader             ((GLvoid APIENTRY (*)(GLuint                                                       ))LoadedOpenGLFunctions[ 8])
#define glAttachShader             ((GLvoid APIENTRY (*)(GLuint ,  GLuint                                             ))LoadedOpenGLFunctions[ 9])
#define glShaderSource             ((GLvoid APIENTRY (*)(GLuint ,  GLuint ,  GLchar** ,  GLuint*                      ))LoadedOpenGLFunctions[10])
#define glCompileShader            ((GLvoid APIENTRY (*)(GLuint                                                       ))LoadedOpenGLFunctions[11])
#define glLinkProgram              ((GLvoid APIENTRY (*)(GLuint                                                       ))LoadedOpenGLFunctions[12])
#define glGetAttribLocation        ((GLint  APIENTRY (*)(GLuint ,  GLchar*                                            ))LoadedOpenGLFunctions[13])
#define glGetUniformLocation       ((GLint  APIENTRY (*)(GLuint ,  GLchar*                                            ))LoadedOpenGLFunctions[14])
#define glUniformMatrix4fv         ((GLvoid APIENTRY (*)(GLint  ,  GLsizei,  GLboolean,  GLfloat*                     ))LoadedOpenGLFunctions[15])
#define glUniform1iv               ((GLvoid APIENTRY (*)(GLint  ,  GLsizei,  GLint*                                   ))LoadedOpenGLFunctions[16])
#define glUniform1fv               ((GLvoid APIENTRY (*)(GLint  ,  GLsizei,  GLfloat*                                 ))LoadedOpenGLFunctions[17])
#define glUniform2uiv              ((GLvoid APIENTRY (*)(GLint  ,  GLsizei,  GLuint*                                  ))LoadedOpenGLFunctions[18])
#define glUniform2fv               ((GLvoid APIENTRY (*)(GLint  ,  GLsizei,  GLfloat*                                 ))LoadedOpenGLFunctions[19])
#define glUniform3uiv              ((GLvoid APIENTRY (*)(GLint  ,  GLsizei,  GLuint*                                  ))LoadedOpenGLFunctions[20])
#define glUniform3fv               ((GLvoid APIENTRY (*)(GLint  ,  GLsizei,  GLfloat*                                 ))LoadedOpenGLFunctions[21])
#define glEnableVertexAttribArray  ((GLvoid APIENTRY (*)(GLint                                                        ))LoadedOpenGLFunctions[22])
#define glVertexAttribPointer      ((GLvoid APIENTRY (*)(GLuint ,  GLint  ,  GLenum   ,  GLboolean,  GLsizei,  GLvoid*))LoadedOpenGLFunctions[23])
#define glVertexAttribIPointer     ((GLvoid APIENTRY (*)(GLuint ,  GLint  ,  GLenum   ,  GLsizei  ,  GLvoid*          ))LoadedOpenGLFunctions[24])
#define glGenVertexArrays          ((GLvoid APIENTRY (*)(GLsizei,  GLuint*                                            ))LoadedOpenGLFunctions[25])
#define glBindVertexArray          ((GLvoid APIENTRY (*)(GLuint                                                       ))LoadedOpenGLFunctions[26])
#define glDeleteVertexArrays       ((GLvoid APIENTRY (*)(GLsizei,  GLuint*                                            ))LoadedOpenGLFunctions[27])
#define glActiveTextureARB         ((GLvoid APIENTRY (*)(GLenum                                                       ))LoadedOpenGLFunctions[28])
#define glGenBuffersARB            ((GLvoid APIENTRY (*)(GLsizei,  GLuint*                                            ))LoadedOpenGLFunctions[29])
#define glBindBufferARB            ((GLvoid APIENTRY (*)(GLenum ,  GLuint                                             ))LoadedOpenGLFunctions[30])
#define glBufferDataARB            ((GLvoid APIENTRY (*)(GLenum ,  GLsizei,  GLvoid*  ,  GLenum                       ))LoadedOpenGLFunctions[31])
#define glBufferSubDataARB         ((GLvoid APIENTRY (*)(GLenum ,  GLint  ,  GLsizei  ,  GLvoid*                      ))LoadedOpenGLFunctions[32])
#define glDeleteBuffersARB         ((GLvoid APIENTRY (*)(GLsizei,  GLuint*                                            ))LoadedOpenGLFunctions[33])
#define glGenFramebuffers          ((GLvoid APIENTRY (*)(GLsizei,  GLuint*                                            ))LoadedOpenGLFunctions[34])
#define glGenRenderbuffers         ((GLvoid APIENTRY (*)(GLsizei,  GLuint*                                            ))LoadedOpenGLFunctions[35])
#define glDeleteFramebuffers       ((GLvoid APIENTRY (*)(GLsizei,  GLuint*                                            ))LoadedOpenGLFunctions[36])
#define glDeleteRenderbuffers      ((GLvoid APIENTRY (*)(GLsizei,  GLuint*                                            ))LoadedOpenGLFunctions[37])
#define glBindFramebuffer          ((GLvoid APIENTRY (*)(GLenum ,  GLuint                                             ))LoadedOpenGLFunctions[38])
#define glBindRenderbuffer         ((GLvoid APIENTRY (*)(GLenum ,  GLuint                                             ))LoadedOpenGLFunctions[39])
#define glRenderbufferStorage      ((GLvoid APIENTRY (*)(GLenum ,  GLenum ,  GLsizei  ,  GLsizei                      ))LoadedOpenGLFunctions[40])
#define glFramebufferTexture2D     ((GLvoid APIENTRY (*)(GLenum ,  GLenum ,  GLenum   ,  GLuint   ,  GLint            ))LoadedOpenGLFunctions[41])
#define glFramebufferRenderbuffer  ((GLvoid APIENTRY (*)(GLenum ,  GLenum ,  GLenum   ,  GLuint                       ))LoadedOpenGLFunctions[42])



#define carrsz(a) (sizeof(a) / sizeof(*a))

#define DEG_CRAD (M_PI / 180.0)
#define RAD_CDEG (180.0 / M_PI)

#define TEX_NSET 0
#define TEX_DFLT 1
#define TEX_FRMB 2

#define UNI_T44F 0
#define UNI_T1II 1
#define UNI_T1FI 2
#define UNI_T1IV 3
#define UNI_T1FV 4
#define UNI_T2UV 5
#define UNI_T2FV 6
#define UNI_T3UV 7
#define UNI_T3FV 8



typedef struct _FTEX {
    GLenum type;
    GLuint indx, xdim, ydim, zdim;
    struct _FVBO *orig;
} FTEX;

typedef struct _UNIF {
    GLenum type, draw;
    GLuint cdat;
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

typedef struct _FRBO {
    GLuint frmb, rndb;
    GLuint tfrn, tbak;
    FVBO *vobj;
} FRBO;



GLvoid MakeShaderSrc(GLuint logt);
GLvoid FreeShaderSrc();

GLint      ShaderProgramStatus(GLuint prog, GLboolean shad, GLenum parm);
GLboolean  ShaderAdd(GLchar *fstr, GLuint prog, GLenum type);
SHDR      *MakeShaderList(GLchar *vert[], GLchar *pixl[],
                          GLuint cuni, UNIF *puni, GLuint *cshd);

FTEX   *BindTex(FVBO  *vobj, GLuint bind, GLuint mode);
GLvoid  MakeTex(FTEX  *retn, GLuint xdim, GLuint ydim, GLuint  zdim,
                GLenum trgt, GLenum wrap, GLint  tmag, GLint   tmin,
                GLenum type, GLenum frmt, GLenum mode, GLvoid *data);

FRBO   *MakeRBO(FVBO *vobj, GLuint tfrn, GLuint tbak);
GLvoid  DrawRBO(FRBO *robj, GLuint shad);
GLvoid  SwapRBO(FRBO *robj);
GLvoid  FreeRBO(FRBO **robj);

FVBO   *MakeVBO(FVBO *prev, GLchar *vshd[], GLchar *pshd[], GLenum elem,
                GLuint catr, UNIF *patr, GLuint cuni, UNIF *puni, GLuint ctex);
GLvoid  DrawVBO(FVBO *vobj, GLuint shad);
GLvoid  FreeVBO(FVBO **vobj);



extern GLvoid *LoadedOpenGLFunctions[];
extern char **sver, **spix;
