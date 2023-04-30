#include "../../memory/Bus.hpp"
#include "../../gamepack/GamePack.hpp"

#include "../../common/Logger.hpp"
#include "../../common/Error.hpp"

namespace GBA::memory {
	LOG_CONTEXT(Memory bus);

	Bus::Bus() :
		m_pack(nullptr), m_wram(nullptr),
		m_iwram(nullptr)
	{
		m_wram = new u8[0x3FFFF];
		m_iwram = new u8[0x7FFF];

		std::fill_n(m_wram, 0x3FFFF, 0x00);
		std::fill_n(m_iwram, 0x7FFF, 0x00);
	}

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
			return m_wram[addr_low];

		case MEMORY_RANGE::IWRAM:
			return m_iwram[addr_low];

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
			return reinterpret_cast<u16*>(m_wram)[addr_low / 2];

		case MEMORY_RANGE::IWRAM:
			return reinterpret_cast<u16*>(m_iwram)[addr_low / 2];

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
			return reinterpret_cast<u32*>(m_wram)[addr_low / 4];

		case MEMORY_RANGE::IWRAM:
			return reinterpret_cast<u32*>(m_iwram)[addr_low / 4];

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
		MEMORY_RANGE region = (MEMORY_RANGE)(address >> 24);
		u32 addr_low = address & 0x00FFFFFF;

		if ((u8)region >= NUM_REGIONS)
			return;

		if (addr_low & ~REGIONS_LEN[(u8)region])
			return;

		switch (region) {
		case MEMORY_RANGE::BIOS:
			break;

		case MEMORY_RANGE::EWRAM:
			m_wram[addr_low] = value;
			break;

		case MEMORY_RANGE::IWRAM:
			m_iwram[addr_low] = value;
			break;

		case MEMORY_RANGE::IO:
			break;

		case MEMORY_RANGE::PAL:
			break;

		case MEMORY_RANGE::VRAM:
			break;

		case MEMORY_RANGE::OAM:
			break;

		case MEMORY_RANGE::ROM_REG_1:
		case MEMORY_RANGE::ROM_REG_2:
		case MEMORY_RANGE::ROM_REG_3:
			break;

		case MEMORY_RANGE::SRAM:
			break;
		}
	}

	template <>
	void Bus::Write(u32 address, u16 value) {
		MEMORY_RANGE region = (MEMORY_RANGE)(address >> 24);
		u32 addr_low = address & 0x00FFFFFF;

		if ((u8)region >= NUM_REGIONS)
			return;

		if (addr_low & ~REGIONS_LEN[(u8)region])
			return;

		addr_low &= ~1;

		switch (region) {
		case MEMORY_RANGE::BIOS:
			break;

		case MEMORY_RANGE::EWRAM:
			reinterpret_cast<u16*>(m_wram)[addr_low / 2] = value;
			break;

		case MEMORY_RANGE::IWRAM:
			reinterpret_cast<u16*>(m_iwram)[addr_low / 2] = value;
			break;

		case MEMORY_RANGE::IO:
			break;

		case MEMORY_RANGE::PAL:
			break;

		case MEMORY_RANGE::VRAM:
			break;

		case MEMORY_RANGE::OAM:
			break;

		case MEMORY_RANGE::ROM_REG_1:
		case MEMORY_RANGE::ROM_REG_2:
		case MEMORY_RANGE::ROM_REG_3:
			break;

		case MEMORY_RANGE::SRAM:
			break;
		}
	}

	template <>
	void Bus::Write(u32 address, u32 value) {
		MEMORY_RANGE region = (MEMORY_RANGE)(address >> 24);
		u32 addr_low = address & 0x00FFFFFF;

		if ((u8)region >= NUM_REGIONS)
			return;

		if (addr_low & ~REGIONS_LEN[(u8)region])
			return;

		addr_low &= ~3;

		switch (region) {
		case MEMORY_RANGE::BIOS:
			LOG_ERROR("Writing to BIOS memory\n"
				"Address : 0x{:x}, Value : 0x{:x}", address, value);
			break;

		case MEMORY_RANGE::EWRAM:
			reinterpret_cast<u32*>(m_wram)[addr_low / 4] = value;
			break;

		case MEMORY_RANGE::IWRAM:
			reinterpret_cast<u32*>(m_iwram)[addr_low / 4] = value;
			break;

		case MEMORY_RANGE::IO:
			LOG_INFO(" IO memory write");
			break;

		case MEMORY_RANGE::PAL:
			break;

		case MEMORY_RANGE::VRAM:
			break;

		case MEMORY_RANGE::OAM:
			break;

		case MEMORY_RANGE::ROM_REG_1:
		case MEMORY_RANGE::ROM_REG_2:
		case MEMORY_RANGE::ROM_REG_3:
			break;

		case MEMORY_RANGE::SRAM:
			break;
		}
	}

	Bus::~Bus() {
		if (m_iwram)
			delete[] m_iwram;

		if (m_wram)
			delete[] m_wram;
	}
}