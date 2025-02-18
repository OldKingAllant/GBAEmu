#pragma once

#include "../common/Defs.hpp"

namespace GBA::apu {
	using namespace common;

	class LenCounter {
	public:
		LenCounter();

		void SetLen(u8 len);

		bool Update();

		template <typename Ar>
		void save(Ar& ar) const {
			ar(m_curr_len);
		}

		template <typename Ar>
		void load(Ar& ar) {
			ar(m_curr_len);
		}

	private:
		u8 m_curr_len;
	};
}