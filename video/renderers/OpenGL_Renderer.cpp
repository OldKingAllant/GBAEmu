#include "OpenGL_Renderer.hpp"

#include <SDL2/SDL.h>

#include <Windows.h>
#include <gl/GL.h>

#include "../../common/Logger.hpp"

namespace GBA::video::renderer {
	LOG_CONTEXT(OpenGLRenderer);

	struct OpenglFunctions {
		typedef void (APIENTRY * GLCLEAR)(GLbitfield mask);
		typedef void (APIENTRY * GLCLEARCOLOR)(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha);

		GLCLEAR glClear;
		GLCLEARCOLOR glClearColor;
	};

	OpenGL::OpenGL() :
		m_window(nullptr), m_gl_context(nullptr),
		m_functions(nullptr), m_opengl()
	{
		LoadOpengl();
	}

	void OpenGL::LoadOpengl() {
		if (!m_opengl.Load("opengl32.dll")) {
			LOG_ERROR("OpenGL load failed!");
			return;
		}

		m_functions = new OpenglFunctions{};

		m_functions->glClear = (OpenglFunctions::GLCLEAR)m_opengl.GetProcAddress("glClear");
		m_functions->glClearColor = (OpenglFunctions::GLCLEARCOLOR)m_opengl.GetProcAddress("glClearColor");
	}

	bool OpenGL::Init(unsigned scale_x, unsigned scale_y) {
		m_scale_x = scale_x;
		m_scale_y = scale_y;

		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);

		SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
		SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

		m_window = SDL_CreateWindow("Game", SDL_WINDOWPOS_UNDEFINED,
			SDL_WINDOWPOS_UNDEFINED, GBA_WIDTH, GBA_HEIGHT,
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

		default:
			break;
		}
	}

	void OpenGL::PresentFrame(common::u8* frame) {
		m_functions->glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		m_functions->glClear(GL_COLOR_BUFFER_BIT);

		SDL_GL_SwapWindow(m_window);
	}

	OpenGL::~OpenGL() {
		if (m_gl_context) {
			SDL_GL_DeleteContext(m_gl_context);
			SDL_DestroyWindow(m_window);
		}
	}
}