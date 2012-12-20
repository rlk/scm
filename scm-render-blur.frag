#version 120

uniform sampler2DRect color0;
uniform sampler2DRect depth0;

uniform vec2 size;
uniform mat4 T;
uniform int  n;

void main()
{
    vec4 c0 = texture2DRect(color0, gl_FragCoord.xy);
    vec4 d0 = texture2DRect(depth0, gl_FragCoord.xy);

    vec4 pn =          gl_FragCoord;
    vec4 pp = T * vec4(gl_FragCoord.xy, d0.r, 1.0);

    pp = pp / pp.w;

    vec4 C = vec4(0.0);

    for (int i = 0; i < n; i++)
    {
        vec4 c = texture2DRect(color0, mix(pn.xy, pp.xy, float(i) / n));
        C.rgb += c.a * c.rgb;
        C.a   += c.a;
    }

    gl_FragColor = vec4(C.rgb * c0.a / C.a, 1.0);
}
