#pragma once

#include "../VideoOutput.hpp"
#include "../../shared_obj/WinOpenglLoader.hpp"

#include <functional>
#include <vector>

struct SDL_Window;
struct SDL_KeyboardEvent;

namespace GBA::video::renderer {
	struct OpenglFunctions;

	using ConfigChangeCallback = std::function<void(std::string, std::string, std::string)>;
	using OnPause = std::function<void(bool)>;
	using RomSelected = std::function<void(std::string)>;
	using SaveLoad = std::function<void(std::string)>;
	using SaveStore = std::function<void(std::string)>;

	class OpenGL : public VideoOutput {
	public :
		OpenGL(bool pause);

		bool Init(unsigned scale_x, unsigned scale_y) override;

		bool ConfirmEventTarget(SDL_Event* ev) override;
		void ProcessEvent(SDL_Event* ev) override;

		void PresentFrame() override;

		void SetFrame(float* buffer) override;

		void SetConfigChangeCallback(ConfigChangeCallback callback) {
			m_conf_callback = callback;
		}

		void SetOnPause(OnPause callback) {
			m_on_pause = callback;
		}

		void SetRomSelectedAction(RomSelected callback) {
			m_on_select = callback;
		}

		void SetSaveSelectedAction(SaveLoad callback) {
			m_save_load = callback;
		}

		void SetSaveStoreAction(SaveStore callback) {
			m_save_store = callback;
		}

		~OpenGL() override;

	private :
		void LoadOpengl();
		void CheckForErrors();

		void KeyDown(SDL_KeyboardEvent* ev);
		void KeyUp(SDL_KeyboardEvent* ev);

		void FileMenu();

		std::string FileDialog(std::string title, std::string filters);

	private :
		SDL_Window* m_window;
		void* m_gl_context;
		OpenglFunctions* m_functions;
		shared_object::WindowsOpenglLoader m_opengl;
		shared_object::SharedObject m_glu;
		
		struct {
			uint32_t texture_id;
			float* placeholder_data;
			uint32_t program_id;
			uint32_t buffer_id;
			uint32_t vertex_array;
			int texture_loc;

			float vertex_data[24] = {
				//Vertex 1
				-1.0f, 1.0f, 0.0f, 0.0f,
				1.0f, 1.0f, 1.0f, 0.0f,
				-1.0f, -1.0f, 0.0f, 1.0f,
				//Vertex 2
				1.0f, 1.0f, 1.0f, 0.0f,
				1.0f, -1.0f, 1.0f, 1.0f,
				-1.0f, -1.0f, 0.0f, 1.0f,
			};
		} m_gl_data;

		ConfigChangeCallback m_conf_callback;
		OnPause m_on_pause;
		RomSelected m_on_select;
		SaveLoad m_save_load;
		SaveStore m_save_store;
		bool m_pause;
	};
}