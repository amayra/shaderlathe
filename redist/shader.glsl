shader_id RAYMARCH

#version 430
layout(location = 0) out vec4 out_color;
layout(location = 1) uniform vec4 parameters;
layout(location = 2) uniform sampler2D tex;
in vec2 ftexcoord;
uniform float crap;//-1. 30.0 1.1
float xres = parameters.x;
float yres = parameters.y;
float time = parameters.z;

float Pi = 3.1415296;
#define INF 100000.0

vec3 rotatex(in vec3 p, float ang) 
{ return vec3(p.x, p.y*cos(ang) - p.z*sin(ang), p.y*sin(ang) + p.z*cos(ang)); }
vec3 rotatey(in vec3 p, float ang) 
{ return vec3(p.x*cos(ang) - p.z*sin(ang), p.y, p.x*sin(ang) + p.z*cos(ang)); }
vec3 rotatez(in vec3 p, float ang) 
{ return vec3(p.x*cos(ang) - p.y*sin(ang), p.x*sin(ang) + p.y*cos(ang), p.z); }

float opS( float d1, float d2 )
{
    return max(-d2,d1);
}

vec2 opU( vec2 d1, vec2 d2 )
{
	return (d1.x<d2.x) ? d1 : d2;
}

vec3 opRep( vec3 p, vec3 c )
{
    return mod(p,c)-0.5*c;
}

vec3 opTwist( vec3 p )
{
    float  c = cos(10.0*p.y+10.0);
    float  s = sin(10.0*p.y+10.0);
    mat2   m = mat2(c,-s,s,c);
    return vec3(m*p.xz,p.y);
}

float sdPlane( vec3 p )
{
	return p.y;
}

float sdBox( vec3 p, vec3 b )
{
  vec3 d = abs(p) - b;
  return min(max(d.x,max(d.y,d.z)),0.0) +
         length(max(d,0.0)) ;
}

float sdCross( in vec3 p )
{
  float da = sdBox(p.xyz,vec3(INF,1.0,1.0));
  float db = sdBox(p.yzx,vec3(1.0,INF,1.0));
  float dc = sdBox(p.zxy,vec3(1.0,1.0,INF));
  return min(da,min(db,dc));
}




float scene(vec3 p)
{
	p = rotatex(p, 0.18*parameters.z);
	p = rotatez(p, 0.20*parameters.z);
	p = rotatey(p, 0.22*parameters.z);

	float d0 = length(max(abs(p) - 0.5, 0.0)) - 0.0 + clamp(sin((p.x +p.y + p.z)*crap)
	*0.03, 0.0, 1.0);
	float d1 = length(p)-0.3;
	return sin(max(d0, -d1));
}

float softshadow( in vec3 ro, in vec3 rd, in float mint, in float tmax )
{
	float res = 1.0;
    float t = mint;
    for( int i=0; i<8; i++ )
    {
		float h = scene( ro + rd*t ).x;
        res = min( res, 8.0*h/t );
        t += clamp( h, 0.02, 0.10 );
        if( h<0.001 || t>tmax ) break;
    }
    return clamp( res, 0.0, 1.0 );

}

vec3 get_normal(vec3 p)
{
	vec3 eps = vec3(0.11, 0.0, 0.0);
	float nx = scene(p + eps.xyy) - scene(p - eps.xyy);
	float ny = scene(p + eps.yxy) - scene(p - eps.yxy);
	float nz = scene(p + eps.yyx) - scene(p - eps.yyx);
	return normalize(vec3(nx, ny, nz));
}

float AO( in vec3 pos, in vec3 nor )
{
	float occ = 0.0;
    float sca = 1.0;
    for( int i=0; i<5; i++ )
    {
        float hr = 0.01 + 0.12*float(i)/4.0;
        vec3 aopos =  nor * hr + pos;
        float dd = scene( aopos ).x;
        occ += -(dd-hr)*sca;
        sca *= 0.95;
    }
    return clamp( 1.0 - 3.0*occ, 0.0, 1.0 );    
}

void main(void)
{
	vec2 p = 2.0 * gl_FragCoord.xy / parameters.xy - 1.0;
	p.x *= parameters.x / parameters.y;

	vec3 ro = vec3(0.0, 0.0, 1.7);
	vec3 rd = normalize(vec3(p, -1.4));
	vec3 color = (1.0 - vec3(length(p*0.5)))*0.2;

	vec3 pos = ro;
	float dist = 0.0;
	for (int i = 0; i < 64; i++)
	{
		float d = scene(pos);
		pos += rd*d;
		dist += d;
	}

	if (dist < 100.0)
	{
		vec3 n = get_normal(pos);
		vec3 r = reflect(normalize(pos - ro), n);
		vec3 h = -normalize(n + pos - ro);
		float diff = 1.0*clamp(dot(n, normalize(vec3(1, 1, 1))), 0.0, 1.0);
		float diff2 = 0.2*clamp(dot(n, normalize(vec3(0.7, -1, 0.5))), 0.0, 1.0);
		float diff3 = 0.1*clamp(dot(n, normalize(vec3(-0.7, -0.4, 0.7))), 0.0, 1.0);
		//float spec = pow(clamp(dot(r, normalize(vec3(1,1,1))), 0.0, 1.0), 50.0); 
		float spec = pow(clamp(dot(h, normalize(vec3(1, 1, 1))), 0.0, 1.0), 50.0);
		float amb = 0.2 + pos.y;
		color = diff*vec3(1, 1, 1) + diff2*vec3(1, 0, 0) + diff3*vec3(1, 0, 1) + spec*vec3(1, 1, 1) + amb*vec3(0.2, 0.2, 0.2);
		color /= dist;
	}
	out_color = vec4(color, 1.0);
}


shader_id POST1

#version 430
layout(location = 0) out vec4 out_color;
layout(location = 1) uniform vec4 parameters;
layout(location = 2) uniform sampler2D tex;
in vec2 ftexcoord;
float xres = parameters.x;
float yres = parameters.y;
float time = parameters.z;
uniform float sepia_level;//0.00 1.00 0.01



vec4 Sepia( in vec4 color )
{
    return vec4(
          clamp(color.r * 0.393 + color.g * 0.769 + color.b * 0.189, 0.0, 1.0)
        , clamp(color.r * 0.349 + color.g * 0.686 + color.b * 0.168, 0.0, 1.0)
        , clamp(color.r * 0.272 + color.g * 0.534 + color.b * 0.131, 0.0, 1.0)
        , color.a
    );
}

void main(void)
{
	vec4 color =  texture(tex, ftexcoord);

	vec4 sep_col = mix(color, Sepia(color), clamp(sepia_level,0.0,1.0) );
	out_color = vec4(sep_col.rgb, 1.0);
}