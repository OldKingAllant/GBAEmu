#include "OpenGL_Renderer.hpp"

#include <SDL2/SDL.h>
#include <GL/glew.h>

#include "../../common/Logger.hpp"
#include "../../memory/Keypad.hpp"
#include "../../emu/Emulator.hpp"
#include "detail/OpengGLFunctions.hpp"
#include "../../audio_device/AudioDevice.hpp"

#include "../../ImGui/imgui.h"
#include "../../ImGui/backends/imgui_impl_sdl2.h"
#include "../../ImGui/backends/imgui_impl_opengl3.h"
#include "../../ImGui/misc/cpp/imgui_stdlib.h"

#include "../../thirdparty/ImGuiFileDialog/ImGuiFileDialog.h"

#include <vector>

#include <fmt/format.h>

namespace GBA::video::renderer {
	LOG_CONTEXT(OpenGLRenderer);

	OpenGL::OpenGL(bool pause, bool hooks_enable) :
		m_window(nullptr), m_gl_context(nullptr),
		m_gl_data{}, m_quick_save{}, m_on_select{},
		m_pause{pause}, m_show_menu_bar{false},
		m_ctrl_status{false}, m_sync_to_audio{true},
		m_alt_status{false}, m_enable_hooks{hooks_enable},
		m_emu{nullptr}, m_show_cheat_insert_win{false},
		m_last_frame_timestamp{}, m_curr_fps{},
		m_audio{nullptr}, m_mute{false}
	{
		m_gl_data.placeholder_data = new float[240 * 160 * 3];

		std::fill_n(m_gl_data.placeholder_data, 240 * 160 * 3, 0.5f);
	}

	void OpenGL::CheckForErrors() {
		GLenum error = 0;

		while ((error = glGetError()) != GL_NO_ERROR) {
			LOG_ERROR(" OpenGL error : {}", error);
		}
	}

	void OpenGL::LoadOpengl() {
		if (glewInit() != GLEW_OK) {
			LOG_ERROR("Glew init failed!");
			return;
		}
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

		glGenBuffers(1, &m_gl_data.buffer_id);
		glBindBuffer(GL_ARRAY_BUFFER, m_gl_data.buffer_id);
		glBufferData(GL_ARRAY_BUFFER, 24 * sizeof(float), m_gl_data.vertex_data, GL_STATIC_DRAW);

		glGenVertexArrays(1, &m_gl_data.vertex_array);
		glBindVertexArray(m_gl_data.vertex_array);

		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (const void*)(2 * sizeof(float)));
		glEnableVertexAttribArray(1);

		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindVertexArray(0);

		glGenTextures(1, &m_gl_data.texture_id);
		glBindTexture(GL_TEXTURE_2D, m_gl_data.texture_id);

		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		auto program = CreateProgram({ 
			std::pair{ "./assets/base.vert", GL_VERTEX_SHADER },
			std::pair{ "./assets/base.frag", GL_FRAGMENT_SHADER }
			});

		m_gl_data.program_id = program.first;

		glUseProgram(m_gl_data.program_id);

		m_gl_data.texture_loc = glGetUniformLocation(m_gl_data.program_id, "frame");

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
		if(m_show_menu_bar)
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
			if (m_alt_status) {
				Rewind(true);
			}
			else {
				m_keypad->KeyPressed(Buttons::BUTTON_RIGHT);
			}
			break;

		case SDL_SCANCODE_LEFT:
			if (m_alt_status) {
				Rewind(false);
			}
			else {
				m_keypad->KeyPressed(Buttons::BUTTON_LEFT);
			}
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

		case SDL_SCANCODE_F5:
			if (m_quick_save)
				m_quick_save(true);
			break;

		case SDL_SCANCODE_F1:
			if (m_quick_save)
				m_quick_save(false);
			break;

		case SDL_SCANCODE_LALT:
		case SDL_SCANCODE_RALT:
			m_alt_status = true;
			OnPauseChange(true);
			break;

		case SDL_SCANCODE_C: {
			if (m_ctrl_status) {
				OnPauseChange(true);
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
			if(!m_alt_status)
				m_keypad->KeyReleased(Buttons::BUTTON_RIGHT);
			break;

		case SDL_SCANCODE_LEFT:
			if(!m_alt_status)
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

		case SDL_SCANCODE_LALT:
		case SDL_SCANCODE_RALT:
			m_alt_status = false;
			OnPauseChange(false);
			break;

		default:
			break;
		}
	}

	void OpenGL::SetFrame(float* buffer) {
		std::copy_n(buffer, GBA_WIDTH * GBA_HEIGHT * 3,
			m_gl_data.placeholder_data);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB,
			240, 160, 0, GL_RGB, GL_FLOAT, 
			(void*)m_gl_data.placeholder_data);
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

					if (save != "NULL") {
						if (!m_emu->GetContext().pack.LoadBackup(save)) {
							fmt::println("[PPU] Load failed");
						}
						else {
							fmt::println("[PPU] Load successfull");
						}
					}

					ImGui::EndMenu();
				}

				ImGui::Dummy(ImVec2(0.0f, 20.0f));

				if (ImGui::BeginMenu("Store")) {
					std::string dest = FileDialog("Save", ".*");

					if (dest != "NULL") {
						if (!m_emu->GetContext().pack.StoreBackup(dest)) {
							fmt::println("[EMU] Save failed");
						}
						else {
							fmt::println("[EMU] Save OK");
						}
					}

					ImGui::EndMenu();
				}

				ImGui::EndMenu();
			}

			/////////////////////

			if (ImGui::BeginMenu("State")) {
				if (ImGui::BeginMenu("Save")) {
					std::string save = FileDialog("Store state", ".*");

					if (save != "NULL") {
						DoSavestate(save, true);
					}

					ImGui::EndMenu();
				}

				ImGui::Dummy(ImVec2(0.0f, 20.0f));

				if (ImGui::BeginMenu("Load")) {
					std::string dest = FileDialog("Load State", ".state");

					if (dest != "NULL") {
						DoSavestate(dest, false);
					}

					ImGui::EndMenu();
				}

				ImGui::EndMenu();
			}

			///////////////////////

			ImGui::EndMenu();
		}
	}

	void OpenGL::CheatMenu() {
		if (ImGui::BeginMenu("Cheats")) {
			ImGui::Checkbox("Show insert window", &m_show_cheat_insert_win);
			
			if (ImGui::Checkbox("Enable hooks", &m_enable_hooks)) {
				m_emu->EnableHooksGlobal(m_enable_hooks);
			}

			ImGui::Separator();

			auto& inserted_cheats = m_emu->GetCheats();

			std::vector<std::string> cheats_for_deletion{};

			for (uint32_t curr_id = 0; auto & [name, cheat_set] : inserted_cheats) {
				bool is_enabled = cheat_set.enabled;
				bool was_removed = false;

				auto widget_name = fmt::format("Remove##cheat_{}", curr_id);
				curr_id++;

				if (ImGui::Button(widget_name.c_str())) {
					was_removed = true;
					cheats_for_deletion.push_back(name);
				}
				else {
					ImGui::SameLine();
				}

				if (!was_removed && ImGui::Checkbox(name.c_str(), &is_enabled)) {
					if (is_enabled) {
						//Was off
						if (m_emu->EnableCheat(name))
							fmt::println("[CHEATS] Enabled \"{}\"", name);
						else 
							fmt::println("[CHEATS] Could not enable \"{}\"", name);
					}
					else {
						//Was on
						m_emu->DisableCheat(name);
						fmt::println("[CHEATS] Disabled \"{}\"", name);
						fmt::println("[CHEATS] You may need to reset depending on the cheat");
					}
				}

			}

			for (auto& cheat_name : cheats_for_deletion) {
				fmt::println("[CHEATS] Removing \"{}\"", cheat_name);
				m_emu->RemoveCheat(cheat_name);
			}

			ImGui::EndMenu();
		}
	}

	void OpenGL::CheatInsertWindow() {
		static int current_type{ 0 };
		static std::string curr_message{};
		static std::string cheat_name{}, cheat_lines{};

		ImGui::Begin("Cheat insert", &m_show_cheat_insert_win);

		ImGui::InputText("Cheat name", &cheat_name);
		ImGui::InputTextMultiline("Cheat", &cheat_lines, ImVec2(0, 0),
			ImGuiInputTextFlags_CharsHexadecimal);

		constexpr const char* TYPES[] = {
			"AR", "CB", "GS"
		};

		constexpr auto NUM_TYPES = std::distance(
			std::begin(TYPES), std::end(TYPES)
		);

		ImGui::Combo("Cheat type", &current_type, TYPES, NUM_TYPES);

		if(ImGui::Button("Insert")) {
			cheats::CheatType ty{};

			switch (current_type)
			{
			case 0:
				ty = cheats::CheatType::ACTION_REPLAY;
				break;
			case 1:
				ty = cheats::CheatType::CODE_BREAKER;
				break;
			case 2:
				ty = cheats::CheatType::GAMESHARK;
				break;
			default:
				break;
			}

			if (cheat_name.empty()) {
				curr_message = "Name cannot be empty";
			}
			else if (cheat_lines.empty()) {
				curr_message = "Please insert cheat";
			}
			else if (m_emu->AddCheat({ cheat_lines }, ty, cheat_name)) {
				curr_message = "Insert ok";
			}
			else {
				curr_message = "Could not insert cheat";
			}
		}

		ImGui::SameLine();

		if (ImGui::Button("Clear input")) {
			current_type = 0;
			cheat_name.clear();
			cheat_lines.clear();
		}

		if (!curr_message.empty()) {
			ImGui::Text("%s", curr_message.c_str());
			ImGui::SameLine();

			if (ImGui::Button("Clear")) {
				curr_message.clear();
			}
		}

		ImGui::End();
	}

	void OpenGL::RewindMenu() {
		if (ImGui::BeginMenu("Rewind")) {
			bool is_enabled = m_emu->IsRewindEnabled();

			if (ImGui::Checkbox("Enabled", &is_enabled)) {
				m_emu->SetRewindEnable(is_enabled);
			}

			ImGui::Text("Current buffer size: %d", m_emu->GetCurrentRewindBufferSize());
			ImGui::SameLine();

			if (ImGui::Button("Clear")) {
				m_emu->RewindClearBuffer();
			}

			auto max_size = int(m_emu->GetMaxRewindBufferSize());

			if (ImGui::SliderInt("Max buffer size", &max_size, 1, 60,
				"%d", ImGuiSliderFlags_AlwaysClamp)) {
				m_emu->SetRewindBufferSize(common::u32(max_size));
			}

			ImGui::EndMenu();
		}
	}

	void OpenGL::EmulationMenu() {
		if (ImGui::BeginMenu("Emulation")) {
			bool pause_temp = m_pause;
			if (ImGui::Checkbox("Pause", &pause_temp)) {
				OnPauseChange(pause_temp);
			}

			if (ImGui::Checkbox("Audio sync (60fps)", &m_sync_to_audio)) {
				SDL_GL_SetSwapInterval(m_sync_to_audio);
				m_audio->AudioSync(m_sync_to_audio);
			}

			bool hle_on  = m_emu->IsHleEnabled();
			bool hle_log = m_emu->IsHleLogEnabled();

			if (ImGui::Checkbox("HLE functions", &hle_on)) {
				m_emu->SetHleEnable(hle_on);
			}

			if (ImGui::Checkbox("HLE logging  ", &hle_log)) {
				m_emu->SetLogHleEnable(hle_log);
			}

			if (ImGui::Button("Reset")) {
				if (m_emu->Reset()) {
					auto framebuf = m_emu->GetContext() 
						.ppu
						.GetFrame(); 
					SetFrame(framebuf);
				}
				else {
					fmt::println("[EMU] Reset failed");
				}
			}

			ImGui::EndMenu();
		}
	}

	void OpenGL::AudioMenu() {
		using namespace common;

		if (ImGui::BeginMenu("Audio")) {
			if (ImGui::Checkbox("Mute", &m_mute)) {
				auto& ctx = m_emu->GetContext();

				if (m_mute) {
					m_audio->Pause();
					ctx.apu.SetCallback([](i16, i16) {}, 1024);
				}
				else {
					ctx.apu.SetCallback(
						[this](i16 sample_l, i16 sample_r) {
							m_audio->PushSample(sample_l, sample_r);
						}, 1024
					);
					m_audio->Start();
				}
			}

			ImGui::EndMenu();
		}
		
	}

	void OpenGL::VideoMenu() {
		if (ImGui::BeginMenu("Video")) {
			using namespace common;

			bool is_threaded_rendering_enabled = m_emu->IsThreadedRenderingEnabled();

			ImGui::BeginDisabled();
			ImGui::Checkbox("Threaded rendering", &is_threaded_rendering_enabled);
			ImGui::EndDisabled();

			if (!is_threaded_rendering_enabled)
				ImGui::BeginDisabled();

			bool is_threshold_enabled = m_emu->IsScanlineDiffThresholdEnabled();
			if (ImGui::Checkbox("Enable diff threshold", &is_threshold_enabled)) {
				m_emu->SetEnableScanlineDiffThreshold(is_threshold_enabled);
			}

			auto diff_threshold = int(m_emu->GetScanlineDiffThreshold());

			if (ImGui::SliderInt("Diff threshold", &diff_threshold, 1,
				ppu::PPU::VISIBLE_LINES, "%d", ImGuiSliderFlags_AlwaysClamp)) {
				m_emu->SetScanlineDiffThreshold(u8(diff_threshold));
			}

			if (!is_threaded_rendering_enabled)
				ImGui::EndDisabled();

			ImGui::EndMenu();
		}
	}

	void OpenGL::UpdateWindowTitle() {
		static uint64_t last_fps_count{0};

		auto const& internal_name = m_emu->GetContext()
			.pack.GetInternalName();

		m_curr_fps += 1;
		auto now = std::chrono::system_clock::now();
		auto diff = std::chrono::duration_cast<std::chrono::seconds>(
			now - m_last_frame_timestamp
		).count();

		if (diff >= 1) {
			m_last_frame_timestamp = now;
			last_fps_count = m_curr_fps;
			m_curr_fps = 0;

			auto title = fmt::format("GBA | {} | {} fps",
				internal_name, last_fps_count);

			SDL_SetWindowTitle(m_window, title.c_str());
		}

	}

	void OpenGL::PresentFrame() {
		UpdateWindowTitle();

		glUseProgram(m_gl_data.program_id);
		glBindTexture(GL_TEXTURE_2D, m_gl_data.texture_id);
		glBindVertexArray(m_gl_data.vertex_array);

		glDrawArrays(GL_TRIANGLES, 0, 6);

		glBindVertexArray(0);

		if (m_show_menu_bar) {
			ImGui_ImplOpenGL3_NewFrame();
			ImGui_ImplSDL2_NewFrame();
			ImGui::NewFrame();

			ImGui::BeginMainMenuBar();

			FileMenu();
			EmulationMenu();
			CheatMenu();
			RewindMenu();
			AudioMenu();
			VideoMenu();

			ImGui::EndMainMenuBar();

			if (m_show_cheat_insert_win) {
				CheatInsertWindow();
			}

			ImGui::Render();
			ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
		}

		SDL_GL_SwapWindow(m_window);
	}

	void OpenGL::Rewind(bool forward) {
		if (forward) {
			if (m_emu->RewindForward()) {
				auto framebuf = m_emu->GetContext()
					.ppu
					.GetFrame();
				SetFrame(framebuf);
			}
			else {
				fmt::println("[EMU] Cannot forward");
			}
		}
		else {
			if (m_emu->RewindBackward()) {
				auto framebuf = m_emu->GetContext()
					.ppu
					.GetFrame();
				SetFrame(framebuf);
			}
			else {
				fmt::println("[EMU] Cannot backward");
			}
		}
	}

	void OpenGL::DoSavestate(std::string const& path, bool store) {
		if (store)
			m_emu->StoreState(path);
		else {
			m_emu->LoadState(path);
			auto framebuf = m_emu->GetContext()
				.ppu
				.GetFrame();
			SetFrame(framebuf);
		}
	}

	void OpenGL::OnPauseChange(bool is_paused) {
		if (m_pause != is_paused && !is_paused) {
			m_emu->RewindPop();
		}

		m_pause = is_paused;
	}

	OpenGL::~OpenGL() {
		if (m_gl_context) {
			ImGui_ImplOpenGL3_Shutdown();
			ImGui_ImplSDL2_Shutdown();
			ImGui::DestroyContext();
			glDeleteTextures(1, &m_gl_data.texture_id);
			SDL_GL_DeleteContext(m_gl_context);
			SDL_DestroyWindow(m_window);
		}

		delete[] m_gl_data.placeholder_data;
	}
}