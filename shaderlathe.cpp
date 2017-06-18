#include "3rdparty/pez.h"
#include "3rdparty/gl3w.h"
#define DR_FSW_IMPLEMENTATION
#include "3rdparty/dr_fsw.h"
#define DR_IMPLEMENTATION
#include "3rdparty/dr.h"
#include <stdlib.h>
#include <string.h>

struct shader_id
{
	int fsid;
	int vsid;
	unsigned int pid;
	bool compiled;
};

struct FBOELEM {
	GLuint fbo;
	GLuint depthbuffer;
	GLuint texture;
	GLuint depthtexture;
	GLint status;
};

shader_id initShader(const char *vsh, const char *fsh)
{
	shader_id shad = { 0 };
	shad.compiled = false;
	shad.vsid = glCreateShaderProgramv(GL_VERTEX_SHADER, 1, &vsh);
	shad.fsid = glCreateShaderProgramv(GL_FRAGMENT_SHADER, 1, &fsh);
	glGenProgramPipelines(1, &shad.pid);
	glBindProgramPipeline(shad.pid);
	glUseProgramStages(shad.pid, GL_VERTEX_SHADER_BIT, shad.vsid);
	glUseProgramStages(shad.pid, GL_FRAGMENT_SHADER_BIT, shad.fsid);
#ifdef DEBUG
	int		result;
	char    info[1536];
	glGetProgramiv(shad.vsid, GL_LINK_STATUS, &result); glGetProgramInfoLog(shad.vsid, 1024, NULL, (char *)info); if (!result) DebugBreak();
	glGetProgramiv(shad.fsid, GL_LINK_STATUS, &result); glGetProgramInfoLog(shad.fsid, 1024, NULL, (char *)info); if (!result) DebugBreak();
	glGetProgramiv(shad.pid, GL_LINK_STATUS, &result); glGetProgramInfoLog(shad.pid, 1024, NULL, (char *)info); if (!result) DebugBreak();
#endif
	glBindProgramPipeline(0);
	shad.compiled = true;
	return shad;
}

shader_id raymarch_shader;

#include "raymarch.h"

static float sceneTime = 0;
drfsw_context* context = NULL;

void PezHandleMouse(int x, int y, int action) { }

 void PezUpdate(unsigned int elapsedMilliseconds) { sceneTime += elapsedMilliseconds/1000.; }

 char* getFileNameFromPath(char* path)
 {
	 for (size_t i = strlen(path) - 1; i; i--)
	 {
		 if (path[i] == '/')
		 {
			 return &path[i + 1];
		 }
	 }
	 return path;
 }

 void recompile_shader(char* path)
 {
	 if (strcmp(getFileNameFromPath(path), "raymarch.glsl") == 0)
	 {
		 raymarch_shader.compiled = false;
		 glDeleteProgramPipelines(1, &raymarch_shader.pid);
		 size_t sizeout;
		 char* pix_shader = dr_open_and_read_text_file(path, &sizeout);
		 raymarch_shader = initShader(vertex_source, (const char*)pix_shader);
		 free(pix_shader);
	 }
 }

void PezRender()
{
	drfsw_event e;
	if (drfsw_peek_event(context, &e))
	{
		switch (e.type)
		{
		case drfsw_event_type_updated: recompile_shader(e.absolutePath); break;
		default: break;
		}
	}
	if (raymarch_shader.compiled)
	draw_raymarch(sceneTime, raymarch_shader, 1280, 720);
}

const char* PezInitialize(int width, int height)
{
	context = drfsw_create_context();
	TCHAR path[512] = { 0 };
	dr_get_executable_directory_path(path, sizeof(path));
	drfsw_add_directory(context, path);
	size_t sizeout;
	char* pix_shader = dr_open_and_read_text_file("raymarch.glsl", &sizeout);
	raymarch_shader = initShader(vertex_source, (const char*)pix_shader);
	free(pix_shader);
	init_raymarch();
    return "Shader Lathe v0.0";
}