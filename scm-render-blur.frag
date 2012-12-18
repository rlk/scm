#version 120
#extension GL_ARB_texture_rectangle : require

uniform sampler2DRect color0;
uniform sampler2DRect depth0;

uniform mat4 L;
uniform mat4 I;
uniform vec2 size;

void main()
{
    vec4 c0 = texture2DRect(color0, gl_FragCoord.xy);
    vec4 d0 = texture2DRect(depth0, gl_FragCoord.xy);

//  vec4 e0 = vec4(2.0 * vec3(gl_FragCoord.xy / size, d0) - 1.0, 1.0);
//  vec4 w  = I * e0;
//  vec4 e1 = L * w;

    gl_FragColor = vec4(vec3(d0 * 0.5), 1.0);
}
