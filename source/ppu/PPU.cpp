#include "../../ppu/PPU.hpp"

#include "../../memory/MMIO.hpp"

/*PPU(memory::MMIO* mmio);

		common::u32 ReadRegister32(common::u8 offset) const;
		void WriteRegister32(common::u8 offset, common::u32 value);

		common::u16 ReadRegister16(common::u8 offset) const;
		void WriteRegister16(common::u8 offset, common::u16 value);

		common::u8 ReadRegister8(common::u8 offset) const;
		void WriteRegister8(common::u8 offset, common::u8 value);


	private:
		void InitHandlers(memory::MMIO* mmio);*/

namespace GBA::ppu {
	using namespace common;

	PPU::PPU(memory::MMIO* mmio) : m_ctx{} {
		InitHandlers(mmio);
	}

	void PPU::InitHandlers(memory::MMIO* mmio) {
		for (unsigned i = 0; i < 0x58; i++) {
			mmio->RegisterRead<u8>([]() -> u8 {return 0x0; }, i);
			mmio->RegisterWrite<u8>([](u8 dummy) {}, i);
		}
		
		for (unsigned i = 0; i < 0x58; i += 2) {
			mmio->RegisterRead<u16>([]() -> u16 {return 0x0; }, i);
			mmio->RegisterWrite<u16>([](u16 dummy) {}, i);
		}

		for (unsigned i = 0; i < 0x58; i += 4) {
			mmio->RegisterRead<u32>([]() -> u32 {return 0x0; }, i);
			mmio->RegisterWrite<u32>([](u32 dummy) {}, i);
		}
	}

	common::u32 PPU::ReadRegister32(common::u8 offset) const {
		return 0;
	}

	void PPU::WriteRegister32(common::u8 offset, common::u32 value) {
		//
	}

	common::u16 PPU::ReadRegister16(common::u8 offset) const {
		return 0;
	}

	void PPU::WriteRegister16(common::u8 offset, common::u16 value) {
		//
	}

	common::u8 PPU::ReadRegister8(common::u8 offset) const {
		return 0;
	}

	void PPU::WriteRegister8(common::u8 offset, common::u8 value) {
		//
	}
}