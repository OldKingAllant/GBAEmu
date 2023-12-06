#pragma once

#include "../common/Defs.hpp"

namespace GBA::apu {
	using namespace common;

	class LenCounter {
	public:
		LenCounter();

		void SetLen(u8 len);

		bool Update();

	private:
		u8 m_curr_len;
	};
}