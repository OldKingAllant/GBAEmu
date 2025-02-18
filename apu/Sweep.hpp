#pragma once

#include "../common/Defs.hpp"

namespace GBA::apu {
	using namespace common;

	class Sweep {
	public :
		Sweep();

		union {
			struct {
				u8 shift : 3;
				bool direction : 1;
				u8 time : 3;
			};

			u16 raw;
		} m_control;

		u32 Update();
		void Restart(u32 freq);

		u32 GetFreq() const;

		static constexpr uint64_t SWEEP_CYCLES = 131'093;

		template <typename Ar>
		void save(Ar& ar) const {
			ar(m_control.raw);
			ar(m_ticks);
			ar(m_freq);
		}

		template <typename Ar>
		void load(Ar& ar) {
			ar(m_control.raw);
			ar(m_ticks);
			ar(m_freq);
		}

	private:
		uint64_t m_ticks;
		u32 m_freq;
	};
}