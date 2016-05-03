uniform vec3 ssyf;
#ifdef PS
uniform vec4 ssf[4];
varying float ssv;
#endif

attribute vec2 attr0;
attribute vec3 attr1;

varying vec2 texCoord;

void main()
{
    gl_Position = vec4( attr1.x, attr1.y, attr1.z, 1.0 );
    texCoord = attr0;
#ifdef PS
ssv=dot(ssf[0],gl_Position);
#endif
gl_Position.xy += ssyf.xy*gl_Position.w;
gl_Position.y*=ssyf.z;
gl_Position.z=gl_Position.z*2.0-gl_Position.w;
}
