#version 120

uniform sampler2DRect color0;
uniform sampler2DRect depth0;

uniform float n;
uniform mat4  L;
uniform mat4  I;
uniform vec2  size;


vec2 toeye(vec2 p)
{
    return 2.0 * p / size - 1.0;
}

vec2 topix(vec2 e)
{
    return size * (e + 1.0) / 2.0;
}


void main()
{
    float D = texture2DRect(depth0, gl_FragCoord.xy).r;

    vec4 en = vec4(2.0 * vec3(gl_FragCoord.xy / size, D) - 1.0, 1.0);
    vec4 wn = I * en;
    vec4 ep = L * wn;
    
    vec2 pn = gl_FragCoord.xy;
    vec2 pp = size * (ep.xy + 1.0) / 2.0;

    vec4  C = vec4(0.0);
    float a = 0.0;
    float i;

    for (i = 0; i < n; i++)
    {
        vec4 c = texture2DRect(color0, mix(pp, pn, i / n));
        C.rgb += c.a * c.rgb;
        C.a   += c.a;
    }

    gl_FragColor = vec4(C.rgb / C.a, 1.0);
}
