#include "../../apu/LenCounter.hpp"

namespace GBA::apu {
	LenCounter::LenCounter() : m_curr_len{} {}

	void LenCounter::SetLen(u8 len) {
		m_curr_len = 64 - len;
	}

	bool LenCounter::Update() {
		if (m_curr_len == 64)
			return false;

		m_curr_len++;

		return m_curr_len != 64;
	}
}