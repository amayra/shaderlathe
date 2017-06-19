#include "3rdparty/pez.h"
#include "3rdparty/gl3w.h"
#define DR_FSW_IMPLEMENTATION
#include "3rdparty/dr_fsw.h"
#define DR_IMPLEMENTATION
#include "3rdparty/dr.h"
#include "3rdparty/nuklear.h"
#include "3rdparty/nuklear_pez_gl3.h"
#include <stdlib.h>
#include <string.h>

#include <mmsystem.h>

#define MAX_VERTEX_BUFFER 512 * 1024
#define MAX_ELEMENT_BUFFER 128 * 1024

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

shader_id initShader(shader_id shad,const char *vsh, const char *fsh)
{
	shad.compiled = true;
	shad.vsid = glCreateShaderProgramv(GL_VERTEX_SHADER, 1, &vsh);
	shad.fsid = glCreateShaderProgramv(GL_FRAGMENT_SHADER, 1, &fsh);
	glGenProgramPipelines(1, &shad.pid);
	glBindProgramPipeline(shad.pid);
	glUseProgramStages(shad.pid, GL_VERTEX_SHADER_BIT, shad.vsid);
	glUseProgramStages(shad.pid, GL_FRAGMENT_SHADER_BIT, shad.fsid);
#ifdef DEBUG
	int		result;
	char    info[1536];
	glGetProgramiv(shad.vsid, GL_LINK_STATUS, &result); glGetProgramInfoLog(shad.vsid, 1024, NULL, (char *)info); if (!result){ goto fail; }
	glGetProgramiv(shad.fsid, GL_LINK_STATUS, &result); glGetProgramInfoLog(shad.fsid, 1024, NULL, (char *)info); if (!result){ goto fail; }
	glGetProgramiv(shad.pid, GL_LINK_STATUS, &result); glGetProgramInfoLog(shad.pid, 1024, NULL, (char *)info); if (!result){ goto fail; }
#endif
	
	glBindProgramPipeline(0);
	shad.compiled = true;
	return shad;
fail:
	{
		shad.compiled = false;
		glDeleteProgram(shad.fsid);
		glDeleteProgram(shad.vsid);
		glBindProgramPipeline(0);
		glDeleteProgramPipelines(1, &shad.pid);
		return shad;
	}
}

shader_id raymarch_shader = { 0 };
static float sceneTime = 0;
drfsw_context* context = NULL;

#include "raymarch.h"

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
unsigned long last_load=0;
 void recompile_shader(char* path)
 {

	 if (strcmp(getFileNameFromPath(path), "raymarch.glsl") == 0)
	 {
		 unsigned long load = timeGetTime();
		 if (load-last_load > 200) { //take into account actual shader recompile time
			 Sleep(100);
			 if (glIsProgramPipeline(raymarch_shader.pid)) {
				 glDeleteProgram(raymarch_shader.fsid);
				 glDeleteProgram(raymarch_shader.vsid);
				 glBindProgramPipeline(0);
				 glDeleteProgramPipelines(1, &raymarch_shader.pid);
			 }
			 raymarch_shader = { 0 };
			 raymarch_shader.compiled = false;
			 size_t sizeout = 0;
			 char* pix_shader = dr_open_and_read_text_file(path, &sizeout);
			 if (pix_shader) {
				 raymarch_shader = initShader(raymarch_shader, vertex_source, (const char*)pix_shader);
				 dr_free_file_data(pix_shader);
			 }
		 }
		 last_load = timeGetTime();
	 }

	 if (strcmp(getFileNameFromPath(path), "post.glsl") == 0)
	 {

	 }
 }

struct nk_color background;


const char *playstop[] = { "Pause","Resume","Play","Stop" };
int action = 0;

void gui()
{
	if (nk_begin(ctx, "Shader Timeline", nk_rect(30, 520, 530, 160),
		NK_WINDOW_BORDER | NK_WINDOW_MOVABLE |
		NK_WINDOW_MINIMIZABLE | NK_WINDOW_TITLE))
	{
		nk_layout_row_static(ctx, 30, 80, 4);
		if (nk_button_label(ctx, "Load"))
			fprintf(stdout, "button pressed\n");
		if (nk_button_label(ctx, playstop[action]))
		{
			action++;
			if (action >= 2)action = 0;
		}
		if (nk_button_label(ctx, "Rewind"))
		{
			
		}
		static size_t prog = 0;
		int max = 100;

		nk_layout_row_dynamic(ctx, 25, 1);
		nk_label(ctx, "Playback position:", NK_TEXT_LEFT);
		nk_layout_row_static(ctx, 30, 500, 2);
		nk_progress(ctx, &prog, max, NK_MODIFIABLE);
		
	}
	nk_end(ctx);
	nk_pez_render(NK_ANTI_ALIASING_ON, MAX_VERTEX_BUFFER, MAX_ELEMENT_BUFFER);
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

	float bg[4];
	nk_color_fv(bg, background);
	glClear(GL_COLOR_BUFFER_BIT);
	glClearColor(bg[0], bg[1], bg[2], bg[3]);
	/* IMPORTANT: `nk_sfml_render` modifies some global OpenGL state
	* with blending, scissor, face culling and depth test and defaults everything
	* back into a default state. Make sure to either save and restore or
	* reset your own state after drawing rendering the UI. */
	
	if (raymarch_shader.compiled)
	{
		draw_raymarch(sceneTime, raymarch_shader, 1280, 720);
	}

	gui();
	
}

const char* PezInitialize(int width, int height)
{
	context = drfsw_create_context();
	TCHAR path[512] = { 0 };
	dr_get_executable_directory_path(path, sizeof(path));
	drfsw_add_directory(context, path);
	size_t sizeout;
	char* pix_shader = dr_open_and_read_text_file("raymarch.glsl", &sizeout);
	raymarch_shader = initShader(raymarch_shader,vertex_source, (const char*)pix_shader);
	free(pix_shader);
	init_raymarch();

	
	background = nk_rgb(28, 48, 62);

    return "Shader Lathe v0.0";
}