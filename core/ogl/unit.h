#include "core.h"
#include <math.h>



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

FVBO   *MakeVBO(FVBO *prev, GLchar *vshd[], GLchar *pshd[], GLenum elem,
                GLuint catr, UNIF *patr, GLuint cuni, UNIF *puni, GLuint ctex);
GLvoid  DrawVBO(FVBO *vobj, GLuint shad);
GLvoid  FreeVBO(FVBO **vobj);



extern char **sver, **spix;
