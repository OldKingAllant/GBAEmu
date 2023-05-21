#include "../../memory/MMIO.hpp"

namespace GBA::memory {
	using namespace common;

	MMIO::MMIO() :
		m_registers{} {

		for (auto& reg : m_registers) {
			reg.pointer = nullptr;
		}
	}

	u8 MMIO::DebuggerReadIO(u16 offset) {
		auto& reg = m_registers[offset];

		if (!reg.pointer)
			return 0xFF;

		return *(reg.pointer);
	}
}