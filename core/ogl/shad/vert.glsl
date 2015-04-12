#version 110

attribute vec3 vert;

uniform sampler2D data;
uniform sampler2D dims;
uniform sampler2D bank;
uniform vec4 disz;
uniform vec4 hitd;

varying vec4 vtex;
varying vec4 voff;

const vec4 orts = vec4(-1.0, 1.0, 0.5, 0.0);
const vec4 coef = vec4(-0.125, 0.250, 0.500, 0.000);

void main() {
    vec4 vdat = texture2D(data, vec2(hitd.z, disz.z) *
                               (floor(vert.zz * hitd.wz) + orts.zz));
    vec4 indx = vdat.wwww * orts.wwzw + hitd.wzww * (orts.zzxw * 2.0)
              * floor(vdat.wwww * coef.yyyy + orts.xxww);
    vec2 offs = vec2(hitd.z, disz.w) * (floor(indx.xy) + orts.zz);
    vec4 vdim = texture2D(dims, offs);
    vec4 vbnk = texture2D(bank, offs);
    vtex = (orts.xzzw * 4.0) * vdim.zwzw *
          ((floor(indx.zzww) * orts.yxww + indx.zwww - coef.yzww) *
           (vert.xyzz - coef.zzww) + coef.xyzw);
    indx = floor(vec4(vdat.z * vdim.z * vdim.w, indx.wxy * 256.0))
         + vbnk.yxww * orts.yyww + orts.wwzz;
    voff = (indx.x < vbnk.z)? indx.xyzw : indx.xyzw
         + floor((indx.xxxx - vbnk.zzzz) / vbnk.wwww)
         * (vbnk.wwww * orts.xwww + orts.wyww)
         + (vbnk.zzzz * orts.xwww + orts.wyww);
    gl_Position = (orts.yyww * vert.xyzz  * vdim.zwxx * vdim.xyzw
                -  orts.xyzw * vdat.xyyy) * disz.xyyy + orts.xyyy;
}
