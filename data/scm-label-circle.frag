void main()
{
    float d = distance(gl_TexCoord[0].xy, vec2(0.5));
    vec2  f = fwidth(gl_TexCoord[0].xy);
    float l = min(f.x, f.y);
    float k = 1.0 - smoothstep(0.0, l, abs(0.5 - l - d));

    gl_FragColor = vec4(gl_Color.rgb, gl_Color.a * k);
}
