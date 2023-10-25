#include "../../memory/DirectMemoryAccess.hpp"
#include "../../memory/MMIO.hpp"
#include "../../memory/Bus.hpp"

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

	DMA::DMA(Bus* bus, u8 dma_id) :
		m_bus(bus), m_id(dma_id), 
		m_source_address{}, m_dest_address{},
		m_word_count{}, m_control{}, 
		m_curr_address{} {}

	void DMA::SetMMIO(MMIO* mmio) {
		mmio->AddRegister<u32>(DMA_SOURCES[m_id], false, true,
			reinterpret_cast<u8*>(&m_source_address), DMA_SOURCE_MASK[m_id]);

		mmio->AddRegister<u32>(DMA_DESTS[m_id], false, true,
			reinterpret_cast<u8*>(&m_dest_address), DMA_DEST_MASK[m_id]);

		mmio->AddRegister<u16>(DMA_WCOUNTS[m_id], false, true,
			reinterpret_cast<u8*>(&m_word_count), DMA_WCOUNT_MASK[m_id]);

		mmio->AddRegister<u16>(DMA_CONTROLS[m_id], true, true,
			reinterpret_cast<u8*>(&m_control), 0xFFFF);
	}

	bool DMA::IsEnabled() const {
		return (m_control >> 15) & 1;
	}

	void DMA::Step() {}
}