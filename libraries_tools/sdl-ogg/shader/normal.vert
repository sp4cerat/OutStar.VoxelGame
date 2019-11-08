
varying vec3 normal;
varying vec3 vertex;

void main(void)
{
	normal		= frac(gl_Vertex.w * vec3(1, 1 / 256.0, 1 / 65536.0)) * 2 - 1;
	//normal		= frac(gl_Vertex.w * vec3(1, 1 / 256.0, 1 / 65536.0));
	//normal		= gl_Normal;//frac(gl_Vertex.w * vec3(1, 1 / 256.0, 1 / 65536.0)) * 2 - 1;
	vertex		= gl_Vertex.xyz;
    gl_Position	= gl_ModelViewProjectionMatrix * vec4(gl_Vertex.xyz,1.0);
}