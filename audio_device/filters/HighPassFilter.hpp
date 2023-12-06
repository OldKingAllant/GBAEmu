#pragma once

#include <cstdint>

namespace GBA::audio {
	template <typename SampleType>
	class HighPassFilter {
	public:
		constexpr HighPassFilter(uint32_t sample_rate, uint32_t cutoff) :
			m_cutoff{cutoff}, m_dt{0.0}, m_rc{0.0}, m_alpha{0.0},
			m_last_sample_in{}, m_last_sample_out{}
		{
			m_dt = (double)1 / sample_rate;
			m_rc = (double)1 / (TWO_PI * cutoff);
			m_alpha = (double)1 / (TWO_PI * m_dt * cutoff + 1);
		}

		constexpr SampleType Filter(SampleType const& x) noexcept {
			SampleType y = (SampleType)(m_alpha * m_last_sample_out + m_alpha * (x - m_last_sample_in));
			m_last_sample_in = x;
			m_last_sample_out = y;
			return y;
		}

		static constexpr double TWO_PI = 2 * 3.14159;

	private:
		uint32_t m_cutoff;
		double m_dt;
		double m_rc;
		double m_alpha;
		SampleType m_last_sample_in;
		SampleType m_last_sample_out;
	};
}