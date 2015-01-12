#extension GL_ARB_texture_rectangle : enable

uniform sampler2DRect color0;
uniform sampler2DRect depth0;

uniform vec3 p;

uniform vec3  atmo_c;
uniform vec2  atmo_r;
uniform mat4  atmo_T;
uniform float atmo_H;
uniform float atmo_P;

float density(vec3 p, float d)
{
    vec3 L = gl_LightSource[0].position.xyz;

    float P = atmo_P * exp(-(length(p) - atmo_r.x) / atmo_H);

    float e = dot(normalize(p), L);
    float l = smoothstep(-0.1, 0.1, e);

    return mix(0.0, clamp(d * P, 0.0, 1.0), l);
}

void main()
{
    // Color and depth of the current fragment

    vec4 c = texture2DRect(color0, gl_TexCoord[0].xy);
    vec4 d = texture2DRect(depth0, gl_TexCoord[0].xy);

    // World-space position of the current fragment

    vec4 w = atmo_T * vec4(gl_TexCoord[0].xy, d.r, 1.0);
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

        // If either intersection occurs in front of the viewer...

        if (tp > 0.0 || tm > 0.0)
        {
            // Locate the beginning and end of the atmosphere segment.

            float t0 = max(min(tp, tm), 0.0);
            float t1 = max(max(tp, tm), 0.0);

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
    }

    gl_FragColor = vec4(c.rgb, 1.0);
}
