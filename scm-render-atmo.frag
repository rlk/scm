#extension GL_ARB_texture_rectangle : enable

uniform sampler2DRect color0;
uniform sampler2DRect depth0;

uniform vec3 p;

uniform vec3 atmo_c;
uniform vec2 atmo_r;
uniform mat4 atmo_T;

/*
float density(vec3 p, float d)
{
    float k = (length(p) - atmo_r.x) / (atmo_r.y - atmo_r.x);
    return clamp(d * (1.0 - k) / 500000.0, 0.0, 1.0);
}
*/

float density(vec3 p, float d)
{
    float H = 11100.0;
    float P = 10.0 * exp(-(length(p) - atmo_r.x) / H);
    return clamp(P, 0.0, 1.0);
}

void main()
{
    // Lightsource direction

    vec3 L = normalize(gl_LightSource[0].position.xyz);

    // Color and depth of the current fragment

    vec4 c = texture2DRect(color0, gl_TexCoord[0].xy);
    vec4 d = texture2DRect(depth0, gl_TexCoord[0].xy);

    // World-space position of the current fragment

    vec4 w = atmo_T * vec4(gl_FragCoord.xy, d.r, 1.0);
    vec3 q = w.xyz / w.w;

    // Ray to cast from p toward q.

    vec3 v = normalize(q - p);

    // Solve for the intersection with the outer atmosphere.

    float A = dot(v, v);
    float B = dot(v, p) * 2.0;
    float C = dot(p, p) - atmo_r.y * atmo_r.y;
    float D = B * B - 4.0 * A * C;

    // If the ray crosses the sphere...

    if (D > 0.0)
    {
        // Compute the nearest and farthest points of intersection.

        float tp = (-B + sqrt(D)) / (2.0 * A);
        float tm = (-B - sqrt(D)) / (2.0 * A);

        float t0 = max(min(tp, tm), 0.0);
        float t1 =     max(tp, tm);

        vec3 a =     p + v * t0;
        vec3 b = mix(p + v * t1, q, c.a);

        // Integrate over the segment from a to b.

        const int n = 16;

        float d = distance(a, b) / float(n);

        for (int i = 0; i < n; i++)
        {
            vec3 u = mix(a, b, float(i) / float(n));

            c.rgb = mix(c.rgb, atmo_c, density(u, d));
        }
    }
    gl_FragColor = vec4(c.rgb, 1.0);
}
