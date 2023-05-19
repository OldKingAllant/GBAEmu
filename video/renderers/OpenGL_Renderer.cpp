#include "OpenGL_Renderer.hpp"

#include <SDL2/SDL.h>

#include <Windows.h>
#include <gl/GL.h>

#include "../../common/Logger.hpp"

#include "../../shared_obj/ProcedureWrapper.hpp"

#include "../../memory/Keypad.hpp"

namespace GBA::video::renderer {
	LOG_CONTEXT(OpenGLRenderer);

	#define GL_CLAMP_TO_EDGE 0x812F

	struct OpenglFunctions {
		typedef void (APIENTRY * GLCLEAR)(GLbitfield mask);
		typedef void (APIENTRY * GLCLEARCOLOR)(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha);
		typedef void (APIENTRY * GLGENTEXTURES)(GLsizei n, GLuint* textures);
		typedef void (APIENTRY * GLDELTEXTURES)(GLsizei n, const GLuint* textures);
		typedef void (APIENTRY * GLBINDTEXTURE)(GLenum target, GLuint texture);
		typedef void (APIENTRY * GLTEXIMAGE2D)(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid* pixels);
		typedef void (APIENTRY * GLTEXPARAMI)(GLenum target, GLenum pname, GLint param);
		typedef GLenum (APIENTRY * GLGETERROR)(void);
		typedef void (APIENTRY * GLBEGIN)(GLenum mode);
		typedef void (APIENTRY * GLEND)(void);
		typedef void (APIENTRY * GLVERTEX2I)(GLshort x, GLshort y);
		typedef void (APIENTRY * GLVIEWPORT)(GLint x, GLint y, GLsizei width, GLsizei height);
		typedef void (APIENTRY * GLMATRIXMODE)(GLenum mode);
		typedef void (APIENTRY * GLIDENT)(void);
		typedef void (APIENTRY * GLUORTHO2D)(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top);
		typedef void (APIENTRY * GLTEXCOORD2F)(GLfloat s, GLfloat t);
		typedef void (APIENTRY * GLVERTEX2F)(GLfloat x, GLfloat y);
		typedef void (APIENTRY * GLENABLE)(GLenum cap);
		typedef void (APIENTRY * GLPIXELSTOREI)(GLenum pname, GLint param);

		//GLCLEAR glClear;
		shared_object::FunctionWrapper* glClear;
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
	};

	OpenGL::OpenGL() :
		m_window(nullptr), m_gl_context(nullptr),
		m_functions(nullptr), m_opengl(), m_glu(),
		m_gl_data{}
	{
		LoadOpengl();

		m_gl_data.placeholder_data = new float[240 * 160 * 3];

		std::fill_n(m_gl_data.placeholder_data, 240 * 160 * 3, 0.5f);
	}

	void OpenGL::CheckForErrors() {
		GLenum error = 0;

		while ((error = m_functions->glGetError()) != GL_NO_ERROR) {
			LOG_ERROR(" OpenGL error : {}", error);
		}
	}

	void OpenGL::LoadOpengl() {
		if (!m_opengl.Load("opengl32.dll")) {
			LOG_ERROR("OpenGL load failed!");
			return;
		}

		if (!m_glu.Load("Glu32.dll")) {
			LOG_ERROR("GLU32 load failed!");
			return;
		}

		m_functions = new OpenglFunctions{};

		m_functions->glClear = new shared_object::FunctionWrapperImpl<void(*)(GLbitfield mask), 
			shared_object::CallConvetion::STCALL>(m_opengl.GetProcAddress("glClear"));
		m_functions->glClearColor = (OpenglFunctions::GLCLEARCOLOR)m_opengl.GetProcAddress("glClearColor");
		m_functions->glGenTextures = (OpenglFunctions::GLGENTEXTURES)m_opengl.GetProcAddress("glGenTextures");
		m_functions->glDeleteTextures = (OpenglFunctions::GLDELTEXTURES)m_opengl.GetProcAddress("glDeleteTextures");
		m_functions->glBindTexture = (OpenglFunctions::GLBINDTEXTURE)m_opengl.GetProcAddress("glBindTexture");
		m_functions->glTexImage2D = (OpenglFunctions::GLTEXIMAGE2D)m_opengl.GetProcAddress("glTexImage2D");
		m_functions->glTexParameteri = (OpenglFunctions::GLTEXPARAMI)m_opengl.GetProcAddress("glTexParameteri");
		m_functions->glGetError = (OpenglFunctions::GLGETERROR)m_opengl.GetProcAddress("glGetError");
		m_functions->glBegin = (OpenglFunctions::GLBEGIN)m_opengl.GetProcAddress("glBegin");
		m_functions->glEnd = (OpenglFunctions::GLEND)m_opengl.GetProcAddress("glEnd");
		m_functions->glVertex2i = (OpenglFunctions::GLVERTEX2I)m_opengl.GetProcAddress("glVertex2i");
		m_functions->glViewport = (OpenglFunctions::GLVIEWPORT)m_opengl.GetProcAddress("glViewport");
		m_functions->glMatrixMode = (OpenglFunctions::GLMATRIXMODE)m_opengl.GetProcAddress("glMatrixMode");
		m_functions->glLoadIdentity = (OpenglFunctions::GLIDENT)m_opengl.GetProcAddress("glLoadIdentity");
		m_functions->gluOrtho2D = (OpenglFunctions::GLUORTHO2D)m_glu.GetProcAddress("gluOrtho2D");
		m_functions->glTexCoord2f = (OpenglFunctions::GLTEXCOORD2F)m_opengl.GetProcAddress("glTexCoord2f");
		m_functions->glVertex2f = (OpenglFunctions::GLVERTEX2F)m_opengl.GetProcAddress("glVertex2f");
		m_functions->glEnable = (OpenglFunctions::GLENABLE)m_opengl.GetProcAddress("glEnable");
		m_functions->glPixelStorei = (OpenglFunctions::GLPIXELSTOREI)m_opengl.GetProcAddress("glPixelStorei");
	}

	bool OpenGL::Init(unsigned scale_x, unsigned scale_y) {
		m_scale_x = scale_x;
		m_scale_y = scale_y;

		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);

		SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
		SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

		m_window = SDL_CreateWindow("Game", SDL_WINDOWPOS_UNDEFINED,
			SDL_WINDOWPOS_UNDEFINED, GBA_WIDTH * m_scale_x, GBA_HEIGHT * m_scale_y,
			SDL_WINDOW_OPENGL);

		if (!m_window) {
			LOG_ERROR("SDL_CreateWindow() Failed ! Error : {}", SDL_GetError());
			return false;
		}

		m_gl_context = SDL_GL_CreateContext(m_window);

		if (!m_gl_context) {
			LOG_ERROR("SDL_GL_CreateContext() Failed ! Error : {}", SDL_GetError());
			return false;
		}

		m_functions->glGenTextures(1, &m_gl_data.texture_id);
		m_functions->glBindTexture(GL_TEXTURE_2D, m_gl_data.texture_id);

		m_functions->glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

		m_functions->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		m_functions->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

		m_functions->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		m_functions->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		CheckForErrors();

		return true;
	}

	bool OpenGL::ConfirmEventTarget(SDL_Event* ev) {
		return SDL_GetWindowFromID(ev->window.windowID) == m_window;
	}

	void OpenGL::ProcessEvent(SDL_Event* ev) {
		switch (ev->type)
		{
		case SDL_WINDOWEVENT: {
			if (ev->window.type == SDL_WINDOWEVENT_CLOSE)
				m_stop = true;
		}
		break;

		case SDL_KEYDOWN: {
			KeyDown(&ev->key);
		}
		break;

		case SDL_KEYUP: {
			KeyUp(&ev->key);
		}
		break;

		default:
			break;
		}
	}

	void OpenGL::KeyDown(SDL_KeyboardEvent* ev) {
		using input::Buttons;

		switch (ev->keysym.scancode)
		{
		case SDL_SCANCODE_RIGHT:
			m_keypad->KeyPressed(Buttons::BUTTON_RIGHT);
			break;

		case SDL_SCANCODE_LEFT:
			m_keypad->KeyPressed(Buttons::BUTTON_LEFT);
			break;

		case SDL_SCANCODE_UP:
			m_keypad->KeyPressed(Buttons::BUTTON_UP);
			break;

		case SDL_SCANCODE_DOWN:
			m_keypad->KeyPressed(Buttons::BUTTON_DOWN);
			break;

		default:
			break;
		}
	}

	void OpenGL::KeyUp(SDL_KeyboardEvent* ev) {
		using input::Buttons;

		switch (ev->keysym.scancode)
		{
		case SDL_SCANCODE_RIGHT:
			m_keypad->KeyReleased(Buttons::BUTTON_RIGHT);
			break;

		case SDL_SCANCODE_LEFT:
			m_keypad->KeyReleased(Buttons::BUTTON_LEFT);
			break;

		case SDL_SCANCODE_UP:
			m_keypad->KeyReleased(Buttons::BUTTON_UP);
			break;

		case SDL_SCANCODE_DOWN:
			m_keypad->KeyReleased(Buttons::BUTTON_DOWN);
			break;

		default:
			break;
		}
	}

	void OpenGL::SetFrame(float* buffer) {
		std::copy_n(buffer, GBA_WIDTH * GBA_HEIGHT * 3,
			m_gl_data.placeholder_data);
	}

	void OpenGL::PresentFrame() {
		m_functions->glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		m_functions->glClear->call<void>((unsigned)GL_COLOR_BUFFER_BIT);

		m_functions->glEnable(GL_TEXTURE_2D);

		m_functions->glBindTexture(GL_TEXTURE_2D, m_gl_data.texture_id);

		m_functions->glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB,
			240, 160, 0, GL_RGB, GL_FLOAT, (void*)m_gl_data.placeholder_data);

		double h = GBA_HEIGHT * (double)m_scale_y;
		double w = GBA_WIDTH * (double)m_scale_x;

		m_functions->glViewport(0, 0, (GLsizei)w, (GLsizei)h);

		m_functions->glMatrixMode(GL_PROJECTION);
		m_functions->glLoadIdentity();

		m_functions->gluOrtho2D(0, w, h, 0);

		CheckForErrors();

		m_functions->glBegin(GL_QUADS);

		m_functions->glTexCoord2f(0.0f, 0.0f);
		m_functions->glVertex2f(0.0f, 0.0f);
		m_functions->glTexCoord2f(1.0f, 0.0f);
		m_functions->glVertex2f((float)w, 0);
		m_functions->glTexCoord2f(1.0f, 1.0f);
		m_functions->glVertex2f((float)w, (float)h);
		m_functions->glTexCoord2f(0.0f, 1.0f);
		m_functions->glVertex2f(0.0f, (float)h);

		m_functions->glEnd();

		CheckForErrors();

		SDL_GL_SwapWindow(m_window);
	}

	OpenGL::~OpenGL() {
		if (m_gl_context) {
			m_functions->glDeleteTextures(1, &m_gl_data.texture_id);
			SDL_GL_DeleteContext(m_gl_context);
			SDL_DestroyWindow(m_window);
		}

		delete[] m_gl_data.placeholder_data;
	}
}