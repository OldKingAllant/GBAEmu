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

		void Update(uint64_t timestamp);

	private:
		uint64_t m_last_update_timestamp;

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