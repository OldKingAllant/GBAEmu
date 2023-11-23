#include "../../apu/APU.hpp"

#include "../../memory/MMIO.hpp"
#include "../../common/BitManip.hpp"

#include "../../memory/DirectMemoryAccess.hpp"

#include <bit>

namespace GBA::apu {
	APU::APU() :
		m_sample_buffer{nullptr}, m_dma1{nullptr},
		m_dma2{nullptr}, m_buffer_callback{},
		m_curr_samples{}, m_req_samples{}, 
		m_internal_A_buffer{}, m_internal_B_buffer{},
		m_A_pos{}, m_B_pos{}, m_soundcnt_h{},
		m_soundcnt_x{}, m_soundbias{}
	{}

	void APU::SetDma(memory::DMA* _1, memory::DMA* _2) {
		m_dma1 = _1;
		m_dma2 = _2;
	}

	void APU::SetCallback(SampleCallback callback, u32 required_samples) {
		m_buffer_callback = callback;
		m_req_samples = required_samples;

		m_sample_buffer = new i8[required_samples];
	}

	void APU::Clock(u32 num_cycles) {
		(void)num_cycles;
	}

	void APU::SetMMIO(memory::MMIO* mmio) {
		u8* a_buf = std::bit_cast<u8*>((i8*)m_internal_A_buffer);

		mmio->AddRegister<u32>(0xA0, false, true, a_buf, 0xFFFF,
			[this](u8 data, u16 offset) {
				offset -= 0xA0;
				if(m_A_pos < 31)
					m_internal_A_buffer[m_A_pos++] = data;
		});

		u8* b_buf = std::bit_cast<u8*>((i8*)m_internal_A_buffer);

		mmio->AddRegister<u32>(0xA4, false, true, b_buf, 0xFFFF,
			[this](u8 data, u16 offset) {
				offset -= 0xA4;
				if(m_B_pos < 31)
					m_internal_B_buffer[m_B_pos++] = data;
		});

		u8* soundcnt_h = std::bit_cast<u8*>(&m_soundcnt_h);

		mmio->AddRegister<u16>(0x82, true, true, soundcnt_h, 0xFFFF,
			[this](u8 data, u16 offset) {
				offset -= 0x82;

				u8* soundcnt_h = std::bit_cast<u8*>(&m_soundcnt_h);

				soundcnt_h[offset] = data;

				if (m_soundcnt_h.fifo_a_reset) {
					m_soundcnt_h.fifo_a_reset = 0;
					m_A_pos = 0;
				}

				if (m_soundcnt_h.fifo_b_reset) {
					m_soundcnt_h.fifo_b_reset = 0;
					m_B_pos = 0;
				}
			});

		u8* master_control = std::bit_cast<u8*>(&m_soundcnt_x);

		mmio->AddRegister<u8>(0x84, true, true, master_control, 0x80,
			[this](u8 data, u16 pos) {
				m_soundcnt_x.master_en = CHECK_BIT(data, 7);
			});

		u8* bias_cnt = std::bit_cast<u8*>(&m_soundbias);

		mmio->AddRegister<u16>(0x88, true, true, bias_cnt, 0xFFFF);
	}

	void APU::TimerOverflow(u8 id) {
		if (!m_soundcnt_x.master_en)
			return;

		if (m_soundcnt_h.fifo_a_timer_sel == id && m_A_pos) {
			m_A_pos--;

			if (m_A_pos <= 16) {
				m_dma1->TriggerDMA(memory::DMAFireType::FIFO_A);
				m_dma2->TriggerDMA(memory::DMAFireType::FIFO_A);
			}
		}
		
		if (m_soundcnt_h.fifo_b_timer_sel == id && m_B_pos) {
			m_B_pos--;

			if (m_B_pos <= 16) {
				m_dma1->TriggerDMA(memory::DMAFireType::FIFO_B);
				m_dma2->TriggerDMA(memory::DMAFireType::FIFO_B);
			}
		}
	}

	APU::~APU() {
		if (m_sample_buffer)
			delete[] m_sample_buffer;
	}
}