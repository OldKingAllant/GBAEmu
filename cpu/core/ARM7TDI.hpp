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

	private :
		bool CheckIRQ();

	private :
		CPUContext m_ctx;
		memory::Bus* m_bus;
		memory::InterruptController* m_int_controller;
		bool m_halt;
	};
}