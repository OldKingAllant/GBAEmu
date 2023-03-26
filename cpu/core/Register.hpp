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
			return m_banks[m_curr_mode].array[id];
		}

		void SetReg(u8 id, u32 value) {
			m_banks[m_curr_mode].array[id] = value;
		}

		u32 GetReg(Mode mode, u8 id) const {
			return m_banks[GetModeFromID(mode)].array[id];
		}

		void SetReg(Mode mode, u8 id, u32 value) {
			m_banks[GetModeFromID(mode)].array[id] = value;
		}

		void AddOffset(u8 id, i32 value) {
			m_banks[m_curr_mode].array[id] =
				static_cast<i32>(m_banks[m_curr_mode].array[id]) + value;
		}

		void AddOffset(Mode mode, u8 id, i32 value) {
			m_banks[GetModeFromID(mode)].array[id] =
				static_cast<i32>(m_banks[GetModeFromID(mode)].array[id]) + value;
		}

		void SwitchMode(Mode new_mode);

	private :
		u8 m_curr_mode;
		Bank m_banks[6];
	};
}

#endif