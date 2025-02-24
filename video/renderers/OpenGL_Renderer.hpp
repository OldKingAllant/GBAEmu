#pragma once

#include "../VideoOutput.hpp"

#include <functional>
#include <vector>
#include <string>

struct SDL_Window;
struct SDL_KeyboardEvent;

namespace GBA::emulation {
	class Emulator;
}

namespace GBA::video::renderer {
	struct OpenglFunctions;

	using QuickSaveCallback = std::function<void(bool)>;
	using OnPause = std::function<void(bool)>;
	using RomSelected = std::function<void(std::string)>;
	using SaveLoad = std::function<void(std::string)>;
	using SaveStore = std::function<void(std::string)>;
	using SaveStateRequest = std::function<void(std::string, bool)>;
	using SyncToAudioCallback = std::function<void(bool)>;
	using RewindCallback = std::function<void(bool)>;
	using ResetCallback = std::function<void()>;
	using HooksEnableCallback = std::function<void(bool)>;

	class OpenGL : public VideoOutput {
	public :
		OpenGL(bool pause, bool hooks_enable);

		bool Init(unsigned scale_x, unsigned scale_y) override;

		bool ConfirmEventTarget(SDL_Event* ev) override;
		void ProcessEvent(SDL_Event* ev) override;

		void PresentFrame() override;

		void SetFrame(float* buffer) override;

		void SetQuickSaveAction(QuickSaveCallback callback) {
			m_quick_save = callback;
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

		void SetSaveStateAction(SaveStateRequest callback) {
			m_save_state = callback;
		}

		void SetSyncToAudioCallback(SyncToAudioCallback callback) {
			m_audio_sync = callback;
		}

		void SetRewindAction(RewindCallback callback) {
			m_rewind = callback;
		}

		void SetResetAction(ResetCallback callback) {
			m_reset = callback;
		}

		void SetHooksEnableAction(HooksEnableCallback callback) {
			m_hooks = callback;
		}

		inline void SetEmu(emulation::Emulator* emu) {
			m_emu = emu;
		}

		~OpenGL() override;

	private :
		void LoadOpengl();
		void CheckForErrors();

		void KeyDown(SDL_KeyboardEvent* ev);
		void KeyUp(SDL_KeyboardEvent* ev);

		void FileMenu();
		void CheatMenu();
		void CheatInsertWindow();
		void RewindMenu();

		std::string FileDialog(std::string title, std::string filters);

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
		OnPause m_on_pause;
		RomSelected m_on_select;
		SaveLoad m_save_load;
		SaveStore m_save_store;
		SaveStateRequest m_save_state;
		SyncToAudioCallback m_audio_sync;
		RewindCallback m_rewind;
		ResetCallback m_reset;
		HooksEnableCallback m_hooks;

		bool m_pause;
		bool m_show_menu_bar;
		bool m_ctrl_status;
		bool m_sync_to_audio;
		bool m_alt_status;
		bool m_enable_hooks;

		emulation::Emulator* m_emu;

		bool m_show_cheat_insert_win;
	};
}