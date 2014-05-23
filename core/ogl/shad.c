#include "unit.h"
#include <stdarg.h>



GLint ShaderProgramStatus(GLuint prog, GLboolean shad, GLenum parm) {
    GLchar buff[2048];
    GLint stat, slen;

    switch (shad) {
        case GL_FALSE:
            glGetProgramiv(prog, parm, &stat);
            if (stat != GL_TRUE) {
                glGetProgramInfoLog(prog, carrsz(buff), &slen, buff);
                printf("Shader program: %s\n", (GLchar*)buff);
            }
            return stat;

        default:
            glGetShaderiv(prog, parm, &stat);
            if (stat != GL_TRUE) {
                glGetShaderInfoLog(prog, carrsz(buff), &slen, buff);
                printf("Shader: %s\n", (GLchar*)buff);
            }
            return stat;
    }
}



GLboolean ShaderAdd(GLchar *fstr, GLuint prog, GLenum type) {
    GLuint slen, shad;

    if (!(shad = glCreateShader(type))) {
        printf("Shader allocation failed (%d)\n", glGetError());
        glDeleteProgram(prog);
        return GL_FALSE;
    }
    slen = strlen(fstr);
    glShaderSource(shad, 1, &fstr, &slen);
    glCompileShader(shad);

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
    GLchar *curp = NULL, *curv = NULL;
    GLint ctmp, step, name, iter = 0;
    GLboolean stop = GL_FALSE;

    while (pixl[iter]) iter++;

    SHDR *retn = (SHDR*)calloc(iter, sizeof(SHDR));

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
        glUseProgramObjectARB(retn[iter].prog);
        glGenVertexArrays(1, &retn[iter].pvao);
        glBindVertexArray(retn[iter].pvao);

        for (retn[iter].cuni = ctmp = 0; ctmp < cuni; ctmp++)
            if ((name = glGetUniformLocation(retn[iter].prog,
                                             puni[ctmp].name)) != -1)
                retn[iter].cuni++;

        for (retn[iter].puni = (UNIF*)malloc(retn[iter].cuni * sizeof(UNIF)),
             step = ctmp = 0; ctmp < cuni; ctmp++)
            if ((name = glGetUniformLocation(retn[iter].prog,
                                             puni[ctmp].name)) != -1) {
                retn[iter].puni[step] = puni[ctmp];
                retn[iter].puni[step++].name = (GLchar*)name;
            }
    }
    glUseProgramObjectARB(0);
    return retn;
}



char *shader(char *frmt, ...) {
    va_list list;
    char *retn;
    unsigned size;

    size = strlen(frmt);
    va_start(list, frmt);
    while ((retn = va_arg(list, char*)))
        size += strlen(retn);
    va_end(list);

    va_start(list, frmt);
    retn = (char*)calloc(1, size + 1);
    vsprintf(retn, frmt, list);
    va_end(list);

    return retn;
}



char *tver[] = {
/// surf - #: main vertex shader
"#version 130\n\
\
attribute vec3 vert;\
attribute uvec2 data;\
\
uniform sampler2D dims;\
uniform sampler2D coef;\
uniform vec2 disz;\
\
varying vec4 vtex;\
varying vec4 voff;\
\
%s\
\
void main() {\
    uint  indx = data.y & 0xFFFFFu;\
    uint  cpal = indx << 8;\
    ivec2 offs = ivec2(indx & txsz, indx >> txlg);\
    ivec2 vpos = ivec2((int(data.x) << 16) >> 16, int(data.x) >> 16);\
    vec4  vdim = texelFetch(dims, offs, 0);\
    vec4  vcoe = vec4(0.5 - (0.5 - vert.x) * (1.0 - ((data.y >> 29) & 0x2u)),\
                      0.5 + (0.5 - vert.y) * (1.0 - ((data.y >> 30) & 0x2u)),\
                      1.0, 1.0) * texelFetch(coef, offs, 0);\
    vtex = vec4(vert.x * vdim.x, vert.y * vdim.y, vdim.x, 0.0);\
    voff = vec4(vdim.z, vdim.w + ((data.y >> 20) & 0x3FFu) * vdim.x * vdim.y,\
                cpal & txsz, cpal >> txlg);\
    gl_Position = vec4((vpos.x + vdim.x * vcoe.x) * disz.x - 1.0,\
                      -(vpos.y - vdim.y * vcoe.y) * disz.y + 1.0,\
                       -vpos.y * disz.y * 0.5 + 1.0, 1.0);\
}",

NULL};



char *tpix[] = {
/// surf - 0: display
"#version 130\n\
\
uniform usampler2DArray atex;\
uniform sampler2D apal;\
\
varying vec4 vtex;\
varying vec4 voff;\
\
%s\
\
void main() {\
    uint  offs = uint(voff.y) + uint(vtex.x) + uint(vtex.y) * uint(vtex.z);\
    uvec4 indx = texelFetch(atex,\
                            ivec3(offs & txsz, offs >> txlg, voff.x), 0);\
    if (indx.x == 0xFFu) discard;\
    gl_FragColor = texelFetch(apal, ivec2(voff.z + indx.x, voff.w), 0);\
}",

NULL};



char **sver, **spix;



GLvoid MakeShaderSrc(GLuint logt) {
    char cons[96];
    int iter;

    sver = calloc(1, sizeof(tver));
    spix = calloc(1, sizeof(tpix));
    sprintf(cons, "const uint txsz = uint(%u), txlg = uint(%u);",
           (1 << logt) - 1, logt);

    for (iter = carrsz(tver) - 2; iter >= 0; iter--)
        sver[iter] = shader(tver[iter], cons, NULL);

    for (iter = carrsz(tpix) - 2; iter >= 0; iter--)
        spix[iter] = shader(tpix[iter], cons, NULL);
}



GLvoid FreeShaderSrc() {
    free(sver);
    free(spix);
}
