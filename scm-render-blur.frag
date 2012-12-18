#version 120

uniform sampler2DRect color0;
uniform sampler2DRect depth0;

uniform mat4 L;
uniform mat4 I;

void main()
{
    vec3  c0 = texture2DRect(color0, gl_FragCoord.xy).rgb;
    float d0 = texture2DRect(depth0, gl_FragCoord.xy).r;

    gl_FragColor = vec4(c0, 1.0);
}
