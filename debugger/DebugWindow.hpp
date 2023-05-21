#pragma once

#include "../common/Defs.hpp"
#include "DebugStructures.hpp"

namespace GBA {
	namespace cpu {
		class ARM7TDI;
		struct CPSR;
	}

	namespace memory {
		class Bus;
	}

	namespace gamepack {
		class GamePack;
	}
}

struct SDL_Window;
struct SDL_Renderer;
struct SDL_Texture;
union SDL_Event;

namespace GBA::debugger {
	class Debugger;

	class DebugWindow {
	public :
		DebugWindow(Debugger& debugger);

		void Init();
		void Stop();

		bool ConfirmEventTarget(SDL_Event* sdl_event);
		void ProcessEvent(SDL_Event* sdl_event);
		void Update();

		void DrawCpuControlWindow();
		void DrawDisassemblerWindow();
		void DrawControlWindow();
		void DrawMemoryWindow();
		void DrawGamepakWindow();
		void DrawIOMemory();

		~DebugWindow();

		bool IsRunning();
		bool StopRequested();

		EmulatorStatus& GetEmulatorStatus() {
			return m_emu_status;
		}

	private :
		void DrawCPSR(cpu::CPSR& cpsr);
		void DrawSPSR();
		void DrawRegisters();
		void DrawMemoryRegion(unsigned id);

		Debugger& m_debugger;

		cpu::ARM7TDI* m_processor;
		memory::Bus* m_bus;
		gamepack::GamePack* m_pack;

		bool m_running;
		bool m_stop_request;

		//Specific SDL stuff
		SDL_Window* m_sdl_window;
		SDL_Renderer* m_sdl_renderer;

		unsigned m_disassembler_address;
		bool m_follow_pc;

		SDL_Texture* m_up_arrow;
		SDL_Texture* m_down_arrow;

		EmulatorStatus m_emu_status;

		common::u32 m_memory_view_addresses[8];

		common::u32 m_break_address;
	};
}