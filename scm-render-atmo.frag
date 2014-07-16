#extension GL_ARB_texture_rectangle : enable

uniform sampler2DRect color0;
uniform sampler2DRect depth0;

uniform vec3 c;
uniform vec2 r;
uniform mat4 T;

void main()
{
    vec4 c0 = texture2DRect(color0, gl_TexCoord[0].xy);
    vec4 d0 = texture2DRect(depth0, gl_TexCoord[0].xy);

    vec4 pw = T * vec4(gl_FragCoord.xy, d0.r, 1.0);

    pw = pw / pw.w;

    vec3 v = (pw.xyz / r.y + 1.0) / 2.0;

    vec3 c = mix(c0.rgb, v, c0.a);

    gl_FragColor = vec4(c, 1.0);
}
