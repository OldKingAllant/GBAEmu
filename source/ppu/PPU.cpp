#include "../../ppu/PPU.hpp"

#include "../../memory/MMIO.hpp"

#include "../../common/Logger.hpp"
#include "../../common/Error.hpp"

namespace GBA::ppu {
	using namespace common;

	LOG_CONTEXT(PPU);

	PPU::PPU() : 
		m_ctx{}, m_mode_cycles{},
		m_curr_mode{}, m_palette_ram(nullptr),
		m_vram(nullptr), m_framebuffer(nullptr),
		m_internal_reference_x_bg0{}, 
		m_internal_reference_x_bg1{},
		m_internal_reference_y_bg0{},
		m_internal_reference_y_bg1{},
		m_frame_ok{false}
	{
		m_palette_ram = new u8[0x400];
		m_vram = new u8[0x17FFF];
		m_framebuffer = new float[240 * 160 * 3];

		std::fill_n(m_palette_ram, 0x400, 0x0);
		std::fill_n(m_vram, 0x17FFF, 0x0);
	}

	void PPU::InitHandlers(memory::MMIO* mmio) {
		for (unsigned i = 0; i < 0x58; i++) {
			mmio->RegisterRead<u8>([this, i]() -> u8 {return this->ReadRegister8(i); }, i);
			mmio->RegisterWrite<u8>([this, i](u8 value) {this->WriteRegister8(i, value); }, i);
		}
		
		for (unsigned i = 0; i < 0x58; i += 2) {
			mmio->RegisterRead<u16>([this, i]() -> u16 {return this->ReadRegister16(i / 2); }, i);
			mmio->RegisterWrite<u16>([this, i](u16 value) {this->WriteRegister16(i / 2, value); }, i);
		}

		for (unsigned i = 0; i < 0x58; i += 4) {
			mmio->RegisterRead<u32>([this, i]() -> u32 {return this->ReadRegister32(i / 4); }, i);
			mmio->RegisterWrite<u32>([this, i](u32 value) {this->WriteRegister32(i / 4, value); }, i);
		}

		mmio->RegisterWrite<u8>(
			[this](u8 value) {
				WriteDisplayControl8(0, value);
			}, 0x0
		);

		mmio->RegisterWrite<u8>(
			[this](u8 value) {
				WriteDisplayControl8(1, value);
			}, 0x1
		);

		mmio->RegisterWrite<u16>(
			[this](u16 value) {
				WriteDisplayControl16(value);
			}, 0x0
		);

		mmio->RegisterWrite<u32>(
			[this](u32 value) {
				WriteDisplayControl16((u16)value);
				m_ctx.m_green_swap = (u16)(value >> 16);
			}, 0x0
		);

		mmio->RegisterWrite<u8>(
			[this](u8 value) {
				WriteDisplayStatus8(value, 0);
			}, 0x4
		);

		mmio->RegisterWrite<u8>(
			[this](u8 value) {
				WriteDisplayStatus8(value, 1);
			}, 0x5
		);

		mmio->RegisterWrite<u16>(
			[this](u16 value) {
				WriteDisplayStatus16(value);
			}, 0x4
		);

		mmio->RegisterWrite<u32>(
			[this](u32 value) {
				WriteDisplayStatus16((u16)value);
				//Ignore writes to VCOUNT
			}, 0x4
		);

		//Ignore writes to VCOUNT
		mmio->RegisterWrite<u8>([](u8) {}, 0x6);
		mmio->RegisterWrite<u8>([](u8) {}, 0x7);
		mmio->RegisterWrite<u16>([](u16) {}, 0x6);

		mmio->RegisterWrite<u8>([this](u8 value) {
			m_ctx.array[0x28] = value;
			m_internal_reference_x_bg0 = ReadRegister32(0x28 / 4);
		}, 0x28);

		mmio->RegisterWrite<u8>([this](u8 value) {
			m_ctx.array[0x29] = value;
			m_internal_reference_x_bg0 = ReadRegister32(0x28 / 4);
		}, 0x29);

		mmio->RegisterWrite<u8>([this](u8 value) {
			m_ctx.array[0x2A] = value;
			m_internal_reference_x_bg0 = ReadRegister32(0x28 / 4);
		}, 0x2A);

		mmio->RegisterWrite<u8>([this](u8 value) {
			m_ctx.array[0x2B] = value;
			m_internal_reference_x_bg0 = ReadRegister32(0x28 / 4);
		}, 0x2B);

		/////////////////

		mmio->RegisterWrite<u8>([this](u8 value) {
			m_ctx.array[0x2C] = value;
			m_internal_reference_y_bg0 = ReadRegister32(0x2C / 4);
			}, 0x2C);

		mmio->RegisterWrite<u8>([this](u8 value) {
			m_ctx.array[0x2D] = value;
			m_internal_reference_y_bg0 = ReadRegister32(0x2C / 4);
			}, 0x2D);

		mmio->RegisterWrite<u8>([this](u8 value) {
			m_ctx.array[0x2E] = value;
			m_internal_reference_y_bg0 = ReadRegister32(0x2C / 4);
			}, 0x2E);

		mmio->RegisterWrite<u8>([this](u8 value) {
			m_ctx.array[0x2F] = value;
			m_internal_reference_y_bg0 = ReadRegister32(0x2C / 4);
			}, 0x2F);

		////////////////

		mmio->RegisterWrite<u16>([this](u16 value) {
			WriteRegister16(0x28, value);
			m_internal_reference_x_bg0 = ReadRegister32(0x28 / 4);
			}, 0x28);

		mmio->RegisterWrite<u16>([this](u16 value) {
			WriteRegister16(0x2A, value);
			m_internal_reference_x_bg0 = ReadRegister32(0x28 / 4);
			}, 0x2A);

		mmio->RegisterWrite<u16>([this](u16 value) {
			WriteRegister16(0x2C, value);
			m_internal_reference_y_bg0 = ReadRegister32(0x2C / 4);
			}, 0x2C);

		mmio->RegisterWrite<u16>([this](u16 value) {
			WriteRegister16(0x2E, value);
			m_internal_reference_y_bg0 = ReadRegister32(0x2C / 4);
			}, 0x2E);

		///////////

		mmio->RegisterWrite<u32>([this](u32 value) {
			WriteRegister32(0x28, value);
			m_internal_reference_x_bg0 = value;
			}, 0x28);

		mmio->RegisterWrite<u32>([this](u32 value) {
			WriteRegister32(0x2C, value);
			m_internal_reference_y_bg0 = value;
			}, 0x2C);
	}

	void PPU::ResetFrameData() {
		m_internal_reference_x_bg0 = ReadRegister32(0x28 / 4);
		//m_internal_reference_x_bg1{},
		m_internal_reference_y_bg0 = ReadRegister32(0x2C / 4);
		//m_internal_reference_y_bg1{}
	}

	void PPU::WriteDisplayControl8(common::u8 offset, common::u8 value) {
		if (offset == 0)
			value &= ~(1 << 3);

		m_ctx.array[offset] = value;
	}

	void PPU::WriteDisplayControl16(common::u16 value) {
		value &= ~(1 << 3);

		reinterpret_cast<u16*>(m_ctx.array)[0] = value;
	}

	void PPU::WriteDisplayStatus8(common::u8 offset, common::u8 value) {
		if (offset == 0) {
			value &= ~0b111;
			value |= (u8)(m_ctx.m_status & 0b111);
		}
		
		m_ctx.array[0x4 + offset] = value;
	}

	void PPU::WriteDisplayStatus16(common::u16 value) {
		value &= ~0b111;
		value |= m_ctx.m_status & 0b111;

		reinterpret_cast<u16*>(m_ctx.array)[0x4] = value;
	}

	common::u32 PPU::ReadRegister32(common::u8 offset) const {
		return reinterpret_cast<u32 const*>(m_ctx.array)[offset];
	}

	void PPU::WriteRegister32(common::u8 offset, common::u32 value) {
		reinterpret_cast<u32*>(m_ctx.array)[offset] = value;
	}

	common::u16 PPU::ReadRegister16(common::u8 offset) const {
		return reinterpret_cast<u16 const*>(m_ctx.array)[offset];
	}

	void PPU::WriteRegister16(common::u8 offset, common::u16 value) {
		reinterpret_cast<u16*>(m_ctx.array)[offset] = value;
	}

	common::u8 PPU::ReadRegister8(common::u8 offset) const {
		return m_ctx.array[offset];
	}

	void PPU::WriteRegister8(common::u8 offset, common::u8 value) {
		m_ctx.array[offset] = value;
	}

	void PPU::VBlank() {
		if (m_mode_cycles < TOTAL_CYCLES_PER_LINE)
			return;

		m_mode_cycles -= TOTAL_CYCLES_PER_LINE;

		m_ctx.m_vcount++;

		if (m_ctx.m_vcount >= TOTAL_LINES) {
			m_ctx.m_vcount = 0;
			m_ctx.m_status &= ~1;

			m_curr_mode = Mode::NORMAL;

			ResetFrameData();
		}

		u8 lyc = (m_ctx.m_status >> 8) & 0xFF;

		if (lyc == m_ctx.m_vcount) {
			//TODO Interrupt
		}
	}

	void PPU::HBlank() {
		if (m_mode_cycles >= CYCLES_BEFORE_HBLANK_FLAG)
			m_ctx.m_status |= 2;

		if (m_mode_cycles >= CYCLES_PER_HBLANK) {
			m_mode_cycles -= CYCLES_PER_HBLANK;

			m_ctx.m_vcount++;

			m_ctx.m_status &= ~2;

			if (m_ctx.m_vcount >= 160) {
				m_curr_mode = Mode::VBLANK;
				m_ctx.m_status |= 1;
				m_frame_ok = true;
			}
			else {
				m_curr_mode = Mode::NORMAL;
			}

			u8 lyc = (m_ctx.m_status >> 8) & 0xFF;

			if (lyc == m_ctx.m_vcount) {
				//TODO Interrupt
			}
		}
	}

	void PPU::Normal() {
		if (m_mode_cycles >= CYCLES_PER_SCANLINE) {
			m_mode_cycles -= CYCLES_PER_SCANLINE;

			u8 mode = m_ctx.m_control & 0x7;

			switch (mode)
			{
			case 0:
				Mode0();
				break;

			case 1:
				Mode1();
				break;

			case 2:
				Mode2();
				break;

			case 3:
				Mode3();
				break;

			case 4:
				Mode4();
				break;

			case 5:
				Mode5();
				break;

			default:
				LOG_ERROR("Invalid display mode {0}!", (unsigned)mode);
				error::DebugBreak();
				break;
			}

			m_curr_mode = Mode::HBLANK;
		}
	}

	void PPU::ClockCycles(u32 num_cycles) {
		m_mode_cycles += num_cycles; 

		switch (m_curr_mode)
		{
		case Mode::NORMAL:
			Normal();
			break;

		case Mode::VBLANK:
			VBlank();
			break;

		case Mode::HBLANK:
			HBlank();
			break;

		default:
			error::Unreachable();
			break;
		}
	}

	PPU::~PPU() {
		delete[] m_palette_ram;
		delete[] m_vram;
		delete[] m_framebuffer;
	}

	void PPU::Mode0() {}
	void PPU::Mode1() {}
	void PPU::Mode2() {}
	void PPU::Mode3() {}
	void PPU::Mode5() {}
}