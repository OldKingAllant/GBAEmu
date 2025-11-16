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

#define STB_IMAGE_IMPLEMENTATION
#include "../../thirdparty/stb/stb_image.h"

#include <vector>
#include <filesystem>
#include <fstream>

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
		m_audio{nullptr}, m_mute{false},
		m_show_achievements_window{false},
		m_textures{}, m_notifications{},
		m_notification_sound_device{},
		m_show_tas_window{}
	{
		m_gl_data.placeholder_data = new float[240 * 160 * 3];
		std::fill_n(m_gl_data.placeholder_data, 240 * 160 * 3, 0.5f);

		SDL_AudioSpec dev_spec{};
		dev_spec.samples  = 4096;
		dev_spec.freq     = 44100;
		dev_spec.channels = 2;
		dev_spec.format   = AUDIO_S16;
		dev_spec.callback = nullptr;

		SDL_AudioSpec got{};

		m_notification_sound_device = uint32_t(SDL_OpenAudioDevice(
			nullptr, 0, &dev_spec, &got, 0
		));
		SDL_PauseAudioDevice(SDL_AudioDeviceID(m_notification_sound_device), 1);

		if (m_notification_sound_device == 0) {
			fmt::print("[OPENGL] Could not open notification audio device\n");
			throw std::runtime_error("Could not open device");
		}
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
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
		io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

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

		case SDL_DROPFILE: {
			if (ev->drop.windowID == SDL_GetWindowID(m_window)) {
				std::filesystem::path fname{ ev->drop.file };
				fmt::println("Dropped file {}", fname.string());
				SDL_free(ev->drop.file);

				auto extension = fname.extension().string();
				if (extension == ".gba" || extension == ".GBA") {
					if (!m_emu->IsInit()) {
						m_on_select(fname.string());
					}
					break;
				}

				if (extension == ".save" || extension == ".SAVE" ||
					extension == ".sav" || extension == ".SAV") {
					m_emu->GetContext().pack.LoadBackup(fname);
					break;
				}

				if (extension == ".state" || extension == ".STATE") {
					DoSavestate(fname.string(), false);
					break;
				}

				if (extension == ".tas" || extension == ".TAS") {
					m_emu->SetTASFile(fname.string());
				}
			}
		} break;

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
							fmt::print("[EMU] Load failed\n");
						}
						else {
							fmt::print("[EMU] Load successfull\n");
						}
					}

					ImGui::EndMenu();
				}

				ImGui::Dummy(ImVec2(0.0f, 20.0f));

				if (ImGui::BeginMenu("Store")) {
					std::string dest = FileDialog("Save", ".*");

					if (dest != "NULL") {
						if (!m_emu->GetContext().pack.StoreBackup(dest)) {
							fmt::print("[EMU] Save failed\n");
						}
						else {
							fmt::print("[EMU] Save OK\n");
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
							fmt::print("[CHEATS] Enabled \"{}\"\n", name);
						else 
							fmt::print("[CHEATS] Could not enable \"{}\"\n", name);
					}
					else {
						//Was on
						m_emu->DisableCheat(name);
						fmt::print("[CHEATS] Disabled \"{}\"\n", name);
						fmt::print("[CHEATS] You may need to reset depending on the cheat\n");
					}
				}

			}

			for (auto& cheat_name : cheats_for_deletion) {
				fmt::print("[CHEATS] Removing \"{}\"\n", cheat_name);
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
					fmt::print("[EMU] Reset failed\n");
				}
			}

			auto& proc = m_emu->GetContext().processor;
			auto const& the_cache = proc.GetCache();

			if (!the_cache.IsInit()) {
				ImGui::BeginDisabled();
			}

			bool cache_enabled = proc.IsCacheEnabled();
			if (ImGui::Checkbox("Enable interpreter cache", &cache_enabled)) {
				proc.SetEnableCachedInterpreter(cache_enabled);
			}

			if (!the_cache.IsInit()) {
				ImGui::EndDisabled();
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

	void OpenGL::AchievementsMenu() {
		if (ImGui::BeginMenu("Achievements")) {
			ImGui::Checkbox("Show window", &m_show_achievements_window);
			ImGui::EndMenu();
		}
	}

	void OpenGL::AchievementsWindow() {
		ImGui::Begin("Achievements", &m_show_achievements_window);
		
		auto& ra = m_emu->GetRetroAchievements();

		if (ra.IsLoggedIn()) {
			auto const& user = ra.GetUser();

			ImGui::Text("Retro Achivemenets Summary");

			if (!m_textures.contains("user_avatar")) {
				LoadTexture(ra.GetAvatarPath(), "user_avatar");
			}

			{
				ImGui::BeginTable("UserDesc", 2, ImGuiTableFlags_Resizable);

				ImGui::TableNextRow();
				ImGui::TableNextColumn();
				
				{
					auto const& texture = m_textures["user_avatar"];
					ImGui::Image(ImTextureID(texture.texture_id), ImVec2(
						float(texture.width), float(texture.height)
					));
				}

				ImGui::TableNextColumn();
				ImGui::Text("Logged in as   : %s", user.username.c_str());
				ImGui::Text("Total points   : %u", user.user_internal.score);
				ImGui::Text("Softcore points: %u", user.user_internal.score_softcore);

				ImGui::EndTable();
			}
			

			ImGui::Separator();
			auto game_info = ra.GetGameInfo();
			ImGui::Text("Game title: %s", game_info->title);

			if (!m_textures.contains("game_badge")) {
				LoadTexture(ra.GetGameBadgePath(), "game_badge");
			}

			{
				auto const& texture = m_textures["game_badge"];
				ImGui::Image(ImTextureID(texture.texture_id), ImVec2(
					float(texture.width), float(texture.height)
				));
			}

			auto const& game_summary = ra.GetUserGameSummary();
			ImGui::Text("Unlocked %u of %u achievements",
				game_summary.num_unlocked_achievements,
				game_summary.num_core_achievements);

			
			for (auto const& bucket : ra.GetAchievements()) {
				auto const& ach_list = bucket.second;

				auto table_name = fmt::format("AchTable_{}", bucket.first);

				ImGui::Separator();
				ImGui::Text("%s", bucket.first.c_str());
				ImGui::BeginTable(table_name.c_str(), 3, ImGuiTableFlags_Borders |
					ImGuiTableFlags_Resizable);

				for (auto const& ach : ach_list) {
					auto image_path = ra.GetGameCachePath();
					image_path += fmt::vformat(ach.unlocked ?
						"/{}_unlocked.png" : "/{}_locked.png",
						fmt::make_format_args(ach.name));

					ImGui::TableNextRow();

					ImGui::TableNextColumn();
					
					std::string texture_name = ach.name;
					texture_name += ach.unlocked ? "_unlocked" : "_locked";
					if (!m_textures.contains(texture_name)) {
						LoadTexture(image_path, texture_name);
					}

					{
						auto const& texture = m_textures[texture_name];
						ImGui::Image(ImTextureID(texture.texture_id), ImVec2(
							float(texture.width), float(texture.height)
						));
					}

					ImGui::TableNextColumn();
					ImGui::Text("%s", ach.description.c_str());
					ImGui::TableNextColumn();

					if (ach.type == emulation::RetroAchievements::RA_AchievementType::UNOFFICIAL) {
						ImGui::Text("Unofficial");
					}
					else {
						ImGui::Text("Core");
					}
					
				}

				ImGui::EndTable();
			}

		}
		else {
			ImGui::Text("Not signed in");
		}

		ImGui::End();
	}

	void OpenGL::TasMenu() {
		if (ImGui::BeginMenu("TAS")) {
			bool record_inputs = m_emu->IsInputRecordingEnabled();
			ImGui::Checkbox("Enable input recording", &record_inputs);
			m_emu->EnableInputRecording(record_inputs);

			if (ImGui::BeginMenu("Store inputs")) {
				std::string dest = FileDialog("Store saved inputs", ".*");

				if (dest != "NULL") {
					m_emu->SaveRecordedInputs(dest);
				}

				ImGui::EndMenu();
			}

			bool fake_prefetch = m_emu->IsFakePrefetchEnabled();
			ImGui::Checkbox("Fake ROM prefetch", &fake_prefetch);
			m_emu->EnableFakePrefetch(fake_prefetch);

			if (m_emu->GetTAS().HasActions()) {
				ImGui::Checkbox("Show command window", &m_show_tas_window);
			}

			if (ImGui::BeginMenu("Load file")) {
				std::string tas = FileDialog("Load TAS", ".*");

				if (tas != "NULL") {
					m_emu->SetTASFile(tas);
				}

				ImGui::EndMenu();
			}

			ImGui::EndMenu();
		}
	}

	void OpenGL::TasWindow() {
		static int32_t s_action_count = {10};
		if (ImGui::Begin("TAS", &m_show_tas_window)) {
			using namespace common;

			auto keystat = m_emu->GetContext().keypad.GetKeyStatus();
			const auto c_BTNCOUNT = uint32_t(std::log2(double(input::Buttons::BUTTON_L)));

			/*
			BUTTON_A = 0x1,
			BUTTON_B = 0x2,
			BUTTON_SELECT = 0x4,
			BUTTON_START = 0x8,
			BUTTON_RIGHT = 0x10,
			BUTTON_LEFT = 0x20,
			BUTTON_UP = 0x40,
			BUTTON_DOWN = 0x80,
			BUTTON_R = 0x100,
			BUTTON_L = 0x200
			*/
			constexpr const char* c_BTN_NAMES[] = {
				"A", "B", "SELECT", "START", "RIGHT",
				"LEFT", "UP", "DOWN", "R", "L"
			};
			for (uint32_t btn_id = 0; btn_id <= c_BTNCOUNT; btn_id++) {
				bool is_btn_pressed = (~keystat & u16(1 << btn_id)) != 0;
				auto curr_btn_name = c_BTN_NAMES[btn_id];
				std::string texture_name = "./assets/images/";
				if (is_btn_pressed) {
					texture_name += fmt::format("{}_button.png", curr_btn_name);
				}
				else {
					texture_name += fmt::format("{}_button_not_pressed.png", curr_btn_name);
				}

				if (!m_textures.contains(texture_name)) {
					LoadTexture(texture_name, texture_name);
				}

				ImGui::SameLine();
				auto const& texture = m_textures[texture_name];
				ImGui::Image(ImTextureID(texture.texture_id), ImVec2(
					40.0f, 40.0f
				));
			}

			auto const& tas = m_emu->GetTAS();
			ImGui::SliderInt("Directives shown", &s_action_count, 0, 100, "%d");
			int32_t curr_action = {};
			auto const& actions = tas.GetActions();
			auto action_id = tas.GetCurrentActionId();
			while (curr_action < s_action_count && size_t(curr_action) + action_id < actions.size()) {
				tas::Action action = actions[size_t(curr_action) + action_id];
				std::string keys = {};

				if (action.buttons == 0x3FF) {
					keys = "WAIT";
				}
				else {
					uint32_t keycnt = 0;
					if ((~action.buttons & u16(input::Buttons::BUTTON_A)) != 0) {
						keys += "A";
						++keycnt;
					}

					if ((~action.buttons & u16(input::Buttons::BUTTON_B)) != 0) {
						if (keycnt > 0) {
							keys += " , ";
						}
						keys += "B";
						++keycnt;
					}

					if ((~action.buttons & u16(input::Buttons::BUTTON_SELECT)) != 0) {
						if (keycnt > 0) {
							keys += " , ";
						}
						keys += "SELECT";
						++keycnt;
					}

					if ((~action.buttons & u16(input::Buttons::BUTTON_START)) != 0) {
						if (keycnt > 0) {
							keys += " , ";
						}
						keys += "START";
						++keycnt;
					}

					if ((~action.buttons & u16(input::Buttons::BUTTON_RIGHT)) != 0) {
						if (keycnt > 0) {
							keys += " , ";
						}
						keys += "RIGHT";
						++keycnt;
					}

					if ((~action.buttons & u16(input::Buttons::BUTTON_LEFT)) != 0) {
						if (keycnt > 0) {
							keys += " , ";
						}
						keys += "LEFT";
						++keycnt;
					}

					if ((~action.buttons & u16(input::Buttons::BUTTON_UP)) != 0) {
						if (keycnt > 0) {
							keys += " , ";
						}
						keys += "UP";
						++keycnt;
					}

					if ((~action.buttons & u16(input::Buttons::BUTTON_DOWN)) != 0) {
						if (keycnt > 0) {
							keys += " , ";
						}
						keys += "DOWN";
						++keycnt;
					}

					if ((~action.buttons & u16(input::Buttons::BUTTON_R)) != 0) {
						if (keycnt > 0) {
							keys += " , ";
						}
						keys += "R";
						++keycnt;
					}

					if ((~action.buttons & u16(input::Buttons::BUTTON_L)) != 0) {
						if (keycnt > 0) {
							keys += " , ";
						}
						keys += "L";
						++keycnt;
					}
				}

				const char* c_FORMAT = "%s for %d frames";
				if (curr_action == 0) {
					ImGui::TextColored(ImVec4(255.f, 255.f, 0.f, 255.f), c_FORMAT,
						keys.c_str(), action.num_frames);
				}
				else {
					ImGui::Text(c_FORMAT, keys.c_str(), action.num_frames);
				}
				curr_action++;
			}
			//ImGui::PopFont();
		}
		ImGui::End();
	}

	void OpenGL::AppendAchievementNotifications() {
		if (!m_emu->GetAchievementsEnabled())
			return;

		auto& ra = m_emu->GetRetroAchievements();

		//////////////////////////////////////

		auto& progress_updates = ra.GetProgressUpdates();

		for (auto& ach : progress_updates) {
			auto image_path = ra.GetGameCachePath();
			image_path += fmt::format("/{}_locked.png",
				ach.name);
			std::string texture_name = fmt::format("{}_locked",
				ach.name);

			if (!m_textures.contains(texture_name)) {
				LoadTexture(image_path, texture_name);
			}

			{
				Notification notification{};
				notification.name = fmt::format("{}_progress_update", ach.name);
				notification.texture_id = texture_name;
				notification.text_lines.push_back(ach.progress);
				m_notifications.push_back(std::move(notification));
			}
		}

		ra.ClearProgressUpdates();

		///////////////////////////////////////

		auto& recently_unlocked = ra.GetRecentlyUnlocked();

		for (auto& ach : recently_unlocked) {
			auto image_path = ra.GetGameCachePath();
			image_path += fmt::format("/{}_unlocked.png",
				ach.name);
			std::string texture_name = fmt::format("{}_unlocked", 
				ach.name);
			
			if (!m_textures.contains(texture_name)) {
				LoadTexture(image_path, texture_name);
			}

			{
				Notification notification{};
				notification.name = texture_name;
				notification.texture_id = texture_name;
				notification.text_lines.push_back("Achievement unlocked!");
				notification.text_lines.push_back(ach.description);
				notification.notification_sound_file = std::filesystem::path{
					"./assets/sound/unlock.wav"
				};
				m_notifications.push_back(std::move(notification));
			}
		}

		ra.ClearRecentlyUnlocked();

		//////////////////////////////////////////////

		if (ra.mastery_achieved_interrupt) {
			ra.mastery_achieved_interrupt = false;

			if (!m_textures.contains("game_badge")) {
				LoadTexture(ra.GetGameBadgePath(), "game_badge");
			}

			{
				auto const& game_summary = ra.GetUserGameSummary();

				Notification notification{};
				notification.name = "mastery";
				notification.texture_id = "game_badge";
				notification.text_lines.push_back("Game completed!");
				notification.text_lines.push_back(fmt::format("{} achievements, {} points", 
					game_summary.num_core_achievements, 
					game_summary.points_core));
				notification.notification_sound_file = std::filesystem::path{
					"./assets/sound/unlock.wav"
				};
				m_notifications.push_back(std::move(notification));
			}
		}
	}

	void OpenGL::ShowNotifications() {
		if (m_notifications.empty())
			return;

		auto& notification = m_notifications.front();

		if (!notification.showing) {
			notification.showing = true;
			notification.start_time = std::chrono::system_clock::now();

			SDL_AudioSpec wav_spec{};
			Uint32 wav_len{};
			Uint8* wav_buf{nullptr};

			if (notification.notification_sound_file.has_value()) {
				auto const& sound_file = notification.notification_sound_file.value();
				auto sound_file_str = sound_file.string();
				if (std::filesystem::exists(sound_file) &&
					sound_file.extension().string() == ".wav" &&
					SDL_LoadWAV(sound_file_str.c_str(), &wav_spec, &wav_buf, 
						&wav_len)) {
					SDL_PauseAudioDevice(SDL_AudioDeviceID(m_notification_sound_device), 0);
					SDL_QueueAudio(SDL_AudioDeviceID(m_notification_sound_device),
						wav_buf, wav_len);
					SDL_FreeWAV(wav_buf);
				}
			}
		}

		auto window_name = fmt::format("{}_notification",
			notification.name);

		ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
		ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.0f, 0.4f, 0.5f));
		ImGui::Begin(window_name.c_str(), nullptr,
			ImGuiWindowFlags_NoCollapse |
			ImGuiWindowFlags_NoFocusOnAppearing |
			ImGuiWindowFlags_NoInputs |
			ImGuiWindowFlags_NoTitleBar |
			ImGuiWindowFlags_AlwaysAutoResize
		);

		if (notification.texture_id.has_value()) {
			auto const& texture_id = notification.texture_id.value();

			auto table_name = fmt::format("{}_notification_table",
				notification.name);
			//Use table
			ImGui::BeginTable(table_name.c_str(), 2);
			ImGui::TableNextRow();

			ImGui::TableNextColumn();

			for (auto const& text_line : notification.text_lines) {
				ImGui::Text(text_line.c_str());
			}
			
			ImGui::PopStyleColor();
			ImGui::TableNextColumn();

			{
				auto const& texture = m_textures[texture_id];
				ImGui::Image(ImTextureID(texture.texture_id), ImVec2(
					40.0f, 40.0f
				));
			}

			ImGui::EndTable();
		}
		else {
			//Use normal text lines
			for (auto const& text_line : notification.text_lines) {
				ImGui::Text(text_line.c_str());
			}

			ImGui::PopStyleColor();
		}

		auto now = std::chrono::system_clock::now();
		auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
			now - notification.start_time
		).count();

		if (elapsed >= Notification::NOTIFICATION_DURATION_SECONDS) {
			if (notification.notification_sound_file.has_value()) {
				SDL_PauseAudioDevice(SDL_AudioDeviceID(m_notification_sound_device), 1);
			}
			m_notifications.erase(m_notifications.begin());
		}

		ImGui::End();
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

		AppendAchievementNotifications();

		bool showing_notification = !m_notifications.empty();

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplSDL2_NewFrame();
		ImGui::NewFrame();

		//ImGui::DockSpaceOverViewport();

		if (m_show_menu_bar || showing_notification) {
			if (m_show_menu_bar) {
				ImGui::BeginMainMenuBar();

				FileMenu();
				EmulationMenu();
				CheatMenu();
				RewindMenu();
				AudioMenu();
				VideoMenu();
				TasMenu();

				if (m_emu->GetAchievementsEnabled()) {
					AchievementsMenu();
				}

				ImGui::EndMainMenuBar();
			}

			if (showing_notification) {
				ShowNotifications();
			}

			if (m_show_cheat_insert_win) {
				CheatInsertWindow();
			}

			if (m_show_achievements_window) {
				AchievementsWindow();
			}
		}

		if (m_show_tas_window) {
			TasWindow();
		}

		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		auto& io = ImGui::GetIO();
		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
			void* gl_current = SDL_GL_GetCurrentContext();
			void* curr_window = SDL_GL_GetCurrentWindow();

			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();

			SDL_GL_MakeCurrent((SDL_Window*)curr_window, gl_current);
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
				fmt::print("[EMU] Cannot forward\n");
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
				fmt::print("[EMU] Cannot backward\n");
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

	void OpenGL::LoadTexture(std::string const& path, std::string const& name) {
		if (!std::filesystem::exists(path)) {
			fmt::print("[OPENGL] Could not load image from {}\n", path);
			return;
		}

		int w{}, h{}, channels{};

#ifdef _MSC_VER
		FILE* fd{ nullptr };
		(void)fopen_s(&fd, path.c_str(), "r+b");
#else
		auto fd = std::fopen(path.c_str(), "r+b");
#endif 

		if (fd == nullptr) {
			fmt::print("[OPENGL] Could not load image from {}\n", path);
			return;
		}
		
		auto loaded_image = stbi_load_from_file(fd, &w, &h, &channels, 4);

		if (loaded_image == nullptr) {
			fmt::print("[OPENGL] Could not load image from {}\n", path);
			return;
		}

		GLuint texture_id{};
		glGenTextures(1, &texture_id);
		glBindTexture(GL_TEXTURE_2D, texture_id);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_BORDER);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

		glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA,
			GL_UNSIGNED_BYTE, std::bit_cast<void*>(loaded_image));

		glBindTexture(GL_TEXTURE_2D, 0);

		stbi_image_free(loaded_image);
		std::fclose(fd);

		CheckForErrors();

		m_textures[name] = Texture{
			.texture_id = texture_id,
			.width  = uint32_t(w),
			.height = uint32_t(h)
		};
	}

	void OpenGL::DestroyTextures() {
		for (auto& [_, texture] : m_textures) {
			glDeleteTextures(1, std::bit_cast<GLuint*>(&texture));
		}
	}

	void OpenGL::SetupNotifications() {
		if (m_emu->GetAchievementsEnabled()) {
			auto& ra = m_emu->GetRetroAchievements();
			auto& user = ra.GetUser();

			{
				std::string avatar_text = fmt::format("Logged in as: {}",
					user.username);

				Notification notification{};
				notification.name = "show_avatar";
				notification.texture_id = std::string{"user_avatar"};
				notification.text_lines.push_back(avatar_text);
				notification.text_lines.push_back(fmt::format(
					"Score : {}", user.user_internal.score
				));
				m_notifications.push_back(std::move(notification));

				if (!m_textures.contains("user_avatar")) {
					LoadTexture(ra.GetAvatarPath(), "user_avatar");
				}
			}

			{
				auto game_info = ra.GetGameInfo();
				auto const& game_summary = ra.GetUserGameSummary();

				Notification notification{};
				notification.name = "game_loaded";
				notification.texture_id = std::string{"game_badge"};
				notification.text_lines.push_back(fmt::format(
					"Loaded: {}", game_info->title
				));
				notification.text_lines.push_back(fmt::format(
					"Unlocked {}/{} achievements", 
					game_summary.num_unlocked_achievements, 
					game_summary.num_core_achievements
				));
				m_notifications.push_back(std::move(notification));

				if (!m_textures.contains("game_badge")) {
					LoadTexture(ra.GetGameBadgePath(), "game_badge");
				}
			}

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
			DestroyTextures();
			ImGui_ImplOpenGL3_Shutdown();
			ImGui_ImplSDL2_Shutdown();
			ImGui::DestroyContext();
			glDeleteTextures(1, &m_gl_data.texture_id);
			SDL_GL_DeleteContext(m_gl_context);
			SDL_DestroyWindow(m_window);
		}

		if (m_notification_sound_device != 0) {
			SDL_PauseAudioDevice(SDL_AudioDeviceID(m_notification_sound_device),
				1);
			SDL_CloseAudioDevice(SDL_AudioDeviceID(m_notification_sound_device));
		}

		delete[] m_gl_data.placeholder_data;
	}
}