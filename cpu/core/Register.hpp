#ifndef REGS
#define REGS

#include "../../common/Defs.hpp"
#include "../ProcessorDefs.h"

namespace GBA::cpu {
	using namespace common;

	union Bank {
		struct {
			u32 r0;
			u32 r1;
			u32 r2;
			u32 r3;
			u32 r4;
			u32 r5;
			u32 r6;
			u32 r7;
			u32 r8;
			u32 r9;
			u32 r10;
			u32 r11;
			u32 r12;
			u32 r13;
			u32 r14;
			u32 r15;
		};

		u32 array[16];
	};

	class RegisterManager {
	public :
		RegisterManager();

		u32 GetReg(u8 id) const {
			return m_curr_regs.array[id];
		}

		void SetReg(u8 id, u32 value) {
			m_curr_regs.array[id] = value;
		}

		u32 GetReg(Mode mode, u8 id) const {
			u8 mode_id = GetModeFromID(mode);

			if (id < 8 || id == 15 || mode_id == m_curr_mode)
				return m_curr_regs.array[id];

			if(id - 8 >= BANK_START[mode_id])
				return m_banked_regs[mode_id][id - 8];

			if (m_curr_mode != GetModeFromID(Mode::FIQ))
				return m_curr_regs.array[id];

			return m_commonly_shared[id - 8];
		}

		void SetReg(Mode mode, u8 id, u32 value) {
			u8 mode_id = GetModeFromID(mode);

			if (id < 8 || id == 15 || mode_id == m_curr_mode) {
				m_curr_regs.array[id] = value; 
				return;
			}

			if (id - 8 >= BANK_START[mode_id]) {
				m_banked_regs[mode_id][id - 8] = value;
				return;
			}

			if (m_curr_mode != GetModeFromID(Mode::FIQ)) {
				m_curr_regs.array[id] = value;
				return;
			}

			m_commonly_shared[id - 8] = value;
		}

		void AddOffset(u8 id, i32 value) {
			m_curr_regs.array[id] =
				static_cast<i32>(m_curr_regs.array[id]) + value;
		}

		void AddOffset(Mode mode, u8 id, i32 value) {
			u32 base_value = GetReg(mode, id);

			base_value =
				static_cast<i32>(base_value) + value;

			SetReg(mode, id, base_value);
		}

		void SwitchMode(Mode new_mode);

		static constexpr inline u8 BANK_START[] = {
			5, 0, 5, 5, 5, 5
		};

	private :
		u8 m_curr_mode;
		Bank m_curr_regs;
		u32 m_commonly_shared[5];
		u32 m_banked_regs[6][7];
	};
}

#endif