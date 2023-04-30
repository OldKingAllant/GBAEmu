#pragma once

#include "./CPSR.hpp"
#include "./Register.hpp"
#include "./Pipeline.hpp"

namespace GBA::cpu {
	struct CPUContext {
		RegisterManager m_regs;
		CPSR m_cpsr;
		CPSR m_spsr[5];
		Pipeline m_pipeline;

		void EnterException(ExceptionCode exc, u8 pc_offset);
		void RestorePreviousMode(u32 old_pc);
		void ChangeMode(Mode new_mode);
	};
}