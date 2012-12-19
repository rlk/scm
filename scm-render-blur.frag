#version 120

uniform sampler2DRect color0;
uniform sampler2DRect depth0;

uniform vec2 size;
uniform mat4 L;
uniform mat4 I;
uniform int  n;

vec4 toneg(vec4 p)
{
    return (p * 2.0) - 1.0;
}

vec4 topos(vec4 n)
{
    return (n + 1.0) / 2.0;
}

void main()
{
    float d0 = texture2DRect(depth0, gl_FragCoord.xy).r;

    vec2 pn = gl_FragCoord.xy;
    vec2 pp = size * topos(L * I * toneg(vec4(pn / size, d0, 1.0))).xy;

    vec4 C = vec4(0.0);

    for (int i = 0; i < n; i++)
    {
        vec4 c = texture2DRect(color0, mix(pn, pp, float(i) / n));
        C.rgb += c.a * c.rgb;
        C.a   += c.a;
    }

    gl_FragColor = vec4(C.rgb / C.a, 1.0);
}
