#include "OpenGL_Renderer.hpp"

#include <SDL2/SDL.h>

#include <Windows.h>
#include <GL/glew.h>
#include <gl/GL.h>

#include "../../common/Logger.hpp"
#include "../../memory/Keypad.hpp"
#include "detail/OpengGLFunctions.hpp"

#include "../../ImGui/imgui.h"
#include "../../ImGui/backends/imgui_impl_sdl2.h"
#include "../../ImGui/backends/imgui_impl_opengl3.h"

#include "../../thirdparty/ImGuiFileDialog/ImGuiFileDialog.h"

namespace GBA::video::renderer {
	LOG_CONTEXT(OpenGLRenderer);

	OpenGL::OpenGL(bool pause) :
		m_window(nullptr), m_gl_context(nullptr),
		m_functions(nullptr), m_opengl(), m_glu(),
		m_gl_data{}, m_conf_callback{},
		m_on_pause{}, m_on_select{},
		m_save_load{}, m_save_store{}, m_save_state{},
		m_audio_sync{},
		m_pause{pause}, m_show_menu_bar{false},
		m_ctrl_status{false}, m_sync_to_audio{true}
	{
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

		if (glewInit() != GLEW_OK) {
			LOG_ERROR("Glew init failed!");
			return;
		}

		m_functions = LoadOpenglFunctions(&m_opengl);

		m_functions->gluOrtho2D = (OpenglFunctions::GLUORTHO2D)m_glu.GetProcAddress("gluOrtho2D");
	}

	bool OpenGL::Init(unsigned scale_x, unsigned scale_y) {
		m_scale_x = scale_x;
		m_scale_y = scale_y;

		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);

		SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
		SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

		m_window = SDL_CreateWindow("Emulator", SDL_WINDOWPOS_UNDEFINED,
			SDL_WINDOWPOS_UNDEFINED, GBA_WIDTH * m_scale_x, GBA_HEIGHT * m_scale_y,
			SDL_WINDOW_OPENGL | SDL_WINDOW_ALLOW_HIGHDPI);

		if (!m_window) {
			LOG_ERROR("SDL_CreateWindow() Failed ! Error : {}", SDL_GetError());
			return false;
		}

		m_gl_context = SDL_GL_CreateContext(m_window);

		if (!m_gl_context) {
			LOG_ERROR("SDL_GL_CreateContext() Failed ! Error : {}", SDL_GetError());
			return false;
		}

		LoadOpengl();

		m_functions->glGenBuffers(1, &m_gl_data.buffer_id);
		m_functions->glBindBuffer(GL_ARRAY_BUFFER, m_gl_data.buffer_id);
		m_functions->glBufferData(GL_ARRAY_BUFFER, 24 * sizeof(float), m_gl_data.vertex_data, GL_STATIC_DRAW);

		m_functions->glGenVertexArrays(1, &m_gl_data.vertex_array);
		m_functions->glBindVertexArray(m_gl_data.vertex_array);

		m_functions->glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
		m_functions->glEnableVertexAttribArray(0);
		m_functions->glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (const void*)(2 * sizeof(float)));
		m_functions->glEnableVertexAttribArray(1);

		m_functions->glBindBuffer(GL_ARRAY_BUFFER, 0);
		m_functions->glBindVertexArray(0);

		m_functions->glGenTextures(1, &m_gl_data.texture_id);
		m_functions->glBindTexture(GL_TEXTURE_2D, m_gl_data.texture_id);

		m_functions->glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

		m_functions->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		m_functions->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

		m_functions->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		m_functions->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		auto program = CreateProgram({ 
			std::pair{ "./assets/base.vert", GL_VERTEX_SHADER },
			std::pair{ "./assets/base.frag", GL_FRAGMENT_SHADER }
			}, m_functions);

		m_gl_data.program_id = program.first;

		m_functions->glUseProgram(m_gl_data.program_id);

		m_gl_data.texture_loc = m_functions->glGetUniformLocation(m_gl_data.program_id, "frame");

		CheckForErrors();

		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO();
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

		ImGui_ImplSDL2_InitForOpenGL(m_window, m_gl_context);
		ImGui_ImplOpenGL3_Init("#version 330");

		SDL_GL_SetSwapInterval(m_sync_to_audio);

		return true;
	}

	bool OpenGL::ConfirmEventTarget(SDL_Event* ev) {
		return SDL_GetWindowFromID(ev->window.windowID) == m_window;
	}

	void OpenGL::ProcessEvent(SDL_Event* ev) {
		ImGui_ImplSDL2_ProcessEvent(ev);

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

		case SDL_QUIT: {
			m_stop = true;
		}
		break;

		default:
			break;
		}
	}

	void OpenGL::KeyDown(SDL_KeyboardEvent* ev) {
		using input::Buttons;

		if (ev->keysym.sym == SDLK_LCTRL) {
			m_ctrl_status = true;
			return;
		}

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

		case SDL_SCANCODE_A:
			m_keypad->KeyPressed(Buttons::BUTTON_A);
			break;

		case SDL_SCANCODE_S:
			m_keypad->KeyPressed(Buttons::BUTTON_B);
			break;

		case SDL_SCANCODE_Q:
			m_keypad->KeyPressed(Buttons::BUTTON_L);
			break;

		case SDL_SCANCODE_E:
			m_keypad->KeyPressed(Buttons::BUTTON_R);
			break;

		case SDL_SCANCODE_BACKSPACE:
			m_keypad->KeyPressed(Buttons::BUTTON_SELECT);
			break;

		case SDL_SCANCODE_SPACE:
			m_keypad->KeyPressed(Buttons::BUTTON_START);
			break;

		case SDL_SCANCODE_ESCAPE:
			m_show_menu_bar = !m_show_menu_bar;
			break;

		case SDL_SCANCODE_C: {
			if (m_ctrl_status) {
				m_pause = true;
				m_on_pause(true);
			}
		}
		break;

		default:
			break;
		}
	}

	void OpenGL::KeyUp(SDL_KeyboardEvent* ev) {
		using input::Buttons;

		if (ev->keysym.sym == SDLK_LCTRL) {
			m_ctrl_status = false;
			return;
		}

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

		case SDL_SCANCODE_A:
			m_keypad->KeyReleased(Buttons::BUTTON_A);
			break;

		case SDL_SCANCODE_S:
			m_keypad->KeyReleased(Buttons::BUTTON_B);
			break;

		case SDL_SCANCODE_Q:
			m_keypad->KeyReleased(Buttons::BUTTON_L);
			break;

		case SDL_SCANCODE_E:
			m_keypad->KeyReleased(Buttons::BUTTON_R);
			break;

		case SDL_SCANCODE_BACKSPACE:
			m_keypad->KeyReleased(Buttons::BUTTON_SELECT);
			break;

		case SDL_SCANCODE_SPACE:
			m_keypad->KeyReleased(Buttons::BUTTON_START);
			break;

		default:
			break;
		}
	}

	void OpenGL::SetFrame(float* buffer) {
		std::copy_n(buffer, GBA_WIDTH * GBA_HEIGHT * 3,
			m_gl_data.placeholder_data);
	}

	std::string OpenGL::FileDialog(std::string title, std::string filters) {
		if (ImGuiFileDialog::Instance()->IsOpened() &&
			ImGuiFileDialog::Instance()->GetOpenedKey() != title)
			ImGuiFileDialog::Instance()->Close();

		ImGuiFileDialog::Instance()->OpenDialog(title, title, filters.c_str(), "");

		if (ImGuiFileDialog::Instance()->Display(title)) {
			if (ImGuiFileDialog::Instance()->IsOk()) {
				return ImGuiFileDialog::Instance()->GetCurrentPath() + "/" +
					ImGuiFileDialog::Instance()->GetCurrentFileName();
			}
		}

		return "NULL";
	}

	void OpenGL::FileMenu() {
		if (ImGui::BeginMenu("File")) {
			if (ImGui::BeginMenu("Rom")) {
				if (ImGui::BeginMenu("Load")) {
					std::string rom = FileDialog("Open Rom", ".gba,.GBA");

					if (rom != "NULL" && m_on_select) {
						m_on_select(rom);
					}

					ImGui::EndMenu();
				}

				ImGui::EndMenu();
			}

			//////////

			if (ImGui::BeginMenu("Save")) {
				if (ImGui::BeginMenu("Load")) {
					std::string save = FileDialog("Load Save", ".save,.sav,.SAVE");

					if (save != "NULL" && m_save_load) {
						m_save_load(save);
					}

					ImGui::EndMenu();
				}

				ImGui::Dummy(ImVec2(0.0f, 20.0f));

				if (ImGui::BeginMenu("Store")) {
					std::string dest = FileDialog("Save", ".*");

					if (dest != "NULL" && m_save_store) {
						m_save_store(dest);
					}

					ImGui::EndMenu();
				}

				ImGui::EndMenu();
			}

			/////////////////////

			if (ImGui::BeginMenu("State")) {
				if (ImGui::BeginMenu("Save")) {
					std::string save = FileDialog("Store state", ".*");

					if (save != "NULL" && m_save_state) {
						m_save_state(save, true);
					}

					ImGui::EndMenu();
				}

				ImGui::Dummy(ImVec2(0.0f, 20.0f));

				if (ImGui::BeginMenu("Load")) {
					std::string dest = FileDialog("Load State", ".state");

					if (dest != "NULL" && m_save_state) {
						m_save_state(dest, false);
					}

					ImGui::EndMenu();
				}

				ImGui::EndMenu();
			}

			///////////////////////

			ImGui::EndMenu();
		}
	}

	void OpenGL::PresentFrame() {
		m_functions->glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		m_functions->glClear(GL_COLOR_BUFFER_BIT);

		m_functions->glUseProgram(m_gl_data.program_id);

		m_functions->glActiveTexture(GL_TEXTURE0);
		m_functions->glBindTexture(GL_TEXTURE_2D, m_gl_data.texture_id);

		m_functions->glUniform1i(m_gl_data.texture_loc, 0);

		m_functions->glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB,
			240, 160, 0, GL_RGB, GL_FLOAT, (void*)m_gl_data.placeholder_data);

		m_functions->glBindVertexArray(m_gl_data.vertex_array);

		m_functions->glDrawArrays(GL_TRIANGLES, 0, 6);

		m_functions->glBindVertexArray(0);

		if (m_show_menu_bar) {
			ImGui_ImplOpenGL3_NewFrame();
			ImGui_ImplSDL2_NewFrame();
			ImGui::NewFrame();

			ImGui::BeginMainMenuBar();

			FileMenu();

			if (ImGui::BeginMenu("Emulation")) {
				ImGui::Checkbox("Pause", &m_pause);

				if (m_on_pause)
					m_on_pause(m_pause);

				ImGui::Checkbox("Audio sync (60fps)", &m_sync_to_audio);

				if (m_audio_sync) {
					m_audio_sync(m_sync_to_audio);
				}

				SDL_GL_SetSwapInterval(m_sync_to_audio);

				ImGui::EndMenu();
			}

			ImGui::EndMainMenuBar();

			ImGui::Render();
			ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
		}

		SDL_GL_SwapWindow(m_window);
	}

	OpenGL::~OpenGL() {
		if (m_gl_context) {
			ImGui_ImplOpenGL3_Shutdown();
			ImGui_ImplSDL2_Shutdown();
			ImGui::DestroyContext();
			m_functions->glDeleteTextures(1, &m_gl_data.texture_id);
			SDL_GL_DeleteContext(m_gl_context);
			SDL_DestroyWindow(m_window);
		}

		delete[] m_gl_data.placeholder_data;
	}
}