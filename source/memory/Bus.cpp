#include "../../memory/Bus.hpp"
#include "../../gamepack/GamePack.hpp"

#include "../../common/Logger.hpp"
#include "../../common/Error.hpp"

#include "../../cpu/core/ARM7TDI.hpp"

#include "../../ppu/PPU.hpp"

namespace GBA::memory {
	LOG_CONTEXT(Memory bus);

	Bus::Bus() :
		m_pack(nullptr), m_wram(nullptr),
		m_iwram(nullptr), m_prefetch{}, 
		m_time{}, m_enable_prefetch(false), 
		m_bios_latch{0x00}, m_open_bus_value{0x00},
		m_open_bus_address{0x00}, m_ppu(nullptr)
	{
		m_wram = new u8[0x3FFFF];
		m_iwram = new u8[0x7FFF];

		std::fill_n(m_wram, 0x3FFFF, 0x00);
		std::fill_n(m_iwram, 0x7FFF, 0x00);

		mmio = new MMIO();

		m_ppu = new ppu::PPU(mmio);
	}

	void Bus::ConnectGamepack(gamepack::GamePack* pack) {
		m_pack = pack;
	}

	/*template <>
	u8 Bus::Read(u32 address, bool code) {
		MEMORY_RANGE region = (MEMORY_RANGE)(address >> 24);
		u32 addr_low = address & 0x00FFFFFF;

		if((u8)region >= NUM_REGIONS)
			return static_cast<u8>(~0);

		if(addr_low & ~REGIONS_LEN[(u8)region])
			return static_cast<u8>(~0);

		switch (region) {
		case MEMORY_RANGE::BIOS:
			m_time.PushCycles<MEMORY_RANGE::BIOS, 1>();
			return 0x00;

		case MEMORY_RANGE::EWRAM:
			m_time.PushCycles<MEMORY_RANGE::EWRAM, 1>();
			return m_wram[addr_low];

		case MEMORY_RANGE::IWRAM:
			m_time.PushCycles<MEMORY_RANGE::IWRAM, 1>();
			return m_iwram[addr_low];

		case MEMORY_RANGE::IO:
			m_time.PushCycles<MEMORY_RANGE::IO, 1>();
			return 0x00;

		case MEMORY_RANGE::PAL:
			m_time.PushCycles<MEMORY_RANGE::PAL, 1>();
			return 0x00;

		case MEMORY_RANGE::VRAM:
			m_time.PushCycles<MEMORY_RANGE::VRAM, 1>();
			return 0x00;

		case MEMORY_RANGE::OAM:
			m_time.PushCycles<MEMORY_RANGE::OAM, 1>();
			return 0x00;

		case MEMORY_RANGE::ROM_REG_1:
		case MEMORY_RANGE::ROM_REG_2:
		case MEMORY_RANGE::ROM_REG_3:
			return (u8)(Prefetch<u16>(address, code, region));

		case MEMORY_RANGE::SRAM:
			m_time.PushCycles<MEMORY_RANGE::SRAM, 1>();
			return 0x00;
		}

		return static_cast<u8>(~0);
	}

	template <>
	u16 Bus::Read(u32 address, bool code) {
		MEMORY_RANGE region = (MEMORY_RANGE)(address >> 24);
		u32 addr_low = address & 0x00FFFFFF;
		addr_low &= ~1;

		if ((u8)region >= NUM_REGIONS)
			return static_cast<u16>(~0);

		if (addr_low & ~REGIONS_LEN[(u8)region])
			return static_cast<u16>(~0);

		switch (region) {
		case MEMORY_RANGE::BIOS:
			m_time.PushCycles<MEMORY_RANGE::BIOS, 2>();
			return 0x00;

		case MEMORY_RANGE::EWRAM:
			m_time.PushCycles<MEMORY_RANGE::EWRAM, 2>();
			return reinterpret_cast<u16*>(m_wram)[addr_low / 2];

		case MEMORY_RANGE::IWRAM:
			m_time.PushCycles<MEMORY_RANGE::IWRAM, 2>();
			return reinterpret_cast<u16*>(m_iwram)[addr_low / 2];

		case MEMORY_RANGE::IO:
			m_time.PushCycles<MEMORY_RANGE::IO, 2>();
			return 0x00;

		case MEMORY_RANGE::PAL:
			m_time.PushCycles<MEMORY_RANGE::PAL, 2>();
			return 0x00;

		case MEMORY_RANGE::VRAM:
			m_time.PushCycles<MEMORY_RANGE::VRAM, 2>();
			return 0x00;

		case MEMORY_RANGE::OAM:
			m_time.PushCycles<MEMORY_RANGE::OAM, 2>();
			return 0x00;

		case MEMORY_RANGE::ROM_REG_1:
		case MEMORY_RANGE::ROM_REG_2:
		case MEMORY_RANGE::ROM_REG_3:
			return Prefetch<u16>(address, code, region);

		case MEMORY_RANGE::SRAM:
			m_time.PushCycles<MEMORY_RANGE::SRAM, 2>();
			return 0x00;
		}

		return static_cast<u16>(~0);
	}

	template <>
	u32 Bus::Read(u32 address, bool code) {
		MEMORY_RANGE region = (MEMORY_RANGE)(address >> 24);
		u32 addr_low = address & 0x00FFFFFF;
		addr_low &= ~3;

		if ((u8)region >= NUM_REGIONS)
			return static_cast<u32>(~0);

		if (addr_low & ~REGIONS_LEN[(u8)region])
			return static_cast<u32>(~0);

		switch (region) {
		case MEMORY_RANGE::BIOS:
			m_time.PushCycles<MEMORY_RANGE::BIOS, 4>();
			return 0x00;

		case MEMORY_RANGE::EWRAM:
			m_time.PushCycles<MEMORY_RANGE::EWRAM, 4>();
			return reinterpret_cast<u32*>(m_wram)[addr_low / 4];

		case MEMORY_RANGE::IWRAM:
			m_time.PushCycles<MEMORY_RANGE::IWRAM, 4>();
			return reinterpret_cast<u32*>(m_iwram)[addr_low / 4];

		case MEMORY_RANGE::IO:
			m_time.PushCycles<MEMORY_RANGE::IO, 4>();
			return 0x00;

		case MEMORY_RANGE::PAL:
			m_time.PushCycles<MEMORY_RANGE::PAL, 4>();
			return 0x00;

		case MEMORY_RANGE::VRAM:
			m_time.PushCycles<MEMORY_RANGE::VRAM, 4>();
			return 0x00;

		case MEMORY_RANGE::OAM:
			m_time.PushCycles<MEMORY_RANGE::OAM, 4>();
			return 0x00;

		case MEMORY_RANGE::ROM_REG_1:
		case MEMORY_RANGE::ROM_REG_2:
		case MEMORY_RANGE::ROM_REG_3:
			return Prefetch<u32>(address, code, region);

		case MEMORY_RANGE::SRAM:
			m_time.PushCycles<MEMORY_RANGE::SRAM, 4>();
			return 0x00;
		}

		return static_cast<u32>(~0);
	}*/

	/*template <>
	void Bus::Write(u32 address, u8 value) {
		MEMORY_RANGE region = (MEMORY_RANGE)(address >> 24);
		u32 addr_low = address & 0x00FFFFFF;

		if ((u8)region >= NUM_REGIONS)
			return;

		if (addr_low & ~REGIONS_LEN[(u8)region])
			return;

		switch (region) {
		case MEMORY_RANGE::BIOS:
			m_time.PushCycles<MEMORY_RANGE::BIOS, 1>();
			break;

		case MEMORY_RANGE::EWRAM:
			m_time.PushCycles<MEMORY_RANGE::EWRAM, 1>();
			m_wram[addr_low] = value;
			break;

		case MEMORY_RANGE::IWRAM:
			m_time.PushCycles<MEMORY_RANGE::IWRAM, 1>();
			m_iwram[addr_low] = value;
			break;

		case MEMORY_RANGE::IO:
			m_time.PushCycles<MEMORY_RANGE::IO, 1>();
			break;

		case MEMORY_RANGE::PAL:
			m_time.PushCycles<MEMORY_RANGE::PAL, 1>();
			break;

		case MEMORY_RANGE::VRAM:
			m_time.PushCycles<MEMORY_RANGE::VRAM, 1>();
			break;

		case MEMORY_RANGE::OAM:
			m_time.PushCycles<MEMORY_RANGE::OAM, 1>();
			break;

		case MEMORY_RANGE::ROM_REG_1:
			m_time.PushCycles<MEMORY_RANGE::ROM_REG_1, 1>();
			StopPrefetch();
			return;

		case MEMORY_RANGE::ROM_REG_2:
			m_time.PushCycles<MEMORY_RANGE::ROM_REG_2, 1>();
			StopPrefetch();
			return;

		case MEMORY_RANGE::ROM_REG_3:
			m_time.PushCycles<MEMORY_RANGE::ROM_REG_3, 1>();
			StopPrefetch();
			break;

		case MEMORY_RANGE::SRAM:
			m_time.PushCycles<MEMORY_RANGE::SRAM, 1>();
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
			m_time.PushCycles<MEMORY_RANGE::BIOS, 2>();
			break;

		case MEMORY_RANGE::EWRAM:
			m_time.PushCycles<MEMORY_RANGE::EWRAM, 2>();
			reinterpret_cast<u16*>(m_wram)[addr_low / 2] = value;
			break;

		case MEMORY_RANGE::IWRAM:
			m_time.PushCycles<MEMORY_RANGE::IWRAM, 2>();
			reinterpret_cast<u16*>(m_iwram)[addr_low / 2] = value;
			break;

		case MEMORY_RANGE::IO:
			m_time.PushCycles<MEMORY_RANGE::IO, 2>();
			break;

		case MEMORY_RANGE::PAL:
			m_time.PushCycles<MEMORY_RANGE::PAL, 2>();
			break;

		case MEMORY_RANGE::VRAM:
			m_time.PushCycles<MEMORY_RANGE::VRAM, 2>();
			break;

		case MEMORY_RANGE::OAM:
			m_time.PushCycles<MEMORY_RANGE::OAM, 2>();
			break;

		case MEMORY_RANGE::ROM_REG_1:
			m_time.PushCycles<MEMORY_RANGE::ROM_REG_1, 2>();
			StopPrefetch();
			return;

		case MEMORY_RANGE::ROM_REG_2:
			m_time.PushCycles<MEMORY_RANGE::ROM_REG_2, 2>();
			StopPrefetch();
			return;

		case MEMORY_RANGE::ROM_REG_3:
			m_time.PushCycles<MEMORY_RANGE::ROM_REG_3, 2>();
			StopPrefetch();
			break;

		case MEMORY_RANGE::SRAM:
			m_time.PushCycles<MEMORY_RANGE::SRAM, 2>();
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
			m_time.PushCycles<MEMORY_RANGE::BIOS, 4>();
			break;

		case MEMORY_RANGE::EWRAM:
			m_time.PushCycles<MEMORY_RANGE::EWRAM, 4>();
			reinterpret_cast<u32*>(m_wram)[addr_low / 4] = value;
			break;

		case MEMORY_RANGE::IWRAM:
			m_time.PushCycles<MEMORY_RANGE::IWRAM, 4>();
			reinterpret_cast<u32*>(m_iwram)[addr_low / 4] = value;
			break;

		case MEMORY_RANGE::IO:
			m_time.PushCycles<MEMORY_RANGE::IO, 4>();
			LOG_INFO(" IO memory write");
			break;

		case MEMORY_RANGE::PAL:
			m_time.PushCycles<MEMORY_RANGE::PAL, 4>();
			break;

		case MEMORY_RANGE::VRAM:
			m_time.PushCycles<MEMORY_RANGE::VRAM, 4>();
			break;

		case MEMORY_RANGE::OAM:
			m_time.PushCycles<MEMORY_RANGE::OAM, 4>();
			break;

		case MEMORY_RANGE::ROM_REG_1:
			m_time.PushCycles<MEMORY_RANGE::ROM_REG_1, 4>();
			StopPrefetch();
			return;

		case MEMORY_RANGE::ROM_REG_2:
			m_time.PushCycles<MEMORY_RANGE::ROM_REG_2, 4>();
			StopPrefetch();
			return;

		case MEMORY_RANGE::ROM_REG_3:
			m_time.PushCycles<MEMORY_RANGE::ROM_REG_3, 4>();
			StopPrefetch();
			break;

		case MEMORY_RANGE::SRAM:
			m_time.PushCycles<MEMORY_RANGE::SRAM, 4>();
			break;
		}
	}*/

	void Bus::InternalCycles(u32 count) {
		if (m_prefetch.active) {
			m_prefetch.current_cycles += count;
		}

		m_time.PushInternalCycles(count);
	}

	u32 Bus::DebuggerRead32(u32 address) {
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
			u32 value = m_pack->Read(addr_low);
			value |= (m_pack->Read(addr_low + 2) << 16);
			return value;
		}

		case MEMORY_RANGE::SRAM:
			return 0x00;
		}

		return static_cast<u32>(~0);
	}

	u16 Bus::DebuggerRead16(u32 address) {
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

	u32 Bus::ReadOpenBus(u32 address) {
		cpu::CPUContext& ctx = m_processor->GetContext();
		if (ctx.m_cpsr.instr_state ==
			cpu::InstructionMode::ARM) {
			return m_open_bus_value;
		}

		LOG_INFO(" Open bus for THUMB mode not implemented");

		return 0;
	}

	u32 Bus::ReadBiosImpl(u32 address) {
		if (m_processor->GetContext().m_regs.GetReg(15) > 0x3FFF)
			return m_bios_latch;

		m_bios_latch = 0x00; //Dummy value

		return m_bios_latch;
	}

	Bus::~Bus() {
		if (m_iwram)
			delete[] m_iwram;

		if (m_wram)
			delete[] m_wram;
	}
}