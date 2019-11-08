uniform sampler2D texDecal; 

uniform vec3 camPos;
uniform vec3 lightPos;
uniform vec4 ambient;
uniform vec4 diffuse;
uniform vec4 specular;

varying vec3 normal;
varying vec3 vertex;

void main(void)
{
    vec3 normalVec = normal;
    vec3 lightVec = vec3( normalize(lightPos-vertex) );
    vec3 camVec   = vec3( normalize(camPos-vertex) );
    
    float l = clamp ( dot ( lightVec , normalVec ) ,0.0,1.0 );
    vec3  r = reflect ( -camVec, normalVec );
    float m = clamp ( dot ( r , lightVec ) ,0.0,1.0);
    float q = m*m; q=q*q;
        
//    gl_FragColor = vec4(normal,1.0) + 0.001 * (diffuse * l * 1.3 + specular * q + ambient);
    gl_FragColor = diffuse * l * 1.3 + specular * q + ambient;
}