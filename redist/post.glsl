#version 430
layout(location = 0) out vec4 out_color;
layout(location = 1) uniform vec4 parameters;
layout(location = 2) uniform sampler2D tex;
in vec2 ftexcoord;

uniform float sepia_level;//0.00 1.00 0.01

float xres = parameters.x;
float yres = parameters.y;
float time = parameters.z;

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