#pragma once

#include "CPUContext.hpp"

namespace GBA::cpu {
	class ARM7TDI {
	public :
		ARM7TDI(memory::Bus* bus);

		void SkipBios();

		u8 Step();
		
		static u32 GetExceptVector(ExceptionCode const& exc);
		static Mode GetModeFromExcept(ExceptionCode const& exc);

		CPUContext& GetContext() {
			return m_ctx;
		}

	private :
		CPUContext m_ctx;
		memory::Bus* m_bus;
	};
}