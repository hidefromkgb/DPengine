#include "../core.h"

#if defined(_WIN32)
    typedef char GLchar;
    #include <gl/gl.h>
    #include <windows.h>
    #define GL_GET_PROC_ADDR     wglGetProcAddress
    #define GL_COMPILE_STATUS               0x8B81
    #define GL_FRAGMENT_SHADER              0x8B30
    #define GL_VERTEX_SHADER                0x8B31
    #define GL_READ_ONLY_ARB                0x88B8
    #define GL_PIXEL_PACK_BUFFER_ARB        0x88EB
    #define GL_READ_FRAMEBUFFER             0x8CA8
    #define GL_DRAW_FRAMEBUFFER             0x8CA9
    #define GL_FRAMEBUFFER                  0x8D40
    #define GL_RENDERBUFFER                 0x8D41
    #define GL_COLOR_ATTACHMENT0            0x8CE0
    #define GL_TEXTURE0                     0x84C0
    #define GL_TEXTURE_CUBE_MAP             0x8513
    #define GL_TEXTURE_CUBE_MAP_POSITIVE_X  0x8515
    #define GL_TEXTURE_WRAP_R               0x8072
    #define GL_TEXTURE_2D_ARRAY             0x8C1A
    #define GL_ARRAY_BUFFER_ARB             0x8892
    #define GL_ELEMENT_ARRAY_BUFFER_ARB     0x8893
    #define GL_DEPTH_COMPONENT16            0x81A5
    #define GL_DEPTH_ATTACHMENT             0x8D00
    #define GL_STREAM_READ_ARB              0x88E1
    #define GL_STREAM_DRAW_ARB              0x88E0
    #define GL_STATIC_DRAW_ARB              0x88E4
    #define GL_CLAMP_TO_EDGE                0x812F
    #define GL_RG32UI                       0x823C
    #define GL_R8UI                         0x8232
    #define GL_RG_INTEGER                   0x8228
    #define GL_RED_INTEGER                  0x8D94
    #define GL_RGBA32F                      0x8814
    #define GL_BGRA                         0x80E1
    #define WGL_CONTEXT_MAJOR_VERSION_ARB   0x2091
    #define WGL_CONTEXT_MINOR_VERSION_ARB   0x2092
    #define WGL_CONTEXT_FLAGS_ARB           0x2094
#else
    #include <GL/gl.h>
    #include <GL/glx.h>
    #define GL_GET_PROC_ADDR     glXGetProcAddress
#endif



#define glUseProgramObjectARB      ((GLvoid  APIENTRY (*)(GLuint                                                                                           ))LoadedOpenGLFunctions[ 0])
#define glGetProgramInfoLog        ((GLvoid  APIENTRY (*)(GLuint ,  GLint  ,  GLint*   ,  GLchar*                                                          ))LoadedOpenGLFunctions[ 1])
#define glGetShaderInfoLog         ((GLvoid  APIENTRY (*)(GLuint ,  GLint  ,  GLint*   ,  GLchar*                                                          ))LoadedOpenGLFunctions[ 2])
#define glGetProgramiv             ((GLvoid  APIENTRY (*)(GLuint ,  GLenum ,  GLint*                                                                       ))LoadedOpenGLFunctions[ 3])
#define glGetShaderiv              ((GLvoid  APIENTRY (*)(GLuint ,  GLenum ,  GLint*                                                                       ))LoadedOpenGLFunctions[ 4])
#define glCreateProgram            ((GLuint  APIENTRY (*)(GLvoid                                                                                           ))LoadedOpenGLFunctions[ 5])
#define glDeleteProgram            ((GLvoid  APIENTRY (*)(GLuint                                                                                           ))LoadedOpenGLFunctions[ 6])
#define glCreateShader             ((GLuint  APIENTRY (*)(GLenum                                                                                           ))LoadedOpenGLFunctions[ 7])
#define glDeleteShader             ((GLvoid  APIENTRY (*)(GLuint                                                                                           ))LoadedOpenGLFunctions[ 8])
#define glAttachShader             ((GLvoid  APIENTRY (*)(GLuint ,  GLuint                                                                                 ))LoadedOpenGLFunctions[ 9])
#define glShaderSource             ((GLvoid  APIENTRY (*)(GLuint ,  GLuint ,  GLchar** ,  GLuint*                                                          ))LoadedOpenGLFunctions[10])
#define glCompileShader            ((GLvoid  APIENTRY (*)(GLuint                                                                                           ))LoadedOpenGLFunctions[11])
#define glLinkProgram              ((GLvoid  APIENTRY (*)(GLuint                                                                                           ))LoadedOpenGLFunctions[12])
#define glGetAttribLocation        ((GLint   APIENTRY (*)(GLuint ,  GLchar*                                                                                ))LoadedOpenGLFunctions[13])
#define glGetUniformLocation       ((GLint   APIENTRY (*)(GLuint ,  GLchar*                                                                                ))LoadedOpenGLFunctions[14])
#define glUniformMatrix4fv         ((GLvoid  APIENTRY (*)(GLint  ,  GLsizei,  GLboolean,  GLfloat*                                                         ))LoadedOpenGLFunctions[15])
#define glUniform1iv               ((GLvoid  APIENTRY (*)(GLint  ,  GLsizei,  GLint*                                                                       ))LoadedOpenGLFunctions[16])
#define glUniform1fv               ((GLvoid  APIENTRY (*)(GLint  ,  GLsizei,  GLfloat*                                                                     ))LoadedOpenGLFunctions[17])
#define glUniform2uiv              ((GLvoid  APIENTRY (*)(GLint  ,  GLsizei,  GLuint*                                                                      ))LoadedOpenGLFunctions[18])
#define glUniform2fv               ((GLvoid  APIENTRY (*)(GLint  ,  GLsizei,  GLfloat*                                                                     ))LoadedOpenGLFunctions[19])
#define glUniform3uiv              ((GLvoid  APIENTRY (*)(GLint  ,  GLsizei,  GLuint*                                                                      ))LoadedOpenGLFunctions[20])
#define glUniform3fv               ((GLvoid  APIENTRY (*)(GLint  ,  GLsizei,  GLfloat*                                                                     ))LoadedOpenGLFunctions[21])
#define glEnableVertexAttribArray  ((GLvoid  APIENTRY (*)(GLint                                                                                            ))LoadedOpenGLFunctions[22])
#define glVertexAttribPointer      ((GLvoid  APIENTRY (*)(GLuint ,  GLint  ,  GLenum   ,  GLboolean,  GLsizei,  GLvoid*                                    ))LoadedOpenGLFunctions[23])
#define glVertexAttribIPointer     ((GLvoid  APIENTRY (*)(GLuint ,  GLint  ,  GLenum   ,  GLsizei  ,  GLvoid*                                              ))LoadedOpenGLFunctions[24])
#define glGenVertexArrays          ((GLvoid  APIENTRY (*)(GLsizei,  GLuint*                                                                                ))LoadedOpenGLFunctions[25])
#define glBindVertexArray          ((GLvoid  APIENTRY (*)(GLuint                                                                                           ))LoadedOpenGLFunctions[26])
#define glDeleteVertexArrays       ((GLvoid  APIENTRY (*)(GLsizei,  GLuint*                                                                                ))LoadedOpenGLFunctions[27])
#define glActiveTextureARB         ((GLvoid  APIENTRY (*)(GLenum                                                                                           ))LoadedOpenGLFunctions[28])
#define glGenBuffersARB            ((GLvoid  APIENTRY (*)(GLsizei,  GLuint*                                                                                ))LoadedOpenGLFunctions[29])
#define glBindBufferARB            ((GLvoid  APIENTRY (*)(GLenum ,  GLuint                                                                                 ))LoadedOpenGLFunctions[30])
#define glBufferDataARB            ((GLvoid  APIENTRY (*)(GLenum ,  GLsizei,  GLvoid*  ,  GLenum                                                           ))LoadedOpenGLFunctions[31])
#define glBufferSubDataARB         ((GLvoid  APIENTRY (*)(GLenum ,  GLint  ,  GLsizei  ,  GLvoid*                                                          ))LoadedOpenGLFunctions[32])
#define glDeleteBuffersARB         ((GLvoid  APIENTRY (*)(GLsizei,  GLuint*                                                                                ))LoadedOpenGLFunctions[33])
#define glGenFramebuffers          ((GLvoid  APIENTRY (*)(GLsizei,  GLuint*                                                                                ))LoadedOpenGLFunctions[34])
#define glGenRenderbuffers         ((GLvoid  APIENTRY (*)(GLsizei,  GLuint*                                                                                ))LoadedOpenGLFunctions[35])
#define glDeleteFramebuffers       ((GLvoid  APIENTRY (*)(GLsizei,  GLuint*                                                                                ))LoadedOpenGLFunctions[36])
#define glDeleteRenderbuffers      ((GLvoid  APIENTRY (*)(GLsizei,  GLuint*                                                                                ))LoadedOpenGLFunctions[37])
#define glBindFramebuffer          ((GLvoid  APIENTRY (*)(GLenum ,  GLuint                                                                                 ))LoadedOpenGLFunctions[38])
#define glBindRenderbuffer         ((GLvoid  APIENTRY (*)(GLenum ,  GLuint                                                                                 ))LoadedOpenGLFunctions[39])
#define glRenderbufferStorage      ((GLvoid  APIENTRY (*)(GLenum ,  GLenum ,  GLsizei  ,  GLsizei                                                          ))LoadedOpenGLFunctions[40])
#define glFramebufferTexture2D     ((GLvoid  APIENTRY (*)(GLenum ,  GLenum ,  GLenum   ,  GLuint   ,  GLint                                                ))LoadedOpenGLFunctions[41])
#define glFramebufferRenderbuffer  ((GLvoid  APIENTRY (*)(GLenum ,  GLenum ,  GLenum   ,  GLuint                                                           ))LoadedOpenGLFunctions[42])
#define glMapBufferARB             ((GLvoid* APIENTRY (*)(GLenum ,  GLenum                                                                                 ))LoadedOpenGLFunctions[43])
#define glUnmapBufferARB           ((GLvoid  APIENTRY (*)(GLenum                                                                                           ))LoadedOpenGLFunctions[44])
#define glTexImage3D               ((GLvoid  APIENTRY (*)(GLenum ,  GLint  ,  GLenum   ,  GLsizei  ,  GLsizei,  GLsizei,  GLint,  GLenum,  GLenum,  GLvoid*))LoadedOpenGLFunctions[45])



#pragma pack(push, 1)
typedef struct _T2FV {
    float u, v;
} T2FV;

typedef struct _T3FV {
    float x, y, z;
} T3FV;

typedef struct _T4FV {
    float x, y, z, w;
} T4FV;

typedef struct _T2UV {
    uint32_t u, v;
} T2UV;

typedef struct _T3UV {
    uint32_t x, y, z;
} T3UV;

typedef struct _T4UV {
    uint32_t x, y, z, w;
} T4UV;
#pragma pack(pop)



long InitRendererOGL();
void MakeRendererOGL(ULIB  *ulib, ulong uniq,
                     T2UV **data, ulong size, ulong rgba);
void SizeRendererOGL(ulong xscr, ulong yscr);
void DrawRendererOGL(T2UV *data, ulong size);
void FreeRendererOGL(T2UV *data);



extern GLvoid *LoadedOpenGLFunctions[];
