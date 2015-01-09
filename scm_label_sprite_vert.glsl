varying float clipDistance;

void main()
{
    gl_TexCoord[0] = gl_MultiTexCoord0;
    gl_FrontColor  = gl_FrontMaterial.diffuse * gl_Color;
    gl_Position    = ftransform();
    clipDistance   = dot(gl_ModelViewMatrix * gl_Vertex, gl_ClipPlane[0]);
}
