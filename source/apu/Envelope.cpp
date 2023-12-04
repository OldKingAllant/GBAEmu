#include "../../apu/Envelope.hpp"

namespace GBA::apu {
	/*
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
	*/

	Envelope::Envelope() :
		m_last_update_timestamp{},
		m_curr_vol{}, m_shadow{},
		m_step{}, m_dir{}, m_init{}
	{}

	void Restart();

	u32 GetCurrVolume() const;

	void SetStepTime(u8 step);
	void SetDirection(bool dir);
	void SetInitVol(u8 init);

	bool Envelope::IsDacOn() const {
		return !(!m_init && !m_dir);
	}

	void Update(uint64_t timestamp);
}