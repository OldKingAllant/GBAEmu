#include "../../memory/MMIO.hpp"

namespace GBA::memory {
	using namespace common;

	MMIO::MMIO() :
		m_registers{} {

		for (auto& reg : m_registers) {
			reg.pointer = nullptr;
		}
	}
}