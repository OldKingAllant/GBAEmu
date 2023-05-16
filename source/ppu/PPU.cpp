#include "../../ppu/PPU.hpp"

#include "../../memory/MMIO.hpp"
#include "../../memory/InterruptController.hpp"

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
		m_frame_ok{false}, m_int_control(nullptr)
	{
		m_palette_ram = new u8[0x400];
		m_vram = new u8[0x17FFF];
		m_framebuffer = new float[240 * 160 * 3];

		std::fill_n(m_palette_ram, 0x400, 0x0);
		std::fill_n(m_vram, 0x17FFF, 0x0);
	}

	void PPU::SetInterruptController(memory::InterruptController* int_controller) {
		m_int_control = int_controller;
	}

	void PPU::InitHandlers(memory::MMIO* mmio) {
		mmio->AddRegister<u16>(0x0, true, true, &m_ctx.array[0x0], 0b1111111111110111);
		mmio->AddRegister<u16>(0x4, true, true, &m_ctx.array[0x4], 0b1111111111111000);
		mmio->AddRegister<u16>(0x6, true, false, &m_ctx.array[0x6], 0x0);
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

		*reinterpret_cast<u16*>(m_ctx.array) = value;
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

		*reinterpret_cast<u16*>(m_ctx.array + 0x4) = value;
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
			//Set VCOUNT flag
			m_ctx.m_status |= (1 << 2);

			if (CHECK_BIT(m_ctx.m_status, 5))
				m_int_control->RequestInterrupt(memory::InterruptType::VCOUNT);
		} else 
			m_ctx.m_status &= ~(1 << 2);
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

				if (CHECK_BIT(m_ctx.m_status, 3)) {
					m_int_control->RequestInterrupt(memory::InterruptType::VBLANK);
				}
			}
			else {
				m_curr_mode = Mode::NORMAL;
			}

			u8 lyc = (m_ctx.m_status >> 8) & 0xFF;

			if (lyc == m_ctx.m_vcount) {
				//Set VCOUNT flag
				m_ctx.m_status |= (1 << 2);

				if (CHECK_BIT(m_ctx.m_status, 5))
					m_int_control->RequestInterrupt(memory::InterruptType::VCOUNT);
			}
			else
				m_ctx.m_status &= ~(1 << 2);
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

			if (CHECK_BIT(m_ctx.m_status, 4)) {
				m_int_control->RequestInterrupt(memory::InterruptType::HBLANK);
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