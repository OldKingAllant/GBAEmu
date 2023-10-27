#pragma once

#include "../common/Defs.hpp"
#include "DMAFireType.hpp"

namespace GBA::memory {
	class Bus;
	class MMIO;
	class InterruptController;

	class DMA {
	public :
		DMA(Bus* bus, common::u8 dma_id, InterruptController* int_controller);

		void SetMMIO(MMIO* mmio);

		bool IsEnabled() const;

		void Step();

		void ResetState();
		void Repeat();

		void TriggerDMA(DMAFireType trigger_type);

	private :
		Bus* m_bus;
		InterruptController* m_int_controller;
		common::u8 m_id;

		common::u32 m_source_address;
		common::u32 m_dest_address;
		common::u16 m_word_count;
		common::u16 m_control;

		common::u32 m_curr_address;
		common::u32 m_curr_word_count;
		common::u32 m_curr_word_sz;
		common::u32 m_curr_source;
		common::u32 m_curr_dest;
		common::u32 m_orig_dad;
		common::i32 m_sad_inc;
		common::i32 m_dad_inc;
	};
}