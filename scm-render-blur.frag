#extension GL_ARB_texture_rectangle : enable

uniform sampler2DRect color0;
uniform sampler2DRect depth0;

uniform mat4 T;
uniform int  n;

void main()
{
    vec4 c0 = texture2DRect(color0, gl_FragCoord.xy);
    vec4 d0 = texture2DRect(depth0, gl_FragCoord.xy);

    vec4 pn =          gl_FragCoord;
    vec4 pp = T * vec4(gl_FragCoord.xy, d0.r, 1.0);

    pp = pp / pp.w;

    vec4 B = vec4(0.0);
    vec4 b;

    for (int i = 0; i < n; i++)
    {
        b  = texture2DRect(color0, mix(pn.xy, pp.xy, float(i) / float(n)));
        B += vec4(b.rgb, 1.0);
    }

    gl_FragColor = vec4(B.rgb / B.a, 1.0);
}
