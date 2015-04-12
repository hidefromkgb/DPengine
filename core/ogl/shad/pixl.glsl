#version 110

uniform sampler3D atex;
uniform sampler2D apal;
uniform vec4 hitd;

varying vec4 vtex;
varying vec4 voff;

const vec4 coef = vec4(1.0, 0.0, 0.5, 255.0);

void main() {
    vec4 pixl = floor(vtex);
    pixl = texture3D(atex, (floor((pixl.yyw * pixl.zzw + pixl.xxw + voff.xxy)
                                 * hitd.wzw) + coef.zzz) * hitd.zzx);
    gl_FragColor = texture2D(apal, hitd.zy * (pixl.xx * coef.wy + voff.zw));
    if (pixl.x == coef.x) discard;
}
