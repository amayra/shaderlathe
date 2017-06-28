GLuint scene_vao, scene_texture;

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

void init_raymarch()
{
	
	// get texture uniform location


	// vao and vbo handle
	GLuint vbo;

	// generate and bind the vao
	glGenVertexArrays(1, &scene_vao);
	glBindVertexArray(scene_vao);

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

	glGenTextures(1, &scene_texture);
	glBindTexture(GL_TEXTURE_2D, scene_texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	// set texture content
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE,image);
	glBindTexture(GL_TEXTURE_2D, 0);
}

void draw_raymarch(float time, shader_id program, int xres, int yres){
	glBindProgramPipeline(program.pid);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, scene_texture);
	glProgramUniform1i(program.fsid,2, 0);
	float fparams[4] = { xres,yres, time, 0.0 };
	glProgramUniform4fv(program.fsid, 1, 1, fparams);
	for (int i = 0; i < rocket_map.size(); i++)
	{
		if (program.pid == rocket_map[i].prog_number)
		{
			int uniform_loc = glGetUniformLocation(program.pid, rocket_map[i].name.c_str());
			glUniform1f(uniform_loc, rocket_map[i].val);
		}
	}
	// bind the vao
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glBindVertexArray(scene_vao);
	// draw
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glBindTexture(GL_TEXTURE_2D, 0);
	glBindVertexArray(0);
	glBindProgramPipeline(0);
	glDisable(GL_BLEND);
}