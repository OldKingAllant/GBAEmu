#include "../../memory/Timing.hpp"

namespace GBA::memory {
	TimeManager::TimeManager() : 
		m_curr_cycles{0}, 
		m_wait_config{},
		m_config_raw{0}
	{
		UpdateWaitstate(0);
	}

	void TimeManager::UpdateWaitstate(u32 reg) {
		reg &= ~(1 << 15);

		m_config_raw = reg;

		m_wait_config.sram = ROM_NONSEQ[m_config_raw & 0x3];

		m_wait_config.rom0[0] = ROM_NONSEQ[(m_config_raw >> 2) & 0x3];
		m_wait_config.rom1[0] = ROM_NONSEQ[(m_config_raw >> 5) & 0x3];
		m_wait_config.rom2[0] = ROM_NONSEQ[(m_config_raw >> 8) & 0x3];

		m_wait_config.rom0[1] = ROM_SEQ[0][(m_config_raw >> 4) & 0x1];
		m_wait_config.rom1[1] = ROM_SEQ[1][(m_config_raw >> 7) & 0x1];
		m_wait_config.rom2[1] = ROM_SEQ[2][(m_config_raw >> 10) & 0x1];
	}
}