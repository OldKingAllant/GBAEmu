#pragma once

#include <string_view>
#include <deque>

#include "../common/Defs.hpp"

#include "../cpu/core/ARM7TDI.hpp"
#include "../memory/Bus.hpp"
#include "../gamepack/GamePack.hpp"
#include "../ppu/PPU.hpp"
#include "../memory/InterruptController.hpp"
#include "../memory/EventScheduler.hpp"
#include "../memory/Keypad.hpp"
#include "../memory/Timers.hpp"
#include "../memory/DirectMemoryAccess.hpp"
#include "../apu/APU.hpp"

namespace GBA::emulation {
	struct EmulatorContext {
		cpu::ARM7TDI processor;
		memory::Bus bus;
		gamepack::GamePack pack;
		ppu::PPU ppu;
		memory::InterruptController* int_controller;
		memory::EventScheduler scheduler;
		input::Keypad keypad;
		timers::TimerChain timers;
		memory::DMA* all_dma[4];
		apu::APU apu;
	};

	class Emulator {
	public :
		Emulator(std::string_view rom_location, std::string_view bios_location);
		Emulator(std::string_view bios_location);

		EmulatorContext& GetContext() {
			return m_ctx;
		}

		void EmulateFor(common::u32 num_instructions);
		void RunTillVblank();

		void UseBIOS();
		void SkipBios();

		bool LoadRom(std::string_view loc);
		void Init();

		void StoreState(std::string const& path);
		void LoadState(std::string const& path);

		void SaveResetState();

		bool RewindPush();
		bool RewindBackward();
		bool RewindForward();
		bool RewindPop();

		bool Reset();

		void SetRewindBufferSize(common::u32 buf_size) {
			m_rewind_buf_size = buf_size;
			m_rewind_pos = 0;

			if (m_rewind_buf.size() >= m_rewind_buf_size) {
				m_rewind_buf.resize(buf_size);
			}
		}

		~Emulator();

		template <typename Ar>
		void save(Ar& ar) const {
			ar(m_ctx.processor);
			ar(m_ctx.bus);
			ar(m_ctx.pack);
			ar(m_ctx.ppu);
			ar(*m_ctx.int_controller);
			ar(m_ctx.keypad);
			ar(m_ctx.timers);
			ar(*m_ctx.all_dma[0]);
			ar(*m_ctx.all_dma[1]);
			ar(*m_ctx.all_dma[2]);
			ar(*m_ctx.all_dma[3]);
			ar(m_ctx.apu);
			ar(m_ctx.scheduler);
		}

		template <typename Ar>
		void load(Ar& ar) {
			ar(m_ctx.processor);
			ar(m_ctx.bus);
			ar(m_ctx.pack);
			ar(m_ctx.ppu);
			ar(*m_ctx.int_controller);
			ar(m_ctx.keypad);
			ar(m_ctx.timers);
			ar(*m_ctx.all_dma[0]);
			ar(*m_ctx.all_dma[1]);
			ar(*m_ctx.all_dma[2]);
			ar(*m_ctx.all_dma[3]);
			ar(m_ctx.apu);
			ar(m_ctx.scheduler);
		}

	private :
		Emulator();

		bool LoadFromCurrentHistoryPosition();

	private :
		EmulatorContext m_ctx;

		std::string m_bios_loc;

		common::u32 m_rewind_buf_size;
		common::u32 m_rewind_pos;
		std::deque<std::string> m_rewind_buf;

		std::string m_reset_state;
		bool m_is_init;
	};
}