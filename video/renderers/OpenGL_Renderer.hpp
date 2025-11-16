#pragma once

#include "../VideoOutput.hpp"

#include <functional>
#include <vector>
#include <string>
#include <chrono>
#include <unordered_map>
#include <filesystem>

struct SDL_Window;
struct SDL_KeyboardEvent;

namespace GBA::emulation {
	class Emulator;
}

namespace GBA::audio {
	class AudioDevice;
}

namespace GBA::video::renderer {
	struct OpenglFunctions;

	using QuickSaveCallback = std::function<void(bool)>;
	using RomSelected = std::function<void(std::string)>;

	class OpenGL : public VideoOutput {
	public :
		OpenGL(bool pause, bool hooks_enable);

		bool Init(unsigned scale_x, unsigned scale_y) override;

		bool ConfirmEventTarget(SDL_Event* ev) override;
		void ProcessEvent(SDL_Event* ev) override;

		void PresentFrame() override;

		void SetFrame(float* buffer) override;

		bool IsPaused() const override {
			return m_pause;
		}

		void OnPauseChange(bool is_paused);

		void SetQuickSaveAction(QuickSaveCallback callback) {
			m_quick_save = callback;
		}

		void SetRomSelectedAction(RomSelected callback) {
			m_on_select = callback;
		}

		inline void SetEmu(emulation::Emulator* emu) {
			m_emu = emu;
		}

		inline void SetAudio(audio::AudioDevice* audio) {
			m_audio = audio;
		}

		void SetupNotifications();

		~OpenGL() override;

		struct Texture {
			uint32_t texture_id = {};
			uint32_t width  = {};
			uint32_t height = {};
		};

		struct Notification {
			static constexpr auto NOTIFICATION_DURATION_SECONDS = 5.0;
			bool showing     = false;
			std::string name = {};
			std::chrono::system_clock::time_point start_time = {};
			std::vector<std::string> text_lines   = {};
			std::optional<std::string> texture_id = std::nullopt;
			std::optional<std::filesystem::path> notification_sound_file = std::nullopt;
		};

	private :
		void LoadOpengl();
		void CheckForErrors();

		void KeyDown(SDL_KeyboardEvent* ev);
		void KeyUp(SDL_KeyboardEvent* ev);

		void FileMenu();
		void CheatMenu();
		void CheatInsertWindow();
		void RewindMenu();
		void EmulationMenu();
		void AudioMenu();
		void VideoMenu();
		void AchievementsMenu();
		void AchievementsWindow();
		void TasMenu();
		void TasWindow();

		void AppendAchievementNotifications();
		void ShowNotifications();

		std::string FileDialog(std::string title, std::string filters);

		void UpdateWindowTitle();

		void Rewind(bool forward);
		void DoSavestate(std::string const& path, bool store);

		void LoadTexture(std::string const& path, std::string const& name);
		void DestroyTextures();

	private :
		SDL_Window* m_window;
		void* m_gl_context;
		
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

		QuickSaveCallback m_quick_save;
		RomSelected m_on_select;

		bool m_pause;
		bool m_show_menu_bar;
		bool m_ctrl_status;
		bool m_sync_to_audio;
		bool m_alt_status;
		bool m_enable_hooks;

		emulation::Emulator* m_emu;

		bool m_show_cheat_insert_win;

		std::chrono::system_clock::time_point m_last_frame_timestamp;
		uint64_t m_curr_fps;

		audio::AudioDevice* m_audio;
		bool m_mute;

		bool m_show_achievements_window;
		std::unordered_map<std::string, Texture> m_textures;
		
		std::vector<Notification> m_notifications;
		uint32_t m_notification_sound_device;

		bool m_show_tas_window;
	};
}