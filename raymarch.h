GLuint raymarch_vao, raymarch_texture;
shader_id raymarch_shader;
#define GLSL(src) #src

const char vertex_source[] =
"#version 330\n"
"layout(location = 0) in vec4 vposition;\n"
"layout(location = 1) in vec2 vtexcoord;\n"
"out vec2 ftexcoord;\n"
"void main() {\n"
"   ftexcoord = vtexcoord;\n"
"   gl_Position =vec4(vposition.xy, 0.0f, 1.0f);\n"
"}\n";



const char* scene1_shader = GLSL(
\n#version 430\n
layout(location = 0) out vec4 out_color;
layout(location = 1) uniform vec4 parameters;
layout(location = 2) uniform sampler2D tex;
in vec2 ftexcoord;
vec3 rotatex(in vec3 p, float ang) { return vec3(p.x, p.y*cos(ang) - p.z*sin(ang), p.y*sin(ang) + p.z*cos(ang)); }

vec3 rotatey(in vec3 p, float ang) { return vec3(p.x*cos(ang) - p.z*sin(ang), p.y, p.x*sin(ang) + p.z*cos(ang)); }

vec3 rotatez(in vec3 p, float ang) { return vec3(p.x*cos(ang) - p.y*sin(ang), p.x*sin(ang) + p.y*cos(ang), p.z); }

float scene(vec3 p)
{
	p = rotatex(p, 0.18*parameters.z);
	p = rotatez(p, 0.20*parameters.z);
	p = rotatey(p, 0.22*parameters.z);

	float d0 = length(max(abs(p) - 0.5, 0.0)) - 0.01 + clamp(sin((p.x +p.y + p.z)*20.0)*0.03, 0.0, 1.0);
	float d1 = length(p) - 0.5;
	return sin(max(d0, -d1));
}

vec3 get_normal(vec3 p)
{
	vec3 eps = vec3(0.01, 0.0, 0.0);
	float nx = scene(p + eps.xyy) - scene(p - eps.xyy);
	float ny = scene(p + eps.yxy) - scene(p - eps.yxy);
	float nz = scene(p + eps.yyx) - scene(p - eps.yyx);
	return normalize(vec3(nx, ny, nz));
}

void main(void)
{
	vec2 p = 2.0 * gl_FragCoord.xy / parameters.xy - 1.0;
	
	p.x *= parameters.x / parameters.y;

	vec3 ro = vec3(0.0, 0.0, 1.7);
	vec3 rd = normalize(vec3(p.x, p.y, -1.4));
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
);

void init_raymarch()
{
	raymarch_shader = initShader( vertex_source, (const char*)scene1_shader);
	// get texture uniform location


	// vao and vbo handle
	GLuint vbo;

	// generate and bind the vao
	glGenVertexArrays(1, &raymarch_vao);
	glBindVertexArray(raymarch_vao);

	// generate and bind the vertex buffer object, to be used with VAO
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	// data for a fullscreen quad (this time with texture coords)
	// we use the texture coords for whenever a LUT is loaded
	typedef struct
	{
		float   x;
		float   y;
		float   z;
		float   u;
		float   v;
	} VBufVertex;
	VBufVertex vertexData[] = {
		//  X     Y     Z           U     V     
		1.0f, 1.0f, 0.0f, 1.0f, 1.0f, // vertex 0
		-1.0f, 1.0f, 0.0f, 0.0f, 1.0f, // vertex 1
		1.0f, -1.0f, 0.0f, 1.0f, 0.0f, // vertex 2
		1.0f, -1.0f, 0.0f, 1.0f, 0.0f, // vertex 2
		-1.0f, -1.0f, 0.0f, 0.0f, 0.0f, // vertex 3
		-1.0f, 1.0f, 0.0f, 0.0f, 1.0f, // vertex 1
	}; // 6 vertices with 5 components (floats) each
	// fill with data
	glBufferData(GL_ARRAY_BUFFER, sizeof(VBufVertex) * 6, vertexData, GL_STATIC_DRAW);
	// set up generic attrib pointers
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(VBufVertex), (void*)offsetof(VBufVertex, x));
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(VBufVertex), (void*)offsetof(VBufVertex, u));
	// "unbind" voa
	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	int width = 32;
	int height = 32;
	GLubyte image[4 * 32 * 32] = { 0 };
	for (int j = 0; j < height; ++j) {
	for (int i = 0; i < width; ++i) {
	size_t index = j*width + i;
	image[4 * index + 0] = 0xFF * (j / 10 % 2)*(i / 10 % 2); // R
	image[4 * index + 1] = 0xFF * (j / 13 % 2)*(i / 13 % 2); // G
	image[4 * index + 2] = 0xFF * (j / 17 % 2)*(i / 17 % 2); // B
	image[4 * index + 3] = 0xFF; // A
	}
	}

	glGenTextures(1, &raymarch_texture);
	glBindTexture(GL_TEXTURE_2D, raymarch_texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	// set texture content
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE,image);
	glBindTexture(GL_TEXTURE_2D, 0);
}

void draw_raymarch(float time, int program, int xres, int yres){
	glBindProgramPipeline(raymarch_shader.pid);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, raymarch_texture);
	glProgramUniform1i(raymarch_shader.fsid,2, 0);

	float fparams[4] = { xres,yres, time, 0.0 };
	glProgramUniform4fv(raymarch_shader.fsid, 1, 1, fparams);
	// bind the vao
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glBindVertexArray(raymarch_vao);
	// draw
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glBindTexture(GL_TEXTURE_2D, 0);
	glBindVertexArray(0);
	glBindProgramPipeline(0);
	glDisable(GL_BLEND);
}