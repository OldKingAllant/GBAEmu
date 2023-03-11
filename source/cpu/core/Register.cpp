#include "../../../cpu/core/Register.hpp"

namespace GBA::cpu {
	RegisterManager::RegisterManager() :
		m_curr_mode(0), m_banks{} {}

	/*
	* Switch current register bank to 
	* the given mode.
	* This is done by copying values when
	* necessary
	*/
	void RegisterManager::SwitchMode(Mode new_mode) {
		u8 mode = GetModeFromID(new_mode); //get an index from the mode ID

		if (mode == m_curr_mode)
			return;

		//Copy first 8 common regs
		for (u8 i = 0; i < 8; i++) {
			m_banks[mode].array[i] =
				m_banks[m_curr_mode].array[i];
		}

		u8 source = 0;
		u8 dest = 0;

		if (new_mode != Mode::FIQ) {
			if (m_curr_mode == 0x01) {
				//The current mode is FIQ
				//No registers can be copied
				//We must retrieve the old values
				source = 0;
				dest = mode;
			}
			else {
				//We can copy shared regs
				source = m_curr_mode;
				dest = mode;
			}
		}
		else {
			//There are no registers to copy in the new bank
			//We must save the current values 
			source = m_curr_mode;
			dest = 0;
		}

		//Copy regs with given config.
		for (u8 i = 8; i < 13; i++) {
			m_banks[dest].array[i] =
				m_banks[source].array[i];
		}

		//The PC is always shared
		m_banks[mode].r15 = m_banks[m_curr_mode].r15;

		m_curr_mode = mode;
	}
}