#extension GL_ARB_texture_rectangle : enable

uniform sampler2DRect color0;
uniform sampler2DRect depth0;
uniform sampler2DRect color1;
uniform sampler2DRect depth1;

uniform mat4  T;
uniform int   n;
uniform float t;

void main()
{
    vec4 c0 = texture2DRect(color0, gl_FragCoord.xy);
    vec4 d0 = texture2DRect(depth0, gl_FragCoord.xy);
    vec4 c1 = texture2DRect(color1, gl_FragCoord.xy);
    vec4 d1 = texture2DRect(depth1, gl_FragCoord.xy);

    vec4 pn =          gl_FragCoord;
    vec4 p0 = T * vec4(gl_FragCoord.xy, d0.r, 1.0);
    vec4 p1 = T * vec4(gl_FragCoord.xy, d1.r, 1.0);

    p0 = p0 / p0.w;
    p1 = p1 / p1.w;

    vec4 B0 = vec4(0.0);
    vec4 B1 = vec4(0.0);

    for (int i = 0; i < n; i++)
    {
        float k = float(i) / float(n);
        vec4 b0 = texture2DRect(color0, mix(pn.xy, p0.xy, k));
        vec4 b1 = texture2DRect(color1, mix(pn.xy, p1.xy, k));
        B0.rgb += b0.a * b0.rgb;
        B1.rgb += b1.a * b1.rgb;
        B0.a   += b0.a;
        B1.a   += b1.a;
    }

    gl_FragColor = vec4(mix(B0.rgb * c0.a / B0.a,
    	                    B1.rgb * c1.a / B1.a, t), 1.0);
}
