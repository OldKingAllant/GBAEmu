#include "../../ppu/PPU.hpp"

#include "../../memory/MMIO.hpp"
#include "../../memory/InterruptController.hpp"
#include "../../memory/EventScheduler.hpp"

#include "../../common/Logger.hpp"
#include "../../common/Error.hpp"

namespace GBA::ppu {
	using namespace common;
	using memory::EventType;

	LOG_CONTEXT(PPU);

	PPU::PPU() : 
		m_ctx{}, m_mode_cycles{},
		m_curr_mode{}, m_palette_ram(nullptr),
		m_vram(nullptr), m_oam(nullptr),
		m_framebuffer(nullptr),
		m_internal_reference_x{}, 
		m_internal_reference_y{},
		m_frame_ok{false}, m_int_control(nullptr),
		m_sched(nullptr), m_last_event_timestamp{0}
	{
		m_palette_ram = new u8[0x400];
		m_vram = new u8[0x18000];
		m_oam = new u8[0x400];
		m_framebuffer = new float[240 * 160 * 3];

		std::fill_n(m_palette_ram, 0x400, 0x0);
		std::fill_n(m_vram, 0x18000, 0x0);

		//u16 oam_fill = 1 << 9;

		std::fill_n(reinterpret_cast<u16*>(m_oam),
			0x200, 0x0000);
	}

	void PPU::SetInterruptController(memory::InterruptController* int_controller) {
		m_int_control = int_controller;
	}

	void HblankEventCallback(void* ppu_ptr);
	void NormalEventCallback(void* ppu_ptr);
	void VblankEventCallback(void* ppu_ptr);
	void ScanlineEventCallback(void* ppu_ptr);

	void HblankEventCallback(void* ppu_ptr) {
		PPU& ppu = *reinterpret_cast<PPU*>( ppu_ptr );

		ppu.Normal();

		if (CHECK_BIT(ppu.m_ctx.m_status, 4)) {
			ppu.m_int_control->RequestInterrupt(memory::InterruptType::HBLANK);
		}

		ppu.m_ctx.m_status |= 2;

		if (ppu.m_ctx.m_vcount + 1 >= PPU::VISIBLE_LINES)
			ppu.m_sched->ScheduleAbsolute(ppu.m_last_event_timestamp + PPU::CYCLES_PER_HBLANK,
				EventType::VBLANK, VblankEventCallback, ppu_ptr);
		else
			ppu.m_sched->ScheduleAbsolute(ppu.m_last_event_timestamp + PPU::CYCLES_PER_HBLANK,
				EventType::PPUNORMAL, NormalEventCallback, ppu_ptr);

		ppu.m_last_event_timestamp += PPU::CYCLES_PER_HBLANK;
	}

	void NormalEventCallback(void* ppu_ptr) {
		PPU& ppu = *reinterpret_cast<PPU*>(ppu_ptr);

		ppu.m_ctx.m_vcount++;
		ppu.m_ctx.m_status &= ~2;

		u8 lyc = (ppu.m_ctx.m_status >> 8) & 0xFF;

		if (lyc == ppu.m_ctx.m_vcount) {
			//Set VCOUNT flag
			ppu.m_ctx.m_status |= (1 << 2);

			if (CHECK_BIT(ppu.m_ctx.m_status, 5))
				ppu.m_int_control->RequestInterrupt(memory::InterruptType::VCOUNT);
		}
		else
			ppu.m_ctx.m_status &= ~(1 << 2);

		ppu.m_sched->ScheduleAbsolute(ppu.m_last_event_timestamp + PPU::CYCLES_PER_SCANLINE,
			EventType::HBLANK, HblankEventCallback, ppu_ptr);

		ppu.m_last_event_timestamp += PPU::CYCLES_PER_SCANLINE;
	}

	void ScanlineEventCallback(void* ppu_ptr) {
		PPU& ppu = *reinterpret_cast<PPU*>(ppu_ptr);

		ppu.m_ctx.m_vcount++;

		if (ppu.m_ctx.m_vcount >= PPU::TOTAL_LINES) {
			ppu.m_ctx.m_vcount = 0;
			ppu.m_ctx.m_status &= ~1;

			ppu.m_sched->ScheduleAbsolute(ppu.m_last_event_timestamp + PPU::CYCLES_PER_SCANLINE,
				EventType::HBLANK, HblankEventCallback, ppu_ptr);
		}
		else {
			ppu.m_sched->ScheduleAbsolute(ppu.m_last_event_timestamp + PPU::CYCLES_PER_SCANLINE,
				EventType::SCANLINE_INC, ScanlineEventCallback, ppu_ptr, true);;
		}

		u8 lyc = (ppu.m_ctx.m_status >> 8) & 0xFF;

		if (lyc == ppu.m_ctx.m_vcount) {
			//Set VCOUNT flag
			ppu.m_ctx.m_status |= (1 << 2);

			if (CHECK_BIT(ppu.m_ctx.m_status, 5))
				ppu.m_int_control->RequestInterrupt(memory::InterruptType::VCOUNT);
		}
		else
			ppu.m_ctx.m_status &= ~(1 << 2);

		ppu.m_last_event_timestamp += PPU::CYCLES_PER_SCANLINE;
	}

	void VblankEventCallback(void* ppu_ptr) {
		PPU& ppu = *reinterpret_cast<PPU*>(ppu_ptr);

		ppu.m_ctx.m_vcount++;

		ppu.m_ctx.m_status &= ~2;
		ppu.m_ctx.m_status |= 1;

		if (CHECK_BIT(ppu.m_ctx.m_status, 3)) {
			ppu.m_int_control->RequestInterrupt(memory::InterruptType::VBLANK);
		}

		ppu.m_sched->ScheduleAbsolute(ppu.m_last_event_timestamp + PPU::TOTAL_CYCLES_PER_LINE,
			EventType::SCANLINE_INC, ScanlineEventCallback, ppu_ptr);

		ppu.m_frame_ok = true;

		ppu.m_last_event_timestamp += PPU::TOTAL_CYCLES_PER_LINE;
	}

	void PPU::SetScheduler(memory::EventScheduler* sched) {
		m_sched = sched;

		sched->ScheduleAbsolute(m_last_event_timestamp +
			CYCLES_PER_SCANLINE, EventType::HBLANK, HblankEventCallback, this);

		m_last_event_timestamp += PPU::CYCLES_PER_SCANLINE;
	}

	void PPU::InitHandlers(memory::MMIO* mmio) {
		mmio->AddRegister<u16>(0x0, true, true, &m_ctx.array[0x0], 0b1111111111110111);
		mmio->AddRegister<u16>(0x4, true, true, &m_ctx.array[0x4], 0b1111111111111000);
		mmio->AddRegister<u16>(0x6, true, false, &m_ctx.array[0x6], 0x0);
		
		//Backround Control
		mmio->AddRegister<u16>(0x8, true, true, &m_ctx.array[0x8], 0xFFFF);
		mmio->AddRegister<u16>(0xA, true, true, &m_ctx.array[0xA], 0xFFFF);
		mmio->AddRegister<u16>(0xC, true, true, &m_ctx.array[0xC], 0xFFFF);
		mmio->AddRegister<u16>(0xE, true, true, &m_ctx.array[0xE], 0xFFFF);

		//Normal backround scroll
		mmio->AddRegister<u16>(0x10, true, true, &m_ctx.array[0x10], 0xFFFF);
		mmio->AddRegister<u16>(0x12, true, true, &m_ctx.array[0x12], 0xFFFF);

		mmio->AddRegister<u16>(0x14, true, true, &m_ctx.array[0x14], 0xFFFF);
		mmio->AddRegister<u16>(0x16, true, true, &m_ctx.array[0x16], 0xFFFF);

		mmio->AddRegister<u16>(0x18, true, true, &m_ctx.array[0x18], 0xFFFF);
		mmio->AddRegister<u16>(0x1A, true, true, &m_ctx.array[0x1A], 0xFFFF);

		mmio->AddRegister<u16>(0x1C, true, true, &m_ctx.array[0x1C], 0xFFFF);
		mmio->AddRegister<u16>(0x1E, true, true, &m_ctx.array[0x1E], 0xFFFF);


		//Affine BG scroll
		mmio->AddRegister<u32>(0x28, true, true, &m_ctx.array[0x28], 0x0F'FF'FF'FF, 
			[this](u8 value, u16 offset) {
				u8 shift_amount = (offset % 4) * 8;

				u32 original_val = *reinterpret_cast<u32*>(m_ctx.array + 0x28);

				original_val &= ~((u32)0xFF << shift_amount);
				original_val |= ((u32)value << shift_amount);

				*reinterpret_cast<u32*>(m_ctx.array + 0x28) = original_val;

				m_internal_reference_x[0] = original_val;
		});

		mmio->AddRegister<u32>(0x2C, true, true, &m_ctx.array[0x2C], 0x0F'FF'FF'FF ,
			[this](u8 value, u16 offset) {
				u8 shift_amount = (offset % 4) * 8;

				u32 original_val = *reinterpret_cast<u32*>(m_ctx.array + 0x2C);

				original_val &= ~((u32)0xFF << shift_amount);
				original_val |= ((u32)value << shift_amount);

				*reinterpret_cast<u32*>(m_ctx.array + 0x2C) = original_val;

				m_internal_reference_y[0] = original_val;
		});

		mmio->AddRegister<u32>(0x38, true, true, &m_ctx.array[0x38], 0x0F'FF'FF'FF,
			[this](u8 value, u16 offset) {
				u8 shift_amount = (offset % 4) * 8;

				u32 original_val = *reinterpret_cast<u32*>(m_ctx.array + 0x38);

				original_val &= ~((u32)0xFF << shift_amount);
				original_val |= ((u32)value << shift_amount);

				*reinterpret_cast<u32*>(m_ctx.array + 0x38) = original_val;

				m_internal_reference_x[1] = original_val;
		});

		mmio->AddRegister<u32>(0x3C, true, true, &m_ctx.array[0x3C], 0x0F'FF'FF'FF, 
			[this](u8 value, u16 offset) {
				u8 shift_amount = (offset % 4) * 8;

				u32 original_val = *reinterpret_cast<u32*>(m_ctx.array + 0x3C);

				original_val &= ~((u32)0xFF << shift_amount);
				original_val |= ((u32)value << shift_amount);

				*reinterpret_cast<u32*>(m_ctx.array + 0x3C) = original_val;

				m_internal_reference_y[1] = original_val;
		});

		//Affine BG parameters
		mmio->AddRegister<u16>(0x20, true, true, &m_ctx.array[0x20], 0xFFFF);
		mmio->AddRegister<u16>(0x22, true, true, &m_ctx.array[0x22], 0xFFFF);
		mmio->AddRegister<u16>(0x24, true, true, &m_ctx.array[0x24], 0xFFFF);
		mmio->AddRegister<u16>(0x26, true, true, &m_ctx.array[0x26], 0xFFFF);

		mmio->AddRegister<u16>(0x30, true, true, &m_ctx.array[0x30], 0xFFFF);
		mmio->AddRegister<u16>(0x32, true, true, &m_ctx.array[0x32], 0xFFFF);
		mmio->AddRegister<u16>(0x34, true, true, &m_ctx.array[0x34], 0xFFFF);
		mmio->AddRegister<u16>(0x36, true, true, &m_ctx.array[0x36], 0xFFFF);

		//Windows
		mmio->AddRegister<u16>(0x40, true, true, &m_ctx.array[0x40], 0xFFFF);
		mmio->AddRegister<u16>(0x42, true, true, &m_ctx.array[0x42], 0xFFFF);

		mmio->AddRegister<u16>(0x44, true, true, &m_ctx.array[0x44], 0xFFFF);
		mmio->AddRegister<u16>(0x46, true, true, &m_ctx.array[0x46], 0xFFFF);

		mmio->AddRegister<u16>(0x48, true, true, &m_ctx.array[0x48], 0xFFFF);
		mmio->AddRegister<u16>(0x4A, true, true, &m_ctx.array[0x4A], 0xFFFF);

		//Mosaic
		mmio->AddRegister<u32>(0x4C, false, true, &m_ctx.array[0x4C], 0xFFFF'FFFF);

		//Color special effects
		mmio->AddRegister<u16>(0x50, true, true, &m_ctx.array[0x50], 0xFFFF);
		mmio->AddRegister<u16>(0x52, true, true, &m_ctx.array[0x52], 0xFFFF);
		mmio->AddRegister<u16>(0x54, true, true, &m_ctx.array[0x54], 0xFFFF);
	}

	void PPU::ResetFrameData() {
		m_internal_reference_x[0] = ReadRegister32(0x28 / 4);
		m_internal_reference_y[0] = ReadRegister32(0x2C / 4);
		m_internal_reference_x[1] = ReadRegister32(0x38 / 4);
		m_internal_reference_y[1] = ReadRegister32(0x3C / 4);

		m_internal_reference_x[0] &= 0x0F'FF'FF'FF;
		m_internal_reference_y[0] &= 0x0F'FF'FF'FF;
		m_internal_reference_x[1] &= 0x0F'FF'FF'FF;
		m_internal_reference_y[1] &= 0x0F'FF'FF'FF;
	}

	void PPU::VBlank() {}

	void PPU::HBlank() {}

	void PPU::Normal() {
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
	}

	void PPU::ClockCycles(u32 num_cycles) {}

	PPU::~PPU() {
		delete[] m_palette_ram;
		delete[] m_vram;
		delete[] m_framebuffer;
		delete[] m_oam;
	}

	void PPU::Mode2() {}

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

	common::u8* PPU::DebuggerGetPalette() {
		return m_palette_ram;
	}

	common::u8* PPU::DebuggerGetVRAM() {
		return m_vram;
	}
}