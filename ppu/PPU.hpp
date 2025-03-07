#pragma once

#include "../common/Logger.hpp"
#include "../common/BitManip.hpp"
#include "../common/Defs.hpp"

#include <array>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <semaphore>

namespace GBA::memory {
	class MMIO;
	class InterruptController;
	class EventScheduler;
	class Bus;
}

namespace GBA::ppu {
	struct Pixel {
		bool is_present;
		bool is_bld_enabled;
		GBA::common::i16 palette_id;
		GBA::common::u16 color;
		GBA::common::u8 priority;

		template <typename Ar>
		void save(Ar& ar) const {
			ar(is_present);
			ar(is_bld_enabled);
			ar(palette_id);
			ar(color);
			ar(priority);
		}

		template <typename Ar>
		void load(Ar& ar) {
			ar(is_present);
			ar(is_bld_enabled);
			ar(palette_id);
			ar(color);
			ar(priority);
		}
	};

	enum class Mode {
		NORMAL,
		VBLANK,
		HBLANK
	};

	using namespace common;

	class PPU {
	public :
		static constexpr u32 CYCLES_PER_PIXEL = 4;
		static constexpr u32 CYCLES_PER_SCANLINE = 960;
		static constexpr u32 CYCLES_BEFORE_HBLANK_FLAG = 46;
		static constexpr u32 CYCLES_PER_HBLANK = 272;
		static constexpr u32 TOTAL_CYCLES_PER_LINE =
			CYCLES_PER_SCANLINE + CYCLES_PER_HBLANK;

		static constexpr u32 VISIBLE_LINES = 160;
		static constexpr u32 TOTAL_LINES = 228;

		static constexpr u32 PALETTE_SIZE = 512;

		static constexpr u32 BG_PALETTE_START = 0x0;
		static constexpr u32 OBJ_PALETTE_START = 0x200;

		PPU();

		u32 ReadRegister32(u8 offset) const;
		u16 ReadRegister16(u8 offset) const;
		u8 ReadRegister8(u8 offset) const;
		
		u32 ReadSavedRegister32(u8 offset) const;
		u16 ReadSavedRegister16(u8 offset) const;
		u8 ReadSavedRegister8(u8 offset) const;

		void Mode0(u16 lcd_y);
		void Mode1(u16 lcd_y);
		void Mode2(u16 lcd_y);
		void Mode3(u16 lcd_y);
		void Mode4(u16 lcd_y);
		void Mode5(u16 lcd_y);

		void RenderScanline(u16 lcd_y);

		void SetMMIO(memory::MMIO* mmio, memory::Bus* bus) {
			InitHandlers(mmio);
			m_bus  = bus;
			m_mmio = mmio;
		}

		template <typename Type>
		Type ReadPalette(u32 address) const {
			address /= sizeof(Type);

			if (m_render_thread.m_running) {
				return reinterpret_cast<Type const*>(m_pal_buf)[address];
			}

			return reinterpret_cast<Type const*>(m_palette_ram)[address];
		}

		//8 bit writes are not allowed
		template <typename Type>
		void WritePalette(u32 address, Type value) {
			address /= sizeof(Type);

			if constexpr (sizeof(Type) == 1) {
				address &= ~1;
				u16 new_val = value * 0x101;

				if (m_render_thread.m_running) {
					*reinterpret_cast<u16*>(m_pal_buf + address) = new_val;
				}
				else {
					*reinterpret_cast<u16*>(m_palette_ram + address) = new_val;
				}
				
			}
			else {

				if (m_render_thread.m_running) {
					reinterpret_cast<Type*>(m_pal_buf)[address] = value;
				}
				else {
					reinterpret_cast<Type*>(m_palette_ram)[address] = value;
				}
				
			}

			m_dirty_pal = true;

			if (m_ctx.m_vcount < VISIBLE_LINES) {
				m_line_has_changes_buf[m_ctx.m_vcount] = true;
			}
			
		}

		template <typename Type>
		Type ReadVRAM(u32 address) const {
			address /= sizeof(Type);

			return reinterpret_cast<Type const*>(m_vram)[address];
		}

		//8 bit writes are not allowed
		template <typename Type>
		void WriteVRAM(u32 address, Type value) {
			address /= sizeof(Type);

			if constexpr (sizeof(Type) == 1) {
				u8 mode = m_ctx.m_control & 0x7;

				if (address < 0x10000 || (mode >= 3 && address < 0x14000)) {
					//Not OBJ, writes are not ignored
					address &= ~1;
					u16 new_val = value * 0x101;
					*reinterpret_cast<u16*>(m_vram + address) = new_val;
				}
			}
			else {
				reinterpret_cast<Type*>(m_vram)[address] = value;
			}

			m_dirty_vram = true;
		}

		template <typename Type>
		Type ReadOAM(u32 address) {
			address /= sizeof(Type);

			if (m_render_thread.m_running) {
				return reinterpret_cast<Type*>(m_oam_buf)[address];
			}

			return reinterpret_cast<Type*>(m_oam)[address];
		}

		template <typename Type>
		void WriteOAM(u32 address, Type value) {
			address /= sizeof(Type);

			if constexpr (sizeof(Type) != 1) {
				m_dirty_oam = true;

				if (m_render_thread.m_running) {
					reinterpret_cast<Type*>(m_oam_buf)[address] = value;
				}
				else {
					reinterpret_cast<Type*>(m_oam)[address] = value;
				}

				if (m_ctx.m_vcount < VISIBLE_LINES) {
					m_line_has_changes_buf[m_ctx.m_vcount] = true;
				}
			}
			
			//Else ignore writes
		}

		bool HasFrame() const {
			return m_frame_ok;
		}

		float* GetFrame() {
			m_frame_ok = false;
			return m_framebuffer;
		}

		void SetInterruptController(memory::InterruptController* int_controller);
		void SetScheduler(memory::EventScheduler* sched);

		friend void HblankEventCallback(void* ppu_ptr);
		friend void NormalEventCallback(void* ppu_ptr);
		friend void VblankEventCallback(void* ppu_ptr);
		friend void ScanlineEventCallback(void* ppu_ptr);
		friend void VblankHblankCallback(void* ppu_ptr);
		friend void VblankEndCallback(void* ppu_ptr);

		~PPU();

		u8* DebuggerGetPalette();
		u8* DebuggerGetVRAM();

		void EnableThreadedRender();

		inline bool IsThreadedRenderingEnabled() const {
			return m_render_thread.m_running;
		}

		template <typename Ar>
		void save(Ar& ar) const {
			using namespace common;
			
			std::vector<u8> palette_temp{};
			std::vector<u8> vram_temp{};
			std::vector<u8> oam_temp{};
			std::vector<float> framebuf_temp{};

			palette_temp.resize(0x400);
			vram_temp.resize(0x18000);
			oam_temp.resize(0x400);
			framebuf_temp.resize(size_t(240) * 160 * 3);

			std::copy_n(m_palette_ram, 0x400, palette_temp.begin());
			std::copy_n(m_vram, 0x18000, vram_temp.begin());
			std::copy_n(m_oam, 0x400, oam_temp.begin());
			std::copy_n(m_framebuffer, framebuf_temp.size(), framebuf_temp.begin());

			ar(m_ctx.array);
			ar(m_mode_cycles);
			ar(m_curr_mode);

			ar(palette_temp);
			ar(vram_temp);
			ar(oam_temp);
			ar(framebuf_temp);

			ar(m_internal_reference_x);
			ar(m_internal_reference_y);

			ar(m_frame_ok);

			ar(m_last_event_timestamp);

			ar(line_sprites_ids);
			ar(line_sprites_count);
			ar(m_line_data);
			ar(m_obj_window_pixels);
		}

		template <typename Ar>
		void load(Ar& ar) {
			using namespace common;

			std::vector<u8> palette_temp{};
			std::vector<u8> vram_temp{};
			std::vector<u8> oam_temp{};
			std::vector<float> framebuf_temp{};

			palette_temp.resize(0x400);
			vram_temp.resize(0x18000);
			oam_temp.resize(0x400);
			framebuf_temp.resize(size_t(240) * 160 * 3);

			ar(m_ctx.array);
			ar(m_mode_cycles);
			ar(m_curr_mode);

			ar(palette_temp);
			ar(vram_temp);
			ar(oam_temp);
			ar(framebuf_temp);

			ar(m_internal_reference_x);
			ar(m_internal_reference_y);

			ar(m_frame_ok);

			ar(m_last_event_timestamp);

			ar(line_sprites_ids);
			ar(line_sprites_count);
			ar(m_line_data);
			ar(m_obj_window_pixels);

			std::copy_n(palette_temp.begin(), 0x400, m_palette_ram);
			std::copy_n(vram_temp.begin(), 0x18000, m_vram);
			std::copy_n(oam_temp.begin(), 0x400, m_oam);
			std::copy_n(framebuf_temp.begin(), framebuf_temp.size(), m_framebuffer);

			m_ctx_saved = m_ctx;

			m_saved_internal_reference_x[0] = m_internal_reference_x[0];
			m_saved_internal_reference_y[0] = m_internal_reference_y[0];
			m_saved_internal_reference_x[1] = m_internal_reference_x[1];
			m_saved_internal_reference_y[1] = m_internal_reference_y[1];

			if (m_render_thread.m_running) {
				//Threaded renderer is enabled,
				//we need to make sure that 
				//buffered oam/pal are in sync
				//with the effective memory
				//Same for all other things

				//First we need to stop the thread,
				//just to make sure that the order
				//of execution is always the same
				//and we avoid deadlocks
				m_render_thread.StopRenderThread();

				//Then we copy the data
				std::copy_n(m_oam, 0x400, m_oam_buf);
				std::copy_n(m_palette_ram, 0x400, m_pal_buf);

				//Reset scanline changes
				std::fill_n(m_line_has_changes_buf, TOTAL_LINES, false);
				std::fill_n(m_line_has_changes, TOTAL_LINES, false);

				//Finally we restart the thread
				m_render_thread.StartRenderThread();
			}
		}

#pragma pack(push, 1)
		union PPUContext {
			struct {
				u16 m_control;
				u16 m_green_swap;
				u16 m_status;
				u16 m_vcount;
				u16 m_bg0_cnt;
				u16 m_bg1_cnt;
				u16 m_bg2_cnt;
				u16 m_bg3_cnt;
			};

			u8 array[0x58];
		};
#pragma pack(pop)

		struct RenderThread {
			bool m_running;
			std::thread				m_render_thread;
			std::mutex				m_rendering_mux;
			std::condition_variable m_rendering_cv;
			std::mutex				m_render_end_mux;
			std::condition_variable m_render_end_cv;
			std::atomic_bool        m_stop;
			std::atomic_bool        m_frame_ready;
			std::atomic_bool        m_start_render;
			PPU*					m_ppu;
			bool                    m_rendering_line;

			std::binary_semaphore m_waiting_update = std::binary_semaphore{0};
			std::binary_semaphore m_finished_update = std::binary_semaphore{ 0 };

			void RenderLoop();
			void StopRenderThread();
			void StartRenderThread();

			void StartScanline();
			void FinalizeScanline();
			void Wait();
			void CheckScanlineChanges();
			void SyncScanlineChanges();
			void UpdateScanlineBatches();
		};

		inline void SetEnableDiffThreshold(bool enable_diff) {
			m_enable_scanline_diff_threshold = enable_diff;
		}

		inline void SetDiffThreshold(u8 threshold) {
			m_scanline_diff_threshold = threshold;
		}

		inline bool IsDiffThresholdEnabled() const {
			return m_enable_scanline_diff_threshold;
		}

		inline u8 GetDiffThreshold() const {
			return m_scanline_diff_threshold;
		}

	private:
		void InitHandlers(memory::MMIO* mmio);
		void ResetFrameData();
		void DrawSprites(int lcd_y);

		void CopyBufferedData(bool multithread);

#include "ModeUtils.inl"

	private :
		PPUContext m_ctx, m_ctx_saved;

		u32 m_mode_cycles;
		
		Mode m_curr_mode;

		u8* m_palette_ram;
		u8* m_vram;
		u8* m_oam;

		float* m_framebuffer;

		u32 m_internal_reference_x[2];
		u32 m_internal_reference_y[2];

		u32 m_saved_internal_reference_x[2];
		u32 m_saved_internal_reference_y[2];

		bool m_dirty_ref_x[2], m_dirty_ref_y[2];

		bool m_frame_ok;

		memory::InterruptController* m_int_control;
		memory::EventScheduler* m_sched;

		uint64_t m_last_event_timestamp;

		memory::Bus* m_bus;
		memory::MMIO* m_mmio;

		u16 line_sprites_ids[128];
		u8 line_sprites_count;

		std::array<Pixel, 240> m_line_data[5];
		std::array<bool, 240> m_obj_window_pixels;

		RenderThread m_render_thread;

		bool m_dirty_oam;
		bool m_dirty_pal;
		bool m_dirty_vram;

		u8* m_pal_buf;
		u8* m_oam_buf;

		bool m_line_has_changes_buf[TOTAL_LINES];
		bool m_line_has_changes[TOTAL_LINES];

		//Enable a threshold s.t. when
		//the number of lines where PPU
		//configuration changes (intra-scanline)
		//is above said threshold, the next frame
		//will have the renderer sync on all scanlines.
		//This is for cases where circle-shaped windows
		//that change size/position between frames,
		//which would result in incorrect rendering
		//without it
		bool m_enable_scanline_diff_threshold;
		u8 m_scanline_diff_threshold;
		bool m_sync_all_lines;
	};
}