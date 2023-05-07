#pragma once

#include "../VideoOutput.hpp"
#include "../../shared_obj/SharedObject.hpp"

struct SDL_Window;

namespace GBA::video::renderer {
	struct OpenglFunctions;

	class OpenGL : public VideoOutput {
	public :
		OpenGL();

		bool Init(unsigned scale_x, unsigned scale_y) override;

		bool ConfirmEventTarget(SDL_Event* ev) override;
		void ProcessEvent(SDL_Event* ev) override;

		void PresentFrame(common::u8* frame) override;

		~OpenGL() override;

	private :
		void LoadOpengl();

	private :
		SDL_Window* m_window;
		void* m_gl_context;
		OpenglFunctions* m_functions;
		shared_object::SharedObject m_opengl;
	};
}