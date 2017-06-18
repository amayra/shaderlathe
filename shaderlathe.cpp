#include "3rdparty/pez.h"
#include "3rdparty/gl3w.h"
#include <stdlib.h>
#include <string.h>

struct shader_id
{
	int fsid;
	int vsid;
	unsigned int pid;
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
	return shad;
}

#include "raymarch.h"

static float sceneTime = 0;

void PezHandleMouse(int x, int y, int action) { }

 void PezUpdate(unsigned int elapsedMilliseconds) { sceneTime += elapsedMilliseconds/1000.; }

void PezRender()
{
	draw_raymarch(sceneTime, 0, 1280, 720);
}

const char* PezInitialize(int width, int height)
{
	init_raymarch();
    return "Shader Lathe v0.0";
}