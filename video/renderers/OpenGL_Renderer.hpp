#pragma once

#include "../VideoOutput.hpp"
#include "../../shared_obj/SharedObject.hpp"

struct SDL_Window;
struct SDL_KeyboardEvent;

namespace GBA::video::renderer {
	struct OpenglFunctions;

	class OpenGL : public VideoOutput {
	public :
		OpenGL();

		bool Init(unsigned scale_x, unsigned scale_y) override;

		bool ConfirmEventTarget(SDL_Event* ev) override;
		void ProcessEvent(SDL_Event* ev) override;

		void PresentFrame() override;

		void SetFrame(float* buffer) override;

		~OpenGL() override;

	private :
		void LoadOpengl();
		void CheckForErrors();

		void KeyDown(SDL_KeyboardEvent* ev);
		void KeyUp(SDL_KeyboardEvent* ev);

	private :
		SDL_Window* m_window;
		void* m_gl_context;
		OpenglFunctions* m_functions;
		shared_object::SharedObject m_opengl;
		shared_object::SharedObject m_glu;
		
		struct {
			uint32_t texture_id;
			float* placeholder_data;
		} m_gl_data;
	};
}