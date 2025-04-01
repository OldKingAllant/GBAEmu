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
#include "RetroAchievements.hpp"

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

	using namespace common;

	class Emulator {
	public :
		Emulator(std::string_view rom_location, std::string_view bios_location);
		Emulator(std::string_view bios_location);

		EmulatorContext& GetContext() {
			return m_ctx;
		}

		void EmulateFor(u32 num_instructions);
		void RunTillVblank();

		void UseBIOS();
		void SkipBios();

		bool LoadRom(std::string_view loc);
		void Init();

		inline bool IsInit() const {
			return m_is_init;
		}

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

		void SetRewindBufferSize(u32 buf_size) {
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

		inline void SetHleEnable(bool hle_enable) {
			m_ctx.processor.GetContext()
				.m_hle_enable = hle_enable;
		}

		inline bool IsHleEnabled() const {
			return m_ctx.processor.GetContext()
				.m_hle_enable;
		}

		inline void SetLogHleEnable(bool log_enable) {
			m_log_hle = log_enable;
		}

		inline bool IsHleLogEnabled() const {
			return m_log_hle;
		}

		////////////////////////

		inline void SetFastmemEnable(bool enable_fastmem, bool precise_bios, bool precise_ppu) {
			m_ctx.bus.SetFastmemEnable(enable_fastmem, precise_bios, precise_ppu);
		}

		inline bool IsFastmemEnabled() const {
			return m_ctx.bus.IsFastmemEnabled();
		}

		////////////////////////

		inline void RunThreadedRender() {
			m_ctx.ppu.EnableThreadedRender();
		}

		inline bool IsThreadedRenderingEnabled() const {
			return m_ctx.ppu.IsThreadedRenderingEnabled();
		}

		inline void SetEnableScanlineDiffThreshold(bool enable_threshold) {
			m_ctx.ppu.SetEnableDiffThreshold(enable_threshold);
		}

		inline void SetScanlineDiffThreshold(u8 diff_threshold) {
			m_ctx.ppu.SetDiffThreshold(diff_threshold);
		}

		inline bool IsScanlineDiffThresholdEnabled() const {
			return m_ctx.ppu.IsDiffThresholdEnabled();
		}

		inline u8 GetScanlineDiffThreshold() const {
			return m_ctx.ppu.GetDiffThreshold();
		}

		///////////////////////

		inline void EnableCachedInterpreter() {
			m_ctx.processor.EnableCachedInterpreter();
		}

		inline void EnableWaitloopDetection() {
			m_ctx.processor.EnableWaitloopDetection();
		}

		inline void SetCachedInterpreterPageSize(u32 region_size) {
			m_ctx.processor.SetInterpreterPageSize(region_size);
		}

		inline void SetCachedInterpreterBlockSize(u32 max_block_size) {
			m_ctx.processor.SetInterpreterBlockSize(max_block_size);
		}

		///////////////////////

		void EnableAchievements(std::string const& credentials_path);

		inline bool GetAchievementsEnabled() const {
			return m_enable_ra;
		}

		inline RetroAchievements& GetRetroAchievements() {
			_ASSERT(m_enable_ra);
			return *m_ra.get();
		}

		void RetroAchievementsClientIdle();

		///////////////////////

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
		void NextEvent();
		void ProcessHooks();

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
		bool m_log_hle;

		bool m_enable_ra;
		std::unique_ptr<RetroAchievements> m_ra;
	};
}