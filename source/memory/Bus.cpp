#include "../../memory/Bus.hpp"
#include "../../gamepack/GamePack.hpp"

namespace GBA::memory {
	Bus::Bus() :
		m_pack(nullptr) {}

	void Bus::ConnectGamepack(gamepack::GamePack* pack) {
		m_pack = pack;
	}

	template <>
	u8 Bus::Read(u32 address) const {
		MEMORY_RANGE region = (MEMORY_RANGE)(address >> 24);
		u32 addr_low = address & 0x00FFFFFF;

		if((u8)region >= NUM_REGIONS)
			return static_cast<u8>(~0);

		if(addr_low & ~REGIONS_LEN[(u8)region])
			return static_cast<u8>(~0);

		switch (region) {
		case MEMORY_RANGE::BIOS:
			return 0x00;

		case MEMORY_RANGE::EWRAM:
			return 0x00;

		case MEMORY_RANGE::IWRAM:
			return 0x00;

		case MEMORY_RANGE::IO:
			return 0x00;

		case MEMORY_RANGE::PAL:
			return 0x00;

		case MEMORY_RANGE::VRAM:
			return 0x00;

		case MEMORY_RANGE::OAM:
			return 0x00;

		case MEMORY_RANGE::ROM_REG_1:
		case MEMORY_RANGE::ROM_REG_2:
		case MEMORY_RANGE::ROM_REG_3:
			return (u8)(m_pack->Read(addr_low));

		case MEMORY_RANGE::SRAM:
			return 0x00;
		}

		return static_cast<u8>(~0);
	}

	template <>
	u16 Bus::Read(u32 address) const {
		MEMORY_RANGE region = (MEMORY_RANGE)(address >> 24);
		u32 addr_low = address & 0x00FFFFFF;
		addr_low &= ~1;

		if ((u8)region >= NUM_REGIONS)
			return static_cast<u16>(~0);

		if (addr_low & ~REGIONS_LEN[(u8)region])
			return static_cast<u16>(~0);

		switch (region) {
		case MEMORY_RANGE::BIOS:
			return 0x00;

		case MEMORY_RANGE::EWRAM:
			return 0x00;

		case MEMORY_RANGE::IWRAM:
			return 0x00;

		case MEMORY_RANGE::IO:
			return 0x00;

		case MEMORY_RANGE::PAL:
			return 0x00;

		case MEMORY_RANGE::VRAM:
			return 0x00;

		case MEMORY_RANGE::OAM:
			return 0x00;

		case MEMORY_RANGE::ROM_REG_1:
		case MEMORY_RANGE::ROM_REG_2:
		case MEMORY_RANGE::ROM_REG_3:
			return m_pack->Read(addr_low);

		case MEMORY_RANGE::SRAM:
			return 0x00;
		}

		return static_cast<u16>(~0);
	}

	template <>
	u32 Bus::Read(u32 address) const {
		MEMORY_RANGE region = (MEMORY_RANGE)(address >> 24);
		u32 addr_low = address & 0x00FFFFFF;
		addr_low &= ~3;

		if ((u8)region >= NUM_REGIONS)
			return static_cast<u32>(~0);

		if (addr_low & ~REGIONS_LEN[(u8)region])
			return static_cast<u32>(~0);

		switch (region) {
		case MEMORY_RANGE::BIOS:
			return 0x00;

		case MEMORY_RANGE::EWRAM:
			return 0x00;

		case MEMORY_RANGE::IWRAM:
			return 0x00;

		case MEMORY_RANGE::IO:
			return 0x00;

		case MEMORY_RANGE::PAL:
			return 0x00;

		case MEMORY_RANGE::VRAM:
			return 0x00;

		case MEMORY_RANGE::OAM:
			return 0x00;

		case MEMORY_RANGE::ROM_REG_1:
		case MEMORY_RANGE::ROM_REG_2:
		case MEMORY_RANGE::ROM_REG_3: {
			u32 ret = m_pack->Read(addr_low);
			ret |= (m_pack->Read(addr_low + 2) << 16);
			return ret;
		}

		case MEMORY_RANGE::SRAM:
			return 0x00;
		}

		return static_cast<u32>(~0);
	}

	template <>
	void Bus::Write(u32 address, u8 value) {
		(void)address;
		(void)value;
	}

	template <>
	void Bus::Write(u32 address, u16 value) {
		(void)address;
		(void)value;
	}

	template <>
	void Bus::Write(u32 address, u32 value) {
		(void)address;
		(void)value;
	}
}