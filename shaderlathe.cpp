#include "3rdparty/pez.h"
#include "3rdparty/gl3w.h"
#define DR_FSW_IMPLEMENTATION
#include "3rdparty/dr_fsw.h"
#define DR_IMPLEMENTATION
#include "3rdparty/dr.h"
#include "3rdparty/nuklear.h"
#include "3rdparty/nuklear_pez_gl3.h"
#include "3rdparty/bass.h"
#pragma comment (lib,"bass.lib")
#include <stdlib.h>
#include <string.h>
#include <mmsystem.h>
#define GB_MATH_IMPLEMENTATION
#include "3rdparty/gb_math.h"
#include "3rdparty/rocket/sync.h"
#include<iostream>
#include<vector>
#include<cstring>
#include<fstream>
#include <string>
#include <sstream>  
#include <Commdlg.h>
#include <windows.h>

using namespace std;
#define MAX_VERTEX_BUFFER 512 * 1024
#define MAX_ELEMENT_BUFFER 128 * 1024

struct glsl2configmap
{
	char name[100];
	int frag_number;
	int program_num;
	float val;
	float min;
	float max;
	float inc;
	bool ispost;
};
std::vector<glsl2configmap>shaderconfig_map;

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

static struct sync_device *device = NULL;
#if !defined(SYNC_PLAYER)
static struct sync_cb cb;
#endif
int rocket_connected = 0;
HSTREAM music_stream = NULL;
float rps = 5.0f;
int audio_is_playing = 1;
shader_id raymarch_shader = { 0 };
shader_id post_shader = { 0 };
GLuint post_texture = 0;
static float sceneTime = 0;
drfsw_context* context = NULL;
GLuint scene_vao, scene_texture;
bool paused = false;
bool seek = false;

#define GLSL(src) #src
const char vertex_source[] =
"#version 430\n"
"out gl_PerVertex{vec4 gl_Position;};"
"out vec2 ftexcoord;"
"void main()"
"{"
"	float x = -1.0 + float((gl_VertexID & 1) << 2);"
"	float y = -1.0 + float((gl_VertexID & 2) << 1);"
"	ftexcoord.x = (x + 1.0)*0.5;"
"	ftexcoord.y = (y + 1.0)*0.5;"
"	gl_Position = vec4(x, y, 0, 1);"
"}";

int row_to_ms_round(int row, float rps)
{
	const float newtime = ((float)(row)) / rps;
	return (int)(floor(newtime + 0.5f));
}

float ms_to_row_f(float time_ms, float rps)
{
	const float row = rps * time_ms;
	return row;
}

int ms_to_row_round(int time_ms, float rps)
{
	const float r = ms_to_row_f(time_ms, rps);
	return (int)(floor(r + 0.5f));
}

#if !defined(SYNC_PLAYER)
static void xpause(void* data, int flag)
{
	(void)data;
	if (flag)
	{
		BASS_ChannelPause(music_stream);
		audio_is_playing = 0;
	}
		
	else
	{
		audio_is_playing = 1;
		BASS_ChannelPlay(music_stream, false);
	}
}

static void xset_row(void* data, int row)
{
	int newtime_ms = row_to_ms_round(row, rps);
	sceneTime = newtime_ms;
	if (BASS_ChannelIsActive(music_stream) != BASS_ACTIVE_STOPPED)
	BASS_ChannelSetPosition(music_stream,BASS_ChannelSeconds2Bytes(music_stream, sceneTime),BASS_POS_BYTE);
	(void)data;
}

static int xis_playing(void* data)
{
	(void)data;
	return audio_is_playing;
}
#endif //!SYNC_PLAYER

int rocket_init(const char* prefix)
{
	device = sync_create_device(prefix);
	if (!device)
	{
		printf("Unable to create rocketDevice\n");
		return 0;
	}
#if !defined( SYNC_PLAYER )
	cb.is_playing = xis_playing;
	cb.pause = xpause;
	cb.set_row = xset_row;

	if (sync_connect(device, "localhost", SYNC_DEFAULT_PORT))
	{
		printf("Rocket failed to connect\n");
		return 0;
	}
#endif
	printf("Rocket connected.\n");
	return 1;
}

void update_rocket()
{
	if (rocket_connected)
	{
		float row_f = ms_to_row_f(sceneTime, rps);
		if (sync_update(device, (int)floor(row_f), &cb, 0))rocket_connected = sync_connect(device, "localhost", SYNC_DEFAULT_PORT);
		if (rocket_connected)
		{
			for (int i = 0; i < shaderconfig_map.size(); i++)
			{
				if (strstr(shaderconfig_map[i].name, "_rkt") != NULL)
				{
					string shit = shaderconfig_map[i].name;
					shit = shit.substr(0, shit.size() - 4);
					const sync_track *track = sync_get_track(device, shit.c_str());
					shaderconfig_map[i].val = sync_get_val(track, row_f);
				}
			}
		}

	}
}

void glsl_to_config(shader_id prog, char *shader_path,bool ispostproc)
{
		vector<string>lines;
		lines.clear();
		ifstream openFile(shader_path);
		string stringToStore; //string to store file line
		while (getline(openFile, stringToStore)) { //checking for existence of file
			lines.push_back(stringToStore);
		}
		openFile.close(); //closes file after done
		//convert GLSL uniforms to variables
		int total = -1;
		glGetProgramiv(prog.fsid, GL_ACTIVE_UNIFORMS, &total);
		for (int i = 0; i < total; ++i) {
			int name_len = -1, num = -1;
			GLenum type = GL_ZERO;
			char name[100] = { 0 };
			glGetActiveUniform(prog.fsid, GLuint(i), sizeof(name) - 1,
			&name_len, &num, &type, name);
			name[name_len] = 0;
			if (type == GL_FLOAT) {
				for (int j= 0; j < lines.size(); j++)
				{
					string shit = "uniform float ";shit += name;
					if (strstr(lines[j].c_str(),name))
					{
						//Nuklear (user controllable)
						std::size_t found = lines[j].rfind("//");
						if (found != std::string::npos)
						{
							string shit2 = lines[j].substr(found + 2, lines[j].length());
							stringstream parse1(shit2);
							parse1.precision(6);
							double min = 0., max = 0., inc = 0.00;
							parse1 >> fixed >> min;
							parse1 >> fixed >> max;
							parse1 >> fixed >> inc;
							glsl2configmap subObj = { 0 };
							strcpy(subObj.name, name);
							subObj.frag_number = prog.fsid;
							subObj.program_num = prog.pid;
							subObj.inc = inc;
							subObj.min = min;
							subObj.max = max;
							subObj.ispost = ispostproc;
							shaderconfig_map.push_back(subObj);
						}
						//GNU Rocket (user scriptable)
						else if(lines[j].rfind("_rkt") != std::string::npos && rocket_connected)
						{
								glsl2configmap subObj = { 0 };
								strcpy(subObj.name, name);
								subObj.frag_number = prog.fsid;
								subObj.program_num = prog.pid;
								float val = 0.0;
								subObj.inc = val;
								subObj.min = subObj.max = subObj.inc;
								subObj.ispost = ispostproc;
								shaderconfig_map.push_back(subObj);
						}
					}

				}
			}
		}
}

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

GLuint init_rendertexture(int resx, int resy)
{
	GLuint texture;
	//texture
	glEnable(GL_TEXTURE_2D);
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, resx, resy, 0, GL_RGBA,GL_UNSIGNED_BYTE, 0);
	glBindTexture(GL_TEXTURE_2D, 0);
	return texture;
}

void draw(float time, shader_id program, int xres, int yres, GLuint texture){
	glBindProgramPipeline(program.pid);
	glViewport(0, 0, xres, yres);
	if (texture)
	{
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, texture);
		glProgramUniform1i(program.fsid, 2, 0);
	}
	float fparams[4] = { xres, yres, time, 0.0 };
	glProgramUniform4fv(program.fsid, 1, 1, fparams);
	for (int i = 0; i < shaderconfig_map.size(); i++)
	{
		if (shaderconfig_map[i].program_num = program.pid)
		{
			int uniform_loc = glGetUniformLocation(program.fsid, shaderconfig_map[i].name);
			glProgramUniform1f(program.fsid, uniform_loc, shaderconfig_map[i].val);
		}	
	}
	// bind the vao
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glBindVertexArray(scene_vao);
	// draw
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glBindVertexArray(0);
	glBindProgramPipeline(0);
	glDisable(GL_BLEND);
}

void PezHandleMouse(int x, int y, int action) { }

void PezUpdate(unsigned int elapsedMilliseconds) {
	if (BASS_ChannelIsActive(music_stream) != BASS_ACTIVE_STOPPED)
	{
		if (audio_is_playing)
		{
			QWORD len = BASS_ChannelGetPosition(music_stream, BASS_POS_BYTE); // the length in bytes
			sceneTime = BASS_ChannelBytes2Seconds(music_stream, len);
		}
	}
	else
	{
		if (rocket_connected)
		{
			if (audio_is_playing)sceneTime += elapsedMilliseconds * 0.001;
			return;
		}
		if(!paused)sceneTime += elapsedMilliseconds * 0.001;
	}
}

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
		static unsigned long last_shaderload = 0;
		unsigned long load = timeGetTime();
		 if (load-last_shaderload > 200) { //take into account actual shader recompile time
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
		 last_shaderload = timeGetTime();
	 }
	 if (strcmp(getFileNameFromPath(path), "post.glsl") == 0)
	 {
		 static unsigned long last_shaderload = 0;
		 unsigned long load = timeGetTime();
		 if (load - last_shaderload > 200) { //take into account actual shader recompile time
			 Sleep(100);
			 if (glIsProgramPipeline(post_shader.pid)) {
				 glDeleteProgram(post_shader.fsid);
				 glDeleteProgram(post_shader.vsid);
				 glBindProgramPipeline(0);
				 glDeleteProgramPipelines(1, &post_shader.pid);
			 }
			 post_shader = { 0 };
			 post_shader.compiled = false;
			 size_t sizeout = 0;
			 char* pix_shader = dr_open_and_read_text_file(path, &sizeout);
			 if (pix_shader) {
				 post_shader = initShader(post_shader, vertex_source, (const char*)pix_shader);
				 dr_free_file_data(pix_shader);
			 }
		 }
		 last_shaderload = timeGetTime();
	 }
	shaderconfig_map.clear();
	if(raymarch_shader.compiled)glsl_to_config(raymarch_shader, "raymarch.glsl",false);
	if (post_shader.compiled) glsl_to_config(post_shader, "post.glsl",true);
 }

char *get_file(void) {
	OPENFILENAME    ofn;
	char     filename[4096] = { 0 };
	static const char   filter[] =
		"(*.*)\0"       "*.*\0"
		"\0"            "\0";
	filename[0] = 0;
	memset(&ofn, 0, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.lpstrFilter = filter;
	ofn.nFilterIndex = 1;
	ofn.lpstrFile = filename;
	ofn.nMaxFile = sizeof(filename);
	ofn.lpstrTitle = "Select the input audio file";
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_LONGNAMES | OFN_EXPLORER | OFN_HIDEREADONLY;

	printf("- %s\n", ofn.lpstrTitle);
	if (!GetOpenFileName(&ofn)) return NULL;
	return(filename);
}

void gui()
{
	static double time=0;
	if (ctx)
	{
		if (nk_begin(ctx, "Timeline", nk_rect(30, 520, 530, 160),
			NK_WINDOW_BORDER | NK_WINDOW_MOVABLE |
			NK_WINDOW_MINIMIZABLE | NK_WINDOW_TITLE))
		{
			nk_layout_row_static(ctx, 30, 100, 4);
			if (nk_button_label(ctx, "Load/Unload"))
			{
				char *file = get_file();
				if (file)
				{
					if (BASS_ChannelIsActive(music_stream) != BASS_ACTIVE_STOPPED)	BASS_StreamFree(music_stream);
					if (music_stream = BASS_StreamCreateFile(FALSE, file, 0, 0,0))
					{
						QWORD len = BASS_ChannelGetLength(music_stream, BASS_POS_BYTE); // the length in bytes
						time = BASS_ChannelBytes2Seconds(music_stream, len);
						BASS_ChannelPlay(music_stream, TRUE);
						sceneTime = 0;
					}
				}
				else
				{
					if (BASS_ChannelIsActive(music_stream) != BASS_ACTIVE_STOPPED)BASS_StreamFree(music_stream);
				}
			}
			char *label1 = paused ? "Resume" : "Pause";
			if (nk_button_label(ctx, label1))
			{
				if (BASS_ChannelIsActive(music_stream) == BASS_ACTIVE_PLAYING)
				{
					BASS_ChannelPause(music_stream);
					paused = !paused;
				}
				else
				{
					paused = !paused;
					BASS_ChannelPlay(music_stream,FALSE);
				}
			}
			if (nk_button_label(ctx, "Rewind"))
			{
				BASS_ChannelSetPosition(music_stream,0,BASS_POS_INEXACT);
				sceneTime = 0;
			}
			if (BASS_ChannelIsActive(music_stream) != BASS_ACTIVE_STOPPED)
			{
				float max = time;
				nk_layout_row_dynamic(ctx, 25, 1);
				char label1[100] = { 0 };
				sprintf(label1, "Progress: %.2f / %.2f seconds", sceneTime,max);
				nk_label(ctx, label1, NK_TEXT_LEFT);
				nk_layout_row_static(ctx, 30, 500, 2);

				seek = nk_slider_float(ctx, 0, (float*)&sceneTime, max, 0.1);
				if (seek)
				{
					BASS_ChannelSetPosition(
						music_stream,
						BASS_ChannelSeconds2Bytes(music_stream, sceneTime),
						BASS_POS_BYTE
					);
				}
			}
			else
			{
				int max = 300;
				nk_layout_row_dynamic(ctx, 25, 1);
				char label1[100] = { 0 };
				sprintf(label1, "Progress: %.2f seconds", sceneTime);
				nk_label(ctx, label1, NK_TEXT_LEFT);
				nk_layout_row_static(ctx, 30, 500, 2);
				seek = nk_slider_float(ctx, 0, (float*)&sceneTime, max, 0.1);
			}
		}
		nk_end(ctx);
		if (nk_begin(ctx, "Raymarch Uniforms", nk_rect(900, 30, 300, 200),
			NK_WINDOW_BORDER | NK_WINDOW_MOVABLE |
			NK_WINDOW_MINIMIZABLE | NK_WINDOW_TITLE))
		{
			for (int i = 0; i < shaderconfig_map.size(); i++) {
				if (strstr(shaderconfig_map[i].name, "_rkt") == NULL  && !shaderconfig_map[i].ispost) {
					nk_layout_row_dynamic(ctx, 25, 1);
					char label1[100] = { 0 };
					sprintf(label1, "%s: %.2f", shaderconfig_map[i].name, shaderconfig_map[i].val);
					nk_label(ctx, label1, NK_TEXT_LEFT);
					nk_layout_row_static(ctx, 30, 250, 2);
					nk_slider_float(ctx, shaderconfig_map[i].min, &shaderconfig_map[i].val, shaderconfig_map[i].max, shaderconfig_map[i].inc);
				}
			}
		}
		nk_end(ctx);
		if (nk_begin(ctx, "Post-Process Uniforms", nk_rect(900, 400, 300, 200),
			NK_WINDOW_BORDER | NK_WINDOW_MOVABLE |
			NK_WINDOW_MINIMIZABLE | NK_WINDOW_TITLE))
		{
			for (int i = 0; i < shaderconfig_map.size(); i++) {
				if (strstr(shaderconfig_map[i].name, "_rkt") == NULL && shaderconfig_map[i].ispost) {
					nk_layout_row_dynamic(ctx, 25, 1);
					char label1[100] = { 0 };
					sprintf(label1, "%s: %.2f", shaderconfig_map[i].name, shaderconfig_map[i].val);
					nk_label(ctx, label1, NK_TEXT_LEFT);
					nk_layout_row_static(ctx, 30, 250, 2);
					nk_slider_float(ctx, shaderconfig_map[i].min, &shaderconfig_map[i].val, shaderconfig_map[i].max, shaderconfig_map[i].inc);
				}
			}
		}
	   nk_end(ctx);
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
	glClear(GL_COLOR_BUFFER_BIT);
	glClearColor(0, 0, 0, 0.0);
	
	if (seek)sceneTime = floor(sceneTime);
	
	update_rocket();

	if (raymarch_shader.compiled) draw(sceneTime, raymarch_shader,PEZ_VIEWPORT_WIDTH,PEZ_VIEWPORT_HEIGHT,NULL);
	if (post_shader.compiled)
	{
		glBindTexture(GL_TEXTURE_2D, post_texture);
		glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, PEZ_VIEWPORT_WIDTH, PEZ_VIEWPORT_HEIGHT);
		glBindTexture(GL_TEXTURE_2D, 0);
		glClear(GL_COLOR_BUFFER_BIT);
		glClearColor(0, 0, 0, 0.0);
		draw(sceneTime, post_shader, PEZ_VIEWPORT_WIDTH, PEZ_VIEWPORT_HEIGHT, post_texture);
	}
	gui();
	nk_pez_render(NK_ANTI_ALIASING_ON, MAX_VERTEX_BUFFER, MAX_ELEMENT_BUFFER);
}

const char* PezInitialize(int width, int height)
{
	shaderconfig_map.clear();
	BASS_Init(-1, 44100, 0, NULL, NULL);
	glGenVertexArrays(1, &scene_vao);
	post_texture = init_rendertexture(PEZ_VIEWPORT_WIDTH, PEZ_VIEWPORT_HEIGHT);
	rocket_connected = rocket_init("rocket");
	context = drfsw_create_context();
	TCHAR path[512] = { 0 };
	dr_get_executable_directory_path(path, sizeof(path));
	dr_set_current_directory(path);
	drfsw_add_directory(context, path);
	recompile_shader("raymarch.glsl");
	recompile_shader("post.glsl");
    return "Shader Lathe v0.1";
}