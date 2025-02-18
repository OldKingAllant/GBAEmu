#pragma once

#include "CPUContext.hpp"

namespace GBA::memory {
	class InterruptController;
}

namespace GBA::cpu {
	class ARM7TDI {
	public :
		ARM7TDI();

		void AttachBus(memory::Bus* bus);

		void SkipBios();

		u8 Step();
		
		static u32 GetExceptVector(ExceptionCode const& exc);
		static Mode GetModeFromExcept(ExceptionCode const& exc);

		CPUContext& GetContext() {
			return m_ctx;
		}

		void SetInterruptControl(memory::InterruptController* int_controller);

		inline void SetHalted() {
			m_halt = true;
		}

		template <typename Ar>
		void save(Ar& ar) const {
			ar(m_halt);
			ar(m_ctx.m_regs);
			ar(m_ctx.m_cpsr);
			ar(m_ctx.m_spsr);
			ar(m_ctx.m_pipeline);
			ar(m_ctx.m_old_pc);
		}

		template <typename Ar>
		void load(Ar& ar) {
			ar(m_halt);
			ar(m_ctx.m_regs);
			ar(m_ctx.m_cpsr);
			ar(m_ctx.m_spsr);
			ar(m_ctx.m_pipeline);
			ar(m_ctx.m_old_pc);
		}

	private :
		bool CheckIRQ();

	private :
		CPUContext m_ctx;
		memory::Bus* m_bus;
		memory::InterruptController* m_int_controller;
		bool m_halt;
	};
}