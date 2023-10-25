#pragma once

#include "../common/Defs.hpp"

namespace GBA::memory {
	class Bus;
	class MMIO;

	class DMA {
	public :
		DMA(Bus* bus, common::u8 dma_id);

		void SetMMIO(MMIO* mmio);

		bool IsEnabled() const;

		void Step();

	private :
		Bus* m_bus;
		common::u8 m_id;

		common::u32 m_source_address;
		common::u32 m_dest_address;
		common::u16 m_word_count;
		common::u16 m_control;

		common::u32 m_curr_address;
	};
}