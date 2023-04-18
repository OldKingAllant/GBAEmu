#include "../../../cpu/core/Register.hpp"

#include <memory>

namespace GBA::cpu {
	/*u8 m_curr_mode;
		Bank m_curr_regs;
		u32 m_commonly_shared[5];
		u32 m_banked_regs[6][7];

		static constexpr inline u8 BANK_START[] = {
			5, 0, 5, 5, 5, 5
		};
	*/

	RegisterManager::RegisterManager() :
		m_curr_mode{}, m_curr_regs {}, 
		m_commonly_shared{}, m_banked_regs{} {}

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

		//Save/retrieve banked registers
		u8 banked_index_curr_mode = BANK_START[m_curr_mode];
		u8 banked_index_new_mode = BANK_START[mode];

		std::copy(m_curr_regs.array + banked_index_curr_mode + 8,
			m_curr_regs.array + 15, m_banked_regs[m_curr_mode] + banked_index_curr_mode);

		if (new_mode == Mode::FIQ) {
			//Save shared regs
			std::copy(m_curr_regs.array + 8, m_curr_regs.array + 13, m_commonly_shared);
		}

		if (m_curr_mode == GetModeFromID(Mode::FIQ)) {
			//Retrieve shared regs
			std::copy(m_commonly_shared, m_commonly_shared + 5, m_curr_regs.array + 8);
		}

		std::copy(m_banked_regs[mode] + banked_index_new_mode, m_banked_regs[mode] + 7,
			m_curr_regs.array + 8 + banked_index_new_mode);

		

		m_curr_mode = mode;
	}
}