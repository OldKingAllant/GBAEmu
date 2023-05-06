#pragma once

#include "../common/Logger.hpp"
#include "../common/BitManip.hpp"
#include "../common/Defs.hpp"

namespace GBA::memory {
	class MMIO;
}

namespace GBA::ppu {
	class PPU {
	public :
		PPU(memory::MMIO* mmio);

		common::u32 ReadRegister32(common::u8 offset) const;
		void WriteRegister32(common::u8 offset, common::u32 value);

		common::u16 ReadRegister16(common::u8 offset) const;
		void WriteRegister16(common::u8 offset, common::u16 value);

		common::u8 ReadRegister8(common::u8 offset) const;
		void WriteRegister8(common::u8 offset, common::u8 value);


	private:
		void InitHandlers(memory::MMIO* mmio);

	private :
		union PPUContext {
			struct {
				common::u16 m_control;
				common::u16 m_green_swap;
				common::u16 m_status;
			};
			
			common::u8 array[0x58];
		};
		
		PPUContext m_ctx;
	};
}