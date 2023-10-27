#include "../../memory/Bus.hpp"
#include "../../gamepack/GamePack.hpp"

#include "../../common/Logger.hpp"
#include "../../common/Error.hpp"

#include "../../cpu/core/ARM7TDI.hpp"

#include "../../ppu/PPU.hpp"

#include "../../memory/DirectMemoryAccess.hpp"

#include <filesystem>
#include <fstream>

namespace GBA::memory {
	LOG_CONTEXT(Memory bus);

	Bus::Bus() :
		m_pack(nullptr), m_wram(nullptr),
		m_iwram(nullptr), m_prefetch{}, 
		m_time{}, m_enable_prefetch(false), 
		m_bios_latch{0x00}, m_open_bus_value{0x00},
		m_open_bus_address{0x00}, m_ppu(nullptr), 
		m_bios(nullptr), m_sched(nullptr),
		active_dmas_count{}, active_dmas{},
		dmas{}
	{
		m_wram = new u8[0x40000];
		m_iwram = new u8[0x8000];

		std::fill_n(m_wram, 0x40000, 0x00);
		std::fill_n(m_iwram, 0x8000, 0x00);

		mmio = new MMIO();

		m_bios = new u8[16 * 1024];
	}

	void Bus::ConnectGamepack(gamepack::GamePack* pack) {
		m_pack = pack;
	}

	void Bus::InternalCycles(u32 count) {
		if (m_prefetch.active) {
			m_prefetch.current_cycles += count;
		}

		m_time.PushInternalCycles(count);
		m_sched->Advance(count);
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
			return *reinterpret_cast<u32*>(m_bios + addr_low);

		case MEMORY_RANGE::EWRAM:
			return *reinterpret_cast<u32*>(m_wram + addr_low);

		case MEMORY_RANGE::IWRAM:
			return *reinterpret_cast<u32*>(m_iwram + addr_low);

		case MEMORY_RANGE::IO:
			return 0x00;

		case MEMORY_RANGE::PAL:
			return *reinterpret_cast<u32*>(m_ppu->DebuggerGetPalette() + addr_low);

		case MEMORY_RANGE::VRAM:
			return *reinterpret_cast<u32*>(m_ppu->DebuggerGetVRAM() + addr_low);

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
			return *reinterpret_cast<u16*>(m_bios + addr_low);

		case MEMORY_RANGE::EWRAM:
			return *reinterpret_cast<u16*>(m_wram + addr_low);

		case MEMORY_RANGE::IWRAM:
			return *reinterpret_cast<u16*>(m_iwram + addr_low);

		case MEMORY_RANGE::IO:
			return 0x00;

		case MEMORY_RANGE::PAL:
			return *reinterpret_cast<u16*>(m_ppu->DebuggerGetPalette() + addr_low);

		case MEMORY_RANGE::VRAM:
			return *reinterpret_cast<u16*>(m_ppu->DebuggerGetVRAM() + addr_low);

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


	void Bus::LoadBIOS(std::string const& location) {
		if (!std::filesystem::exists(location)) {
			LOG_ERROR(" Could not load bios from path {}", location);
			return;
		}
		
		if (!std::filesystem::is_regular_file(location)) {
			LOG_ERROR(" Could not load bios from path {}", location);
			return;
		}

		constexpr std::size_t bios_size = (std::size_t)16 * 1024;

		auto file_size = std::filesystem::file_size(location);

		if (file_size != bios_size) {
			LOG_ERROR(" Invalid bios file size, expected exactly 16 KB, got {} bytes", file_size);
			return;
		}

		std::ifstream bios_file(location, std::ios::in | std::ios::binary);

		bios_file.read(reinterpret_cast<char*>(m_bios), bios_size);

		bios_file.close();

		m_bios_latch = *reinterpret_cast<u32*>(m_bios + 0xDC + 8);
	}

	u32 Bus::ReadBiosImpl(u32 address) {
		if (m_processor->GetContext().m_regs.GetReg(15) > 0x3FFF)
			return m_bios_latch;

		m_bios_latch = *reinterpret_cast<u32*>(m_bios + address);

		return m_bios_latch;
	}

	void Bus::LoadBiosResetOpcode() {
		m_bios_latch = *reinterpret_cast<u32*>(m_bios + 0xDC + 8);
	}

	void Bus::TryTriggerDMA(DMAFireType trigger_type) {
		dmas[0]->TriggerDMA(trigger_type);
		dmas[1]->TriggerDMA(trigger_type);
		dmas[2]->TriggerDMA(trigger_type);
		dmas[3]->TriggerDMA(trigger_type);
	}

	Bus::~Bus() {
		if (m_iwram)
			delete[] m_iwram;

		if (m_wram)
			delete[] m_wram;

		if (mmio)
			delete mmio;

		if (m_bios)
			delete[] m_bios;
	}
}