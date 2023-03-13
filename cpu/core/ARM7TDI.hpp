#pragma once

#include "./CPSR.hpp"
#include "./Register.hpp"
#include "./Pipeline.hpp"

namespace GBA::cpu {
	class ARM7TDI {
	public :
		ARM7TDI(memory::Bus* bus);

		void EnterException(ExceptionCode const& exc);
		void RestorePreviousMode(u32 old_pc);

		u8 Step();
		
		static u32 GetExceptVector(ExceptionCode const& exc);
		static Mode GetModeFromExcept(ExceptionCode const& exc);

	private :
		RegisterManager m_regs;
		CPSR m_cpsr;
		CPSR m_spsr[5];
		Pipeline m_pipeline;
	};
}