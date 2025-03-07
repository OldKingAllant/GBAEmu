#include "../../ppu/PPU.hpp"

#include "../../memory/MMIO.hpp"
#include "../../memory/InterruptController.hpp"
#include "../../memory/EventScheduler.hpp"
#include "../../memory/Bus.hpp"

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
		m_saved_internal_reference_x{},
		m_saved_internal_reference_y{},
		m_dirty_ref_x{}, m_dirty_ref_y{},
		m_frame_ok{false}, m_int_control(nullptr),
		m_sched(nullptr), m_last_event_timestamp{0},
		m_bus{nullptr}, m_mmio{nullptr},
		line_sprites_ids{}, line_sprites_count(0), 
		m_line_data{}, m_obj_window_pixels{},
		m_render_thread{},
		m_dirty_oam{}, m_dirty_pal{}, m_dirty_vram{},
		m_pal_buf{nullptr}, m_oam_buf{nullptr},
		m_line_has_changes_buf{}, 
		m_line_has_changes{},
		m_enable_scanline_diff_threshold{false},
		m_scanline_diff_threshold{0},
		m_sync_all_lines{false}
	{
		m_palette_ram = new u8[0x400];
		m_vram = new u8[0x18000];
		m_oam = new u8[0x400];
		m_framebuffer = new float[240 * 160 * 3];

		std::fill_n(m_palette_ram, 0x400, 0x0);
		std::fill_n(m_vram, 0x18000, 0x0);

		std::fill_n(reinterpret_cast<u16*>(m_oam),
			0x200, 0x0000);

		m_render_thread.m_ppu = this;
	}

	void PPU::SetInterruptController(memory::InterruptController* int_controller) {
		m_int_control = int_controller;
	}

	void HblankEventCallback(void* ppu_ptr);
	void NormalEventCallback(void* ppu_ptr);
	void VblankEventCallback(void* ppu_ptr);
	void ScanlineEventCallback(void* ppu_ptr);
	void VblankHblankCallback(void* ppu_ptr);
	void VblankEndCallback(void* ppu_ptr);

	void HblankEventCallback(void* ppu_ptr) {
		PPU& ppu = *reinterpret_cast<PPU*>( ppu_ptr );

		if (ppu.m_render_thread.m_running) {
			//ppu.m_line_has_changes_buf[ppu.m_ctx.m_vcount] = false;
		}
		else {
			ppu.CopyBufferedData(false);
			ppu.RenderScanline(ppu.m_ctx_saved.m_vcount);
		}

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

		ppu.m_bus->TryTriggerDMA(memory::DMAFireType::HBLANK);
	}

	void NormalEventCallback(void* ppu_ptr) {
		PPU& ppu = *reinterpret_cast<PPU*>(ppu_ptr);

		if (ppu.m_render_thread.m_running) {
			ppu.m_render_thread.CheckScanlineChanges();
		}

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

		if (ppu.m_render_thread.m_running) {
			ppu.m_render_thread.SyncScanlineChanges();
		}

		ppu.m_last_event_timestamp += PPU::CYCLES_PER_SCANLINE;
	}

	void ScanlineEventCallback(void* ppu_ptr) {
		PPU& ppu = *reinterpret_cast<PPU*>(ppu_ptr);

		ppu.m_ctx.m_vcount++;

		ppu.m_ctx.m_status &= ~2; //Clear HBLANK flag

		ppu.m_sched->ScheduleAbsolute(ppu.m_last_event_timestamp + PPU::CYCLES_PER_SCANLINE,
			EventType::HBLANK_IN_VBLANK, VblankHblankCallback, ppu_ptr);

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

		u8 lyc = (ppu.m_ctx.m_status >> 8) & 0xFF;

		if (lyc == ppu.m_ctx.m_vcount) {
			//Set VCOUNT flag
			ppu.m_ctx.m_status |= (1 << 2);

			if (CHECK_BIT(ppu.m_ctx.m_status, 5))
				ppu.m_int_control->RequestInterrupt(memory::InterruptType::VCOUNT);
		}
		else
			ppu.m_ctx.m_status &= ~(1 << 2);

		ppu.m_ctx.m_status &= ~2;
		ppu.m_ctx.m_status |= 1;

		if (CHECK_BIT(ppu.m_ctx.m_status, 3)) {
			ppu.m_int_control->RequestInterrupt(memory::InterruptType::VBLANK);
		}

		ppu.m_sched->ScheduleAbsolute(ppu.m_last_event_timestamp + PPU::CYCLES_PER_SCANLINE,
			EventType::HBLANK_IN_VBLANK, VblankHblankCallback, ppu_ptr);

		if (ppu.m_render_thread.m_running) {
			ppu.m_render_thread.Wait();
		}
		
		ppu.m_frame_ok = true;

		ppu.m_last_event_timestamp += PPU::CYCLES_PER_SCANLINE;

		ppu.m_bus->TryTriggerDMA(memory::DMAFireType::VBLANK);

		ppu.ResetFrameData();
	}

	//This event is very specific in the sense that
	//it is called when an HBLANK occurs during
	//VBLANK period
	void VblankHblankCallback(void* ppu_ptr) {
		PPU* ppu = reinterpret_cast<PPU*>(ppu_ptr);

		if (CHECK_BIT(ppu->m_ctx.m_status, 4)) {
			ppu->m_int_control->RequestInterrupt(memory::InterruptType::HBLANK);
		}

		ppu->m_ctx.m_status |= 2;

		/*
			Event flow:
			During VBLANK, start with a normal scanline event
			which lasts 960 cycles. After that, an HBLANK event
			is triggered:
				- If the current scanline is the end of the VBLANK
				  period, schedule a "Vblank End" event
				- Else, schedule a Scanline event
		*/

		if (ppu->m_ctx.m_vcount + 1 >= PPU::TOTAL_LINES) {
			ppu->m_sched->ScheduleAbsolute(ppu->m_last_event_timestamp + PPU::CYCLES_PER_HBLANK,
				EventType::END_VBLANK, VblankEndCallback, ppu_ptr);
		}
		else {
			ppu->m_sched->ScheduleAbsolute(ppu->m_last_event_timestamp + PPU::CYCLES_PER_HBLANK,
				EventType::SCANLINE_INC, ScanlineEventCallback, ppu_ptr);;
		}

		ppu->m_last_event_timestamp += PPU::CYCLES_PER_HBLANK;
	}

	void VblankEndCallback(void* ppu_ptr) {
		PPU* ppu = reinterpret_cast<PPU*>(ppu_ptr);

		ppu->m_ctx.m_vcount = 0;

		u8 lyc = (ppu->m_ctx.m_status >> 8) & 0xFF;

		if (lyc == ppu->m_ctx.m_vcount) {
			//Set VCOUNT flag
			ppu->m_ctx.m_status |= (1 << 2);

			if (CHECK_BIT(ppu->m_ctx.m_status, 5))
				ppu->m_int_control->RequestInterrupt(memory::InterruptType::VCOUNT);
		}
		else
			ppu->m_ctx.m_status &= ~(1 << 2);

		ppu->m_ctx.m_status &= ~1; //Clear VBLANK flag
		ppu->m_ctx.m_status &= ~2; //Clear HBLANK flag

		//Return to normal operation
		//Next event is an HBLANK event
		ppu->m_sched->ScheduleAbsolute(ppu->m_last_event_timestamp + PPU::CYCLES_PER_SCANLINE,
			EventType::HBLANK, HblankEventCallback, ppu_ptr);

		ppu->m_last_event_timestamp += PPU::CYCLES_PER_SCANLINE;

		if (ppu->m_render_thread.m_running) {
			ppu->m_render_thread.UpdateScanlineBatches();
			ppu->m_render_thread.StartScanline();
		}
	}

	void PPU::SetScheduler(memory::EventScheduler* sched) {
		m_sched = sched;

		m_sched->SetEventTypeRodata(EventType::HBLANK, HblankEventCallback,
			std::bit_cast<void*>(this));
		m_sched->SetEventTypeRodata(EventType::VBLANK, VblankEventCallback,
			std::bit_cast<void*>(this));
		m_sched->SetEventTypeRodata(EventType::PPUNORMAL, NormalEventCallback,
			std::bit_cast<void*>(this));
		m_sched->SetEventTypeRodata(EventType::SCANLINE_INC, ScanlineEventCallback,
			std::bit_cast<void*>(this));
		m_sched->SetEventTypeRodata(EventType::HBLANK_IN_VBLANK, VblankHblankCallback,
			std::bit_cast<void*>(this));
		m_sched->SetEventTypeRodata(EventType::END_VBLANK, VblankEndCallback,
			std::bit_cast<void*>(this));

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
		mmio->AddRegister<u16>(0x10, false, true, &m_ctx.array[0x10], 0xFFFF);
		mmio->AddRegister<u16>(0x12, false, true, &m_ctx.array[0x12], 0xFFFF);

		mmio->AddRegister<u16>(0x14, false, true, &m_ctx.array[0x14], 0xFFFF);
		mmio->AddRegister<u16>(0x16, false, true, &m_ctx.array[0x16], 0xFFFF);

		mmio->AddRegister<u16>(0x18, false, true, &m_ctx.array[0x18], 0xFFFF);
		mmio->AddRegister<u16>(0x1A, false, true, &m_ctx.array[0x1A], 0xFFFF);

		mmio->AddRegister<u16>(0x1C, false, true, &m_ctx.array[0x1C], 0xFFFF);
		mmio->AddRegister<u16>(0x1E, false, true, &m_ctx.array[0x1E], 0xFFFF);


		//Affine BG scroll
		mmio->AddRegister<u32>(0x28, false, true, &m_ctx.array[0x28], 0x0F'FF'FF'FF,
			[this](u8 value, u16 offset) {
				u8 shift_amount = (offset % 4) * 8;

				u32 original_val = *reinterpret_cast<u32*>(m_ctx.array + 0x28);

				original_val &= ~((u32)0xFF << shift_amount);
				original_val |= ((u32)value << shift_amount);

				*reinterpret_cast<u32*>(m_ctx.array + 0x28) = original_val;

				m_internal_reference_x[0] = original_val;
				m_dirty_ref_x[0] = true;
		});

		mmio->AddRegister<u32>(0x2C, false, true, &m_ctx.array[0x2C], 0x0F'FF'FF'FF ,
			[this](u8 value, u16 offset) {
				u8 shift_amount = (offset % 4) * 8;

				u32 original_val = *reinterpret_cast<u32*>(m_ctx.array + 0x2C);

				original_val &= ~((u32)0xFF << shift_amount);
				original_val |= ((u32)value << shift_amount);

				*reinterpret_cast<u32*>(m_ctx.array + 0x2C) = original_val;

				m_internal_reference_y[0] = original_val;
				m_dirty_ref_y[0] = true;
		});

		mmio->AddRegister<u32>(0x38, false, true, &m_ctx.array[0x38], 0x0F'FF'FF'FF,
			[this](u8 value, u16 offset) {
				u8 shift_amount = (offset % 4) * 8;

				u32 original_val = *reinterpret_cast<u32*>(m_ctx.array + 0x38);

				original_val &= ~((u32)0xFF << shift_amount);
				original_val |= ((u32)value << shift_amount);

				*reinterpret_cast<u32*>(m_ctx.array + 0x38) = original_val;

				m_internal_reference_x[1] = original_val;
				m_dirty_ref_x[1] = true;
		});

		mmio->AddRegister<u32>(0x3C, false, true, &m_ctx.array[0x3C], 0x0F'FF'FF'FF,
			[this](u8 value, u16 offset) {
				u8 shift_amount = (offset % 4) * 8;

				u32 original_val = *reinterpret_cast<u32*>(m_ctx.array + 0x3C);

				original_val &= ~((u32)0xFF << shift_amount);
				original_val |= ((u32)value << shift_amount);

				*reinterpret_cast<u32*>(m_ctx.array + 0x3C) = original_val;

				m_internal_reference_y[1] = original_val;
				m_dirty_ref_y[1] = true;
		});

		//Affine BG parameters
		mmio->AddRegister<u16>(0x20, false, true, &m_ctx.array[0x20], 0xFFFF);
		mmio->AddRegister<u16>(0x22, false, true, &m_ctx.array[0x22], 0xFFFF);
		mmio->AddRegister<u16>(0x24, false, true, &m_ctx.array[0x24], 0xFFFF);
		mmio->AddRegister<u16>(0x26, false, true, &m_ctx.array[0x26], 0xFFFF);

		mmio->AddRegister<u16>(0x30, false, true, &m_ctx.array[0x30], 0xFFFF);
		mmio->AddRegister<u16>(0x32, false, true, &m_ctx.array[0x32], 0xFFFF);
		mmio->AddRegister<u16>(0x34, false, true, &m_ctx.array[0x34], 0xFFFF);
		mmio->AddRegister<u16>(0x36, false, true, &m_ctx.array[0x36], 0xFFFF);

		//Windows
		mmio->AddRegister<u16>(0x40, false, true, &m_ctx.array[0x40], 0xFFFF);
		mmio->AddRegister<u16>(0x42, false, true, &m_ctx.array[0x42], 0xFFFF);

		mmio->AddRegister<u16>(0x44, false, true, &m_ctx.array[0x44], 0xFFFF);
		mmio->AddRegister<u16>(0x46, false, true, &m_ctx.array[0x46], 0xFFFF);

		mmio->AddRegister<u16>(0x48, true, true, &m_ctx.array[0x48], 0xFFFF);
		mmio->AddRegister<u16>(0x4A, true, true, &m_ctx.array[0x4A], 0xFFFF);

		//Mosaic
		mmio->AddRegister<u32>(0x4C, false, true, &m_ctx.array[0x4C], 0xFFFF'FFFF);

		//Color special effects
		mmio->AddRegister<u16>(0x50, true, true, &m_ctx.array[0x50], 0xFFFF);
		mmio->AddRegister<u16>(0x52, true, true, &m_ctx.array[0x52], 0xFFFF);
		mmio->AddRegister<u16>(0x54, false, true, &m_ctx.array[0x54], 0xFFFF);
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

		//////////////////////////////////

		m_saved_internal_reference_x[0] = m_internal_reference_x[0];
		m_saved_internal_reference_y[0] = m_internal_reference_y[0];
		m_saved_internal_reference_x[1] = m_internal_reference_x[1];
		m_saved_internal_reference_y[1] = m_internal_reference_y[1];
	}

	void PPU::RenderScanline(u16 lcd_y) {
		u8 mode = m_ctx_saved.m_control & 0x7;
		m_obj_window_pixels = {};

		switch (mode)
		{
		case 0:
			Mode0(lcd_y);
			break;

		case 1:
			Mode1(lcd_y);
			break;

		case 2:
			Mode2(lcd_y);
			break;

		case 3:
			Mode3(lcd_y);
			break;

		case 4:
			Mode4(lcd_y);
			break;

		case 5:
			Mode5(lcd_y);
			break;

		default:
			LOG_ERROR("Invalid display mode {0}!", (unsigned)mode);
			error::DebugBreak();
			break;
		}
	}

	void PPU::EnableThreadedRender() {
		m_pal_buf = new u8[0x400];
		m_oam_buf = new u8[0x400];

		std::fill_n(m_pal_buf, 0x400, 0x0);
		std::fill_n(m_oam_buf, 0x400, 0x0);

		m_render_thread.StartRenderThread();
	}

	PPU::~PPU() {
		bool was_running = m_render_thread.m_running;
		m_render_thread.StopRenderThread();

		if (was_running) {
			delete[] m_pal_buf;
			delete[] m_oam_buf;
		}

		delete[] m_palette_ram;
		delete[] m_vram;
		delete[] m_framebuffer;
		delete[] m_oam;
	}

	//////////////////////////////////////////

	u32 PPU::ReadRegister32(u8 offset) const {
		return reinterpret_cast<u32 const*>(m_ctx.array)[offset];
	}

	u16 PPU::ReadRegister16(u8 offset) const {
		return reinterpret_cast<u16 const*>(m_ctx.array)[offset];
	}

	u8 PPU::ReadRegister8(u8 offset) const {
		return m_ctx.array[offset];
	}

	u32 PPU::ReadSavedRegister32(u8 offset) const {
		return reinterpret_cast<u32 const*>(m_ctx_saved.array)[offset];
	}

	u16 PPU::ReadSavedRegister16(u8 offset) const {
		return reinterpret_cast<u16 const*>(m_ctx_saved.array)[offset];
	}

	u8 PPU::ReadSavedRegister8(u8 offset) const {
		return m_ctx_saved.array[offset];
	}

	u8* PPU::DebuggerGetPalette() {
		return m_palette_ram;
	}

	u8* PPU::DebuggerGetVRAM() {
		return m_vram;
	}

	//////////////////////////////////////

	void PPU::RenderThread::StartScanline() {
		if (m_rendering_line) {
			fmt::println("[PPU] Already rendering scanline!");
			return;
		}

		m_rendering_line = true;

		std::unique_lock<std::mutex> _lk{m_rendering_mux};
		//Perform copies of registers/video memory
		m_ppu->CopyBufferedData(true);

		m_start_render.store(true);
		m_rendering_cv.notify_one();
	}

	void PPU::RenderThread::FinalizeScanline() {
		if (!m_rendering_line) {
			fmt::println("[PPU] Not rendering scanline!");
			return;
		}
		m_rendering_line = false;
	}

	void PPU::RenderThread::Wait() {
		std::unique_lock<std::mutex> _lk{m_render_end_mux};
		
		if (!m_frame_ready.load()) {
			m_render_end_cv.wait(_lk, [this]() { return m_frame_ready.load(); });
		}
		
		m_frame_ready.store(false);
		m_rendering_line = false;
	}

	void PPU::RenderThread::RenderLoop() {
		{
			std::unique_lock<std::mutex> _lk{m_render_end_mux};
			m_render_end_cv.notify_one();
		}

		m_frame_ready.store(true);

		u16 curr_line{ 0 };

		while (true) {
			{
				if (curr_line > 0 && 
					(
						m_ppu->m_line_has_changes[curr_line - 1] ||
						m_ppu->m_sync_all_lines
					)) {
					//fmt::println("[PPU] Line {} had changes in previous frame", curr_line);
					m_waiting_update.release();
					m_finished_update.acquire();
				}

				m_ppu->RenderScanline(curr_line);

				++curr_line;
			}

			if (curr_line == VISIBLE_LINES) { 
				curr_line = 0; 

				{
					std::unique_lock<std::mutex> _lk{ m_render_end_mux };
					m_frame_ready.store(true);
					m_render_end_cv.notify_one();
				}
				
				std::unique_lock<std::mutex> _lk{ m_rendering_mux }; 

				if (!m_start_render.load()) { 
					m_rendering_cv.wait(_lk, [this]() { return m_start_render.load(); }); 
				}
				m_start_render.store(false);

				if (m_stop.load()) {
					break;
				}
			}
		}
	}

	void PPU::RenderThread::StartRenderThread() {
		if (m_running)
			return;

		m_stop.store(false);
		m_running = true;
		m_render_thread = std::thread([this]() {
			this->RenderLoop();
		});
		
		//Wait for thread to have started
		std::unique_lock<std::mutex> _lk{m_render_end_mux};
		m_render_end_cv.wait(_lk);
	}

	void PPU::RenderThread::StopRenderThread() {
		if (!m_running)
			return;

		m_running = false;
		
		{
			std::unique_lock<std::mutex> _lk{ m_rendering_mux };
			m_stop.store(true);
			m_start_render.store(true);
			m_rendering_cv.notify_one();
		}

		m_render_thread.join();
	}

	void PPU::RenderThread::UpdateScanlineBatches() {
		std::unique_lock<std::mutex> _lk{ m_rendering_mux };

		if (m_ppu->m_enable_scanline_diff_threshold) {
			u8 diff_lines{ 0 };

			for (u8 curr_line = 0; curr_line < VISIBLE_LINES; curr_line++) {
				if (m_ppu->m_line_has_changes_buf[curr_line] !=
					m_ppu->m_line_has_changes[curr_line])
					++diff_lines;
			}

			m_ppu->m_sync_all_lines = diff_lines >= m_ppu->m_scanline_diff_threshold;
		}
		else {
			m_ppu->m_sync_all_lines = false;
		}

		std::copy_n(m_ppu->m_line_has_changes_buf, TOTAL_LINES, 
			m_ppu->m_line_has_changes); 
	}

	void PPU::RenderThread::CheckScanlineChanges() {
		static PPUContext prev_scanline{};

		auto ctx_temp1 = m_ppu->m_ctx;
		auto ctx_temp2 = m_ppu->m_ctx_saved;

		ctx_temp1.m_vcount = 0; ctx_temp2.m_vcount = 0;
		ctx_temp1.m_status = 0; ctx_temp2.m_status = 0;

		if (std::memcmp((void*)&ctx_temp1, (void*)&ctx_temp2, sizeof(PPUContext))) {
			if (std::memcmp((void*)&ctx_temp1, (void*)&prev_scanline, sizeof(PPUContext))) {
				m_ppu->m_line_has_changes_buf[m_ppu->m_ctx.m_vcount] = true;
				prev_scanline = ctx_temp1;
			}
		}
		else if (!m_ppu->m_dirty_oam && !m_ppu->m_dirty_pal) {
			m_ppu->m_line_has_changes_buf[m_ppu->m_ctx.m_vcount] = false;
		}
	}

	void PPU::RenderThread::SyncScanlineChanges() {
		if (m_ppu->m_ctx.m_vcount > 0 && 
			(
				m_ppu->m_line_has_changes[m_ppu->m_ctx.m_vcount - 1] ||
				m_ppu->m_sync_all_lines
			)) {
			m_waiting_update.acquire();
			m_ppu->CopyBufferedData(true);
			m_finished_update.release();
		}
	}

	///////////////////////////////////////////////

	void PPU::CopyBufferedData(bool multithread) {
		m_ctx_saved = m_ctx;

		if (m_dirty_ref_x[0]) {
			m_saved_internal_reference_x[0] = m_internal_reference_x[0];
		}

		if (m_dirty_ref_x[1]) {
			m_saved_internal_reference_x[1] = m_internal_reference_x[1];
		}

		////////////////

		if (m_dirty_ref_y[0]) { 
			m_saved_internal_reference_y[0] = m_internal_reference_y[0];
		}

		if (m_dirty_ref_y[1]) {
			m_saved_internal_reference_y[1] = m_internal_reference_y[1];
		}

		std::fill_n(m_dirty_ref_x, 2, false);
		std::fill_n(m_dirty_ref_y, 2, false);

		if (m_dirty_oam && m_render_thread.m_running) {
			std::copy_n(m_oam_buf, 0x400, m_oam);
		}

		if (m_dirty_pal && m_render_thread.m_running) {
			std::copy_n(m_pal_buf, 0x400, m_palette_ram);
		}

		if (m_dirty_vram && m_render_thread.m_running) {
			//if(m_ctx_saved.m_vcount != 0)
				//fmt::println("[PPU] Dirty VRAM");
		}

		m_dirty_oam = false;
		m_dirty_pal = false;
		m_dirty_vram = false;
	}
}