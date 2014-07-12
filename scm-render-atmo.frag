#extension GL_ARB_texture_rectangle : enable

uniform sampler2DRect color0;

uniform float t;

void main()
{
	gl_FragColor = texture2DRect(color0, gl_TexCoord[0].xy);
}
