#pragma once

#include <string_view>
#include <deque>
#include <vector>
#include <list>
#include <unordered_map>

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

#include "Cheats.hpp"

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

		//////////////////////////

		void StoreState(std::string const& path);
		void LoadState(std::string const& path);

		void SaveResetState();

		//////////////////////////

		void SetRewindEnable(bool enable_rewind);

		inline bool IsRewindEnabled() const {
			return m_enable_rewind;
		}

		inline uint32_t GetCurrentRewindBufferSize() const {
			return uint32_t(m_rewind_buf.size());
		}

		inline uint32_t GetMaxRewindBufferSize() const {
			return m_rewind_buf_size;
		}

		inline void RewindClearBuffer() {
			m_rewind_pos = 0;
			m_rewind_buf.clear();
		}

		bool RewindPush();
		bool RewindBackward();
		bool RewindForward();
		bool RewindPop();

		bool Reset();

		void SetRewindBufferSize(common::u32 buf_size) {
			m_rewind_buf_size = buf_size;
			m_rewind_pos = 0;

			if (m_rewind_buf.size() > m_rewind_buf_size) {
				m_rewind_buf.resize(buf_size);
			}
		}

		/////////////////////////

		inline void EnableHooksGlobal(bool set_enabled) {
			m_enable_hooks = set_enabled;
		}

		bool AddCheat(std::vector<std::string> lines, cheats::CheatType ty, std::string name);
		void RemoveCheat(std::string const& name);
		bool EnableCheat(std::string const& name);
		void DisableCheat(std::string const& name);

		void AddHook(uint32_t pc, std::string const& name);
		void RemoveHook(std::string const& name);

		inline std::unordered_map<std::string, cheats::CheatSet> const& GetCheats() const {
			return m_cheats;
		}

		/////////////////////////

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

		void ProcessCheats();

	private :
		EmulatorContext m_ctx;

		std::string m_bios_loc;

		common::u32 m_rewind_buf_size;
		common::u32 m_rewind_pos;
		std::deque<std::string> m_rewind_buf;
		bool m_enable_rewind;

		std::string m_reset_state;
		bool m_is_init;

		std::unordered_map<std::string, cheats::CheatSet> m_cheats;
		std::list<std::string> m_enabled_cheats;

		std::unordered_multimap<uint32_t, std::string> m_hooks;

		bool m_enable_hooks;
	};
}