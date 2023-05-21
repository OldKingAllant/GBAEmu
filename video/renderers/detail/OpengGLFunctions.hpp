#pragma once

#include <Windows.h>
#include <GL/glew.h>
#include <gl/GL.h>

#include "../../../shared_obj/SharedObject.hpp"

#include <fstream>
#include <filesystem>
#include <sstream>
#include <vector>

#include "../../../common/Logger.hpp"

#define GL_CLAMP_TO_EDGE 0x812F

namespace GBA::video::renderer {

	struct OpenglFunctions {
		typedef void (APIENTRY* GLCLEAR)(GLbitfield mask);
		typedef void (APIENTRY* GLCLEARCOLOR)(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha);
		typedef void (APIENTRY* GLGENTEXTURES)(GLsizei n, GLuint* textures);
		typedef void (APIENTRY* GLDELTEXTURES)(GLsizei n, const GLuint* textures);
		typedef void (APIENTRY* GLBINDTEXTURE)(GLenum target, GLuint texture);
		typedef void (APIENTRY* GLTEXIMAGE2D)(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid* pixels);
		typedef void (APIENTRY* GLTEXPARAMI)(GLenum target, GLenum pname, GLint param);
		typedef GLenum(APIENTRY* GLGETERROR)(void);
		typedef void (APIENTRY* GLBEGIN)(GLenum mode);
		typedef void (APIENTRY* GLEND)(void);
		typedef void (APIENTRY* GLVERTEX2I)(GLshort x, GLshort y);
		typedef void (APIENTRY* GLVIEWPORT)(GLint x, GLint y, GLsizei width, GLsizei height);
		typedef void (APIENTRY* GLMATRIXMODE)(GLenum mode);
		typedef void (APIENTRY* GLIDENT)(void);
		typedef void (APIENTRY* GLUORTHO2D)(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top);
		typedef void (APIENTRY* GLTEXCOORD2F)(GLfloat s, GLfloat t);
		typedef void (APIENTRY* GLVERTEX2F)(GLfloat x, GLfloat y);
		typedef void (APIENTRY* GLENABLE)(GLenum cap);
		typedef void (APIENTRY* GLPIXELSTOREI)(GLenum pname, GLint param);
		
		typedef GLuint(APIENTRY* GLCREATESHADER)(GLenum shaderType);
		typedef void (APIENTRY* GLSHADERSOURCE)(GLuint shader, GLsizei count, const char** string, const GLint* length);
		typedef GLuint(APIENTRY* GLCREATEPROGRAM)(void);
		typedef void(APIENTRY* GLATTACHSHADER)(GLuint program, GLuint shader);
		typedef void(APIENTRY* GLLINKPROGRAM)(GLuint program);
		typedef void(APIENTRY* GLGETPROGRAMINFOLOG)(GLuint program, GLsizei maxLen, GLsizei* length, char* infoLog);
		typedef void(APIENTRY* GLUSEPROGRAM)(GLuint program);
		typedef void(APIENTRY* GLCOMPILESHADER)(GLuint shader);
		typedef void(APIENTRY* GLGETSHADERINFOLOG)(GLuint shader, GLsizei maxLen, GLsizei* length, char* infoLog);
		typedef void(APIENTRY* GLGENBUFFERS)(GLsizei n, GLuint* buffers);
		typedef void(APIENTRY* GLBUFFERDATA)(GLuint buffer, uint64_t size, const void* data, GLenum usage);
		typedef void(APIENTRY* GLBINDBUFFER)(GLenum target, GLuint buffer);
		typedef void(APIENTRY* GLACTIVETEXTURE)(GLenum texture);
		typedef void(APIENTRY* GLDRAWARRAYS)(GLenum mode, GLint first, GLsizei count);
		typedef void(APIENTRY* GLVERTEXATTRIBPOINTER)(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void* pointer);
		typedef void(APIENTRY* GLGENVERTEXARRAYS)(GLsizei n, GLuint* arrays);;
		typedef void(APIENTRY* GLBINDVERTEXARRAY)(GLuint array);
		typedef void(APIENTRY* GLENABLEVERTEXATTRIBARRAY)(GLuint index);
		typedef GLint(APIENTRY* GLGETUNIFORMLOCATION)(GLuint program, const char* name);
		typedef void(APIENTRY* GLUNIFORM1I)(GLint location, GLint v0);

		GLCLEAR glClear;
		GLCLEARCOLOR glClearColor;
		GLGENTEXTURES glGenTextures;
		GLDELTEXTURES glDeleteTextures;
		GLBINDTEXTURE glBindTexture;
		GLTEXIMAGE2D glTexImage2D;
		GLTEXPARAMI glTexParameteri;
		GLGETERROR glGetError;
		GLBEGIN glBegin;
		GLEND glEnd;
		GLVERTEX2I glVertex2i;
		GLVIEWPORT glViewport;
		GLMATRIXMODE glMatrixMode;
		GLIDENT glLoadIdentity;
		GLUORTHO2D gluOrtho2D;
		GLTEXCOORD2F glTexCoord2f;
		GLVERTEX2F glVertex2f;
		GLENABLE glEnable;
		GLPIXELSTOREI glPixelStorei;


		GLCREATESHADER glCreateShader;
		GLSHADERSOURCE glShaderSource;
		GLCOMPILESHADER glCompileShader;
		GLGETSHADERINFOLOG glGetShaderInfoLog;
		GLCREATEPROGRAM glCreateProgram;
		GLATTACHSHADER glAttachShader;
		GLLINKPROGRAM glLinkProgram;
		GLGETPROGRAMINFOLOG glGetProgramInfoLog;
		GLGETUNIFORMLOCATION glGetUniformLocation;
		GLUSEPROGRAM glUseProgram;
		GLGENBUFFERS glGenBuffers;
		GLBUFFERDATA glBufferData;
		GLBINDBUFFER glBindBuffer;
		GLACTIVETEXTURE glActiveTexture;
		GLUNIFORM1I glUniform1i;
		GLDRAWARRAYS glDrawArrays;
		GLVERTEXATTRIBPOINTER glVertexAttribPointer;
		GLGENVERTEXARRAYS glGenVertexArrays;
		GLBINDVERTEXARRAY glBindVertexArray;
		GLENABLEVERTEXATTRIBARRAY glEnableVertexAttribArray;

	};

	namespace shared_obj = GBA::shared_object;

	OpenglFunctions* LoadOpenglFunctions(shared_obj::SharedObject* library) {
		OpenglFunctions* functions = new OpenglFunctions{};

		functions->glClear = (OpenglFunctions::GLCLEAR)library->GetProcAddress("glClear");
		functions->glClearColor = (OpenglFunctions::GLCLEARCOLOR)library->GetProcAddress("glClearColor");
		functions->glGenTextures = (OpenglFunctions::GLGENTEXTURES)library->GetProcAddress("glGenTextures");
		functions->glDeleteTextures = (OpenglFunctions::GLDELTEXTURES)library->GetProcAddress("glDeleteTextures");
		functions->glBindTexture = (OpenglFunctions::GLBINDTEXTURE)library->GetProcAddress("glBindTexture");
		functions->glTexImage2D = (OpenglFunctions::GLTEXIMAGE2D)library->GetProcAddress("glTexImage2D");
		functions->glTexParameteri = (OpenglFunctions::GLTEXPARAMI)library->GetProcAddress("glTexParameteri");
		functions->glGetError = (OpenglFunctions::GLGETERROR)library->GetProcAddress("glGetError");
		functions->glBegin = (OpenglFunctions::GLBEGIN)library->GetProcAddress("glBegin");
		functions->glEnd = (OpenglFunctions::GLEND)library->GetProcAddress("glEnd");
		functions->glVertex2i = (OpenglFunctions::GLVERTEX2I)library->GetProcAddress("glVertex2i");
		functions->glViewport = (OpenglFunctions::GLVIEWPORT)library->GetProcAddress("glViewport");
		functions->glMatrixMode = (OpenglFunctions::GLMATRIXMODE)library->GetProcAddress("glMatrixMode");
		functions->glLoadIdentity = (OpenglFunctions::GLIDENT)library->GetProcAddress("glLoadIdentity");
		functions->glTexCoord2f = (OpenglFunctions::GLTEXCOORD2F)library->GetProcAddress("glTexCoord2f");
		functions->glVertex2f = (OpenglFunctions::GLVERTEX2F)library->GetProcAddress("glVertex2f");
		functions->glEnable = (OpenglFunctions::GLENABLE)library->GetProcAddress("glEnable");
		functions->glPixelStorei = (OpenglFunctions::GLPIXELSTOREI)library->GetProcAddress("glPixelStorei");

		functions->glCreateShader = (OpenglFunctions::GLCREATESHADER)library->GetProcAddress("glCreateShader");
		functions->glShaderSource = (OpenglFunctions::GLSHADERSOURCE)library->GetProcAddress("glShaderSource");
		functions->glCompileShader = (OpenglFunctions::GLCOMPILESHADER)library->GetProcAddress("glCompileShader");
		functions->glGetShaderInfoLog = (OpenglFunctions::GLGETSHADERINFOLOG)library->GetProcAddress("glGetShaderInfoLog");
		functions->glCreateProgram = (OpenglFunctions::GLCREATEPROGRAM)library->GetProcAddress("glCreateProgram");
		functions->glAttachShader = (OpenglFunctions::GLATTACHSHADER)library->GetProcAddress("glAttachShader");
		functions->glLinkProgram = (OpenglFunctions::GLLINKPROGRAM)library->GetProcAddress("glLinkProgram");
		functions->glGetProgramInfoLog = (OpenglFunctions::GLGETPROGRAMINFOLOG)library->GetProcAddress("glGetProgramInfoLog");
		functions->glGetUniformLocation = (OpenglFunctions::GLGETUNIFORMLOCATION)library->GetProcAddress("glGetUniformLocation");
		functions->glUseProgram = (OpenglFunctions::GLUSEPROGRAM)library->GetProcAddress("glUseProgram");
		functions->glGenBuffers = (OpenglFunctions::GLGENBUFFERS)library->GetProcAddress("glGenBuffers");
		functions->glBufferData = (OpenglFunctions::GLBUFFERDATA)library->GetProcAddress("glBufferData");
		functions->glBindBuffer = (OpenglFunctions::GLBINDBUFFER)library->GetProcAddress("glBindBuffer");
		functions->glActiveTexture = (OpenglFunctions::GLACTIVETEXTURE)library->GetProcAddress("glActiveTexture");
		functions->glUniform1i = (OpenglFunctions::GLUNIFORM1I)library->GetProcAddress("glUniform1i");
		functions->glGenVertexArrays = (OpenglFunctions::GLGENVERTEXARRAYS)library->GetProcAddress("glGenVertexArrays");
		functions->glBindVertexArray = (OpenglFunctions::GLBINDVERTEXARRAY)library->GetProcAddress("glBindVertexArray");
		functions->glVertexAttribPointer = (OpenglFunctions::GLVERTEXATTRIBPOINTER)library->GetProcAddress("glVertexAttribPointer");
		functions->glEnableVertexAttribArray = (OpenglFunctions::GLENABLEVERTEXATTRIBARRAY)library->GetProcAddress("glEnableVertexAttribArray");
		functions->glDrawArrays = (OpenglFunctions::GLDRAWARRAYS)library->GetProcAddress("glDrawArrays");

		return functions;
	}

	std::string LoadShaderSource(std::filesystem::path const& path) {
		if (!std::filesystem::exists(path))
			return "FILE NOT FOUND";

		std::ostringstream shader_source{ "" };

		std::ifstream shader(path, std::ios::in | std::ios::beg);

		while (!shader.eof()) {
			std::string line = "";

			std::getline(shader, line);

			if (!line.empty())
				shader_source << line << "\n";
		}

		return shader_source.str();
	}

	std::pair<GLuint, std::string> CreateShader(std::string const& path, GLenum type, OpenglFunctions* functions) {
		GLenum shader_id = functions->glCreateShader(type);

		std::string source = LoadShaderSource(path);

		GLint len = (GLint)source.length();

		const char* source_ptr = source.c_str();

		functions->glShaderSource(shader_id, 1, &source_ptr, &len);
		functions->glCompileShader(shader_id);

		GLint status = 0;

		glGetShaderiv(shader_id, GL_COMPILE_STATUS, &status);

		char info_log[512] = {};
		GLsizei info_len = 0;

		if (status == GL_FALSE) {
			functions->glGetShaderInfoLog(shader_id, 512, &info_len, info_log);
		}

		return std::pair{ shader_id, std::string(info_log, info_len) };
	}

	std::pair<GLuint, std::string> CreateProgram(std::vector<std::pair<std::string, GLenum>> const& shaders, OpenglFunctions* functions) {
		GLuint program_id = functions->glCreateProgram();

		for (auto const& shader : shaders) {
			auto shader_result = CreateShader(shader.first, shader.second, functions);

			logging::Logger::Instance().LogInfo("OpenGL Renderer", "Shader log for shader {0} : {1}", shader.first, shader_result.second);

			functions->glAttachShader(program_id, shader_result.first);
		}

		functions->glLinkProgram(program_id);

		GLint status = 0;

		glGetProgramiv(program_id, GL_LINK_STATUS, &status);
		glValidateProgram(program_id);

		char info_log[512] = {};
		GLsizei info_len = 0;

		functions->glGetProgramInfoLog(program_id, 512, &info_len, info_log);

		return std::pair{ program_id, std::string(info_log, info_len) };
	}
}