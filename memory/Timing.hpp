#pragma once

#include "../common/Defs.hpp"
#include "../common/BitManip.hpp"

namespace GBA::memory {
	using namespace common;

	enum class MEMORY_RANGE : u8 {
		BIOS = 0x00,
		EWRAM = 0x02,
		IWRAM = 0x03,
		IO = 0x04,
		PAL = 0x05,
		VRAM = 0x06,
		OAM = 0x07,
		ROM_REG_1 = 0x08,
		ROM_REG_2 = 0x0A,
		ROM_REG_3 = 0x0C,
		SRAM = 0x0E
	};

	enum class Access {
		NonSeq,
		Seq
	};

	class TimeManager {
	public :
		Access access;

		TimeManager();

		u32 PopCycles() {
			u32 temp = m_curr_cycles;
			m_curr_cycles = 0;
			return temp;
		}

		u32 GetCycles() {
			return m_curr_cycles;
		}
		
		template <MEMORY_RANGE range, unsigned Size>
		u32 PushCycles() {
			constexpr u32 multiplier = []() {
				static_assert(Size == 1 || 
					Size == 2 || Size == 4);

				if constexpr (Size == 1 || Size == 2)
					return 1;
				else
					return 2;
			}();

			u32 original_value = m_curr_cycles;

			if constexpr (range == MEMORY_RANGE::EWRAM)
				m_curr_cycles += 3 * multiplier;
			else if constexpr (range == MEMORY_RANGE::SRAM)
				m_curr_cycles += m_wait_config.sram * multiplier + 1;
			else if constexpr (range == MEMORY_RANGE::ROM_REG_1) {
				if constexpr (multiplier == 1)
					m_curr_cycles += m_wait_config.rom0[(u8)access] + 1;
				else
					m_curr_cycles += m_wait_config.rom0[(u8)access]
					+ m_wait_config.rom0[1] + 2;
			}
			else if constexpr (range == MEMORY_RANGE::ROM_REG_2) {
				if constexpr (multiplier == 1)
					m_curr_cycles += m_wait_config.rom1[(u8)access] + 1;
				else
					m_curr_cycles += m_wait_config.rom1[(u8)access]
					+ m_wait_config.rom1[1] + 2;
			}
			else if constexpr (range == MEMORY_RANGE::ROM_REG_3) {
				if constexpr (multiplier == 1)
					m_curr_cycles += m_wait_config.rom2[(u8)access] + 1;
				else
					m_curr_cycles += m_wait_config.rom2[(u8)access]
					+ m_wait_config.rom2[1] + 2;
			}
			else
				m_curr_cycles += 1;

			return m_curr_cycles - original_value;
		}

		template <MEMORY_RANGE range, unsigned Size>
		u32 PushCycles(Access acc) {
			constexpr u32 multiplier = []() {
				static_assert(Size == 1 ||
					Size == 2 || Size == 4);

				if constexpr (Size == 1 || Size == 2)
					return 1;
				else
					return 2;
			}();

			u32 original_value = m_curr_cycles;

			if constexpr (range == MEMORY_RANGE::EWRAM)
				m_curr_cycles += 3 * multiplier;
			else if constexpr (range == MEMORY_RANGE::SRAM)
				m_curr_cycles += m_wait_config.sram * multiplier + 1;
			else if constexpr (range == MEMORY_RANGE::ROM_REG_1) {
				if constexpr (multiplier == 1)
					m_curr_cycles += m_wait_config.rom0[(u8)acc] + 1;
				else
					m_curr_cycles += m_wait_config.rom0[(u8)acc]
					+ m_wait_config.rom0[1] + 2;
			}
			else if constexpr (range == MEMORY_RANGE::ROM_REG_2) {
				if constexpr (multiplier == 1)
					m_curr_cycles += m_wait_config.rom1[(u8)acc] + 1;
				else
					m_curr_cycles += m_wait_config.rom1[(u8)acc]
					+ m_wait_config.rom1[1] + 2;
			}
			else if constexpr (range == MEMORY_RANGE::ROM_REG_3) {
				if constexpr (multiplier == 1)
					m_curr_cycles += m_wait_config.rom2[(u8)acc] + 1;
				else
					m_curr_cycles += m_wait_config.rom2[(u8)acc]
					+ m_wait_config.rom2[1] + 2;
			}
			else
				m_curr_cycles += 1;

			return m_curr_cycles - original_value;
		}

		void PushInternalCycles(u32 count) {
			m_curr_cycles += count;
		}

		void UpdateWaitstate(u32 reg);

		u32 ReadConfig() const {
			return m_config_raw;
		}

		static constexpr u32 ROM_NONSEQ[] = {
			4, 3, 2, 8
		};

		static constexpr u32 ROM_SEQ[][2] = {
			{ 2, 1 },
			{ 4, 1 },
			{ 8, 1 }
		};

		u32 GetSequentialAccess(MEMORY_RANGE region) {
			switch (region)
			{
			case GBA::memory::MEMORY_RANGE::ROM_REG_1:
				return m_wait_config.rom0[1];
				break;
			case GBA::memory::MEMORY_RANGE::ROM_REG_2:
				return m_wait_config.rom1[1];
				break;
			case GBA::memory::MEMORY_RANGE::ROM_REG_3:
				return m_wait_config.rom2[1];
				break;
			case GBA::memory::MEMORY_RANGE::SRAM:
				return m_wait_config.sram;
				break;
			default:
				break;
			}

			return 1;
		}

	public :
		u32 m_config_raw;

	private :
		u32 m_curr_cycles;

		struct {
			u32 sram;
			//0 -> NonSeq, 1 -> Seq
			u32 rom0[2];
			u32 rom1[2];
			u32 rom2[2];
		} m_wait_config;
	};
}