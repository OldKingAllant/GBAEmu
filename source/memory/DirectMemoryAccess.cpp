#include "../../memory/DirectMemoryAccess.hpp"
#include "../../memory/MMIO.hpp"
#include "../../memory/Bus.hpp"
#include "../../memory/InterruptController.hpp"

#include "../../common/Error.hpp"
#include "../../common/Logger.hpp"

namespace GBA::memory {
	using namespace common;

	constexpr u16 DMA_SOURCES[] = {
		0xB0, 0xBC, 0xC8, 0xD4
	};

	constexpr u16 DMA_DESTS[] = {
		0xB4, 0xC0, 0xCC, 0xD8
	};

	constexpr u16 DMA_WCOUNTS[] = {
		0xB8, 0xC4, 0xD0, 0xDC
	};

	constexpr u16 DMA_CONTROLS[] = {
		0xBA, 0xC6, 0xD2, 0xDE
	};

	constexpr u32 DMA_SOURCE_MASK[] = {
		0x7FFFFFF, 0xFFFFFFF, 0xFFFFFFF, 0xFFFFFFF
	};

	constexpr u32 DMA_DEST_MASK[] = {
		0x7FFFFFF, 0x7FFFFFF, 0x7FFFFFF, 0xFFFFFFF
	};

	constexpr u16 DMA_WCOUNT_MASK[] = {
		0x3FFF, 0x3FFF, 0x3FFF, 0xFFFF
	};

	DMA::DMA(Bus* bus, u8 dma_id, InterruptController* int_controller) :
		m_bus(bus), m_int_controller(int_controller),
		m_id(dma_id), 
		m_source_address{}, m_dest_address{},
		m_word_count{}, m_control{}, 
		m_curr_address{}, m_curr_word_count{},
		m_curr_word_sz{}, m_curr_source{},
		m_curr_dest{}, m_orig_dad{},
		m_sad_inc{}, m_dad_inc{}
	{}

	void DMA::SetMMIO(MMIO* mmio) {
		mmio->AddRegister<u32>(DMA_SOURCES[m_id], false, true,
			reinterpret_cast<u8*>(&m_source_address), DMA_SOURCE_MASK[m_id]);

		mmio->AddRegister<u32>(DMA_DESTS[m_id], false, true,
			reinterpret_cast<u8*>(&m_dest_address), DMA_DEST_MASK[m_id]);

		mmio->AddRegister<u16>(DMA_WCOUNTS[m_id], false, true,
			reinterpret_cast<u8*>(&m_word_count), DMA_WCOUNT_MASK[m_id]);

		mmio->AddRegister<u16>(DMA_CONTROLS[m_id], true, true,
			reinterpret_cast<u8*>(&m_control), 0xFFFF, 
			[this](u8 val, u16 address) {
				address -= DMA_CONTROLS[m_id];

				bool enable_before = CHECK_BIT(m_control, 15);

				m_control &= ~(0xFF << (address * 8));
				m_control |= ((u16)val << (address * 8));

				bool enable_after = CHECK_BIT(m_control, 15);

				if (!enable_before && enable_after) {
					u8 timing = (m_control >> 12) & 3;

					ResetState();

					if (!timing) {
						m_bus->AddActiveDma(m_id);
					}
				}
			});
	}

	void DMA::ResetState() {
		if (m_word_count) {
			m_curr_word_count = m_word_count;
		}
		else {
			m_curr_word_count = m_id == 3 ? 0x10000 : 0x4000;
		}
		//m_curr_word_count = m_word_count ? m_word_count : 0xFFFF;
		m_curr_address = 0;
		m_curr_word_sz = CHECK_BIT(m_control, 10) ? 4 : 2;
		m_curr_source = m_source_address;
		m_curr_dest = m_dest_address;
		m_orig_dad = m_curr_dest;

		u8 dest_control = (m_control >> 5) & 0x3;
		u8 source_control = (m_control >> 7) & 0x3;

		switch (dest_control)
		{
		case 0:
			m_dad_inc = m_curr_word_sz;
			break;
		case 1:
			m_dad_inc = -(i32)m_curr_word_sz;
			break;
		case 2:
			m_dad_inc = 0;
			break;
		case 3:
			m_dad_inc = m_curr_word_sz;
			break;
		default:
			error::Unreachable();
			break;
		}

		switch (source_control)
		{
		case 0:
			m_sad_inc = m_curr_word_sz;
			break;
		case 1:
			m_sad_inc = -(i32)m_curr_word_sz;
			break;
		case 2:
			m_sad_inc = 0;
			break;
		case 3:
			m_sad_inc = m_curr_word_sz;
			break;
		default:
			error::Unreachable();
			break;
		}
	}

	void DMA::Repeat() {
		m_curr_address = 0;

		if (m_word_count) {
			m_curr_word_count = m_word_count;
		}
		else {
			m_curr_word_count = m_id == 3 ? 0x10000 : 0x4000;
		}

		u8 dest_control = (m_control >> 5) & 0x3;

		if (dest_control == 3)
			m_curr_dest = m_orig_dad;

		u8 timing = (m_control >> 12) & 3;

		if ((m_id == 1 || m_id == 2) && timing == 0x3) {
			m_dad_inc = 0;
			m_curr_word_count = 4;
			m_curr_word_sz = 4;
		}
	}

	bool DMA::IsEnabled() const {
		return (m_control >> 15) & 1;
	}

	void DMA::Step() {
		m_bus->m_time.access = !m_curr_address ? Access::NonSeq : Access::Seq;

		bool repeat = CHECK_BIT(m_control, 9);
		bool issue_irq = CHECK_BIT(m_control, 14);

		if (m_curr_word_count) {
			if (m_curr_word_sz == 2) {
				u16 read = m_bus->Read<u16>(m_curr_source);
				m_bus->Write<u16>(m_curr_dest, read);
			}
			else {
				u32 read = m_bus->Read<u32>(m_curr_source);
				m_bus->Write<u32>(m_curr_dest, read);
			}

			m_curr_address += m_curr_word_sz;
			m_curr_source += m_sad_inc;
			m_curr_dest += m_dad_inc;
			m_curr_word_count--;
		}

		if (!m_curr_word_count) {
			if(!repeat)
				m_control = CLEAR_BIT(m_control, 15);

			m_bus->RemoveActiveDma();

			u8 dest_control = (m_control >> 5) & 0x3;

			if(dest_control == 3)
				m_curr_dest = m_orig_dad;

			if (issue_irq) {
				m_int_controller->RequestInterrupt(
					(InterruptType)((u16)InterruptType::DMA0 << m_id)
				);
			}
		}
	}

	void DMA::TriggerDMA(DMAFireType trigger_type) {
		if (!CHECK_BIT(m_control, 15))
			return;

		bool must_run = false;

		u8 timing = (m_control >> 12) & 3;

		if (!timing)
			return;

		switch (trigger_type)
		{
		case GBA::memory::DMAFireType::HBLANK:
			must_run = timing == 0x2;
			break;
		case GBA::memory::DMAFireType::VBLANK:
			must_run = timing == 0x1;
			break;
		case GBA::memory::DMAFireType::FIFO_A:
			must_run = timing == 0x3 && m_dest_address == 0x40000A0;
			break;
		case GBA::memory::DMAFireType::FIFO_B:
			must_run = timing == 0x3 && m_dest_address == 0x40000A4;
			break;
		case GBA::memory::DMAFireType::CAPTURE:
			error::DebugBreak();
			break;
		default:
			error::Unreachable();
			break;
		}

		if (must_run) {
			Repeat();
			m_bus->AddActiveDma(m_id);
		}
	}
}