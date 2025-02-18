#pragma once

#include "../common/Defs.hpp"

namespace GBA::apu {
	using namespace common;

	class Envelope {
	public:
		Envelope();

		void Restart();

		u32 GetCurrVolume() const;

		void SetStepTime(u8 step);
		void SetDirection(bool dir);
		void SetInitVol(u8 init);

		bool IsDacOn() const;

		void Update();

		template <typename Ar>
		void save(Ar& ar) const {
			ar(m_ticks);
			ar(m_curr_vol);
			ar(m_shadow.step);
			ar(m_shadow.dir);
			ar(m_shadow.init);
			ar(m_step);
			ar(m_dir);
			ar(m_init);
		}

		template <typename Ar>
		void load(Ar& ar) {
			ar(m_ticks);
			ar(m_curr_vol);
			ar(m_shadow.step);
			ar(m_shadow.dir);
			ar(m_shadow.init);
			ar(m_step);
			ar(m_dir);
			ar(m_init);
		}

	private:
		uint64_t m_ticks;

		u32 m_curr_vol;
		
		struct {
			u8 step;
			bool dir;
			u8 init;
		} m_shadow;

		u8 m_step;
		bool m_dir;
		u8 m_init;
	};
}