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

    for (int i = 0; i < n; i++)
    {
        float k = float(i) / float(n);
        vec4  b = texture2DRect(color0, mix(pn.xy, pp.xy, k));
        B.rgb  += b.a * b.rgb;
        B.a    += b.a;
    }

    gl_FragColor = vec4(B.rgb * c0.a / B.a, 1.0);
}
