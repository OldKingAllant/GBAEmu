#pragma once

#include "../common/Logger.hpp"
#include "../common/BitManip.hpp"
#include "../common/Defs.hpp"

#include <array>

namespace GBA::memory {
	class MMIO;
	class InterruptController;
	class EventScheduler;
	class Bus;
}

namespace GBA::ppu {
	struct Pixel {
		bool is_obj;
		bool is_bld_enabled;
		GBA::common::i16 palette_id;
		GBA::common::u16 color;
		GBA::common::u8 priority;
	};

	enum class Mode {
		NORMAL,
		VBLANK,
		HBLANK
	};

	class PPU {
	public :
		PPU();

		common::u32 ReadRegister32(common::u8 offset) const;
		void WriteRegister32(common::u8 offset, common::u32 value);

		common::u16 ReadRegister16(common::u8 offset) const;
		void WriteRegister16(common::u8 offset, common::u16 value);

		common::u8 ReadRegister8(common::u8 offset) const;
		void WriteRegister8(common::u8 offset, common::u8 value);

		void ClockCycles(common::u32 num_cycles);

		void Mode0();
		void Mode1();
		void Mode2();
		void Mode3();
		void Mode4();
		void Mode5();

		void VBlank();
		void HBlank();
		void Normal();

		void SetMMIO(memory::MMIO* mmio, memory::Bus* bus) {
			InitHandlers(mmio);
			m_bus = bus;
		}

		template <typename Type>
		Type ReadPalette(common::u32 address) const {
			address /= sizeof(Type);

			return reinterpret_cast<Type const*>(m_palette_ram)[address];
		}

		//8 bit writes are not allowed
		template <typename Type>
		void WritePalette(common::u32 address, Type value) {
			address /= sizeof(Type);

			if constexpr (sizeof(Type) == 1) {
				address &= ~1;
				common::u16 new_val = value * 0x101;
				*reinterpret_cast<common::u16*>(m_palette_ram + address) = new_val;
			}
			else {
				reinterpret_cast<Type*>(m_palette_ram)[address] = value;
			}
		}

		template <typename Type>
		Type ReadVRAM(common::u32 address) const {
			address /= sizeof(Type);

			return reinterpret_cast<Type const*>(m_vram)[address];
		}

		//8 bit writes are not allowed
		template <typename Type>
		void WriteVRAM(common::u32 address, Type value) {
			address /= sizeof(Type);

			if constexpr (sizeof(Type) == 1) {
				common::u8 mode = m_ctx.m_control & 0x7;

				if (address < 0x10000 || (mode >= 3 && address < 0x14000)) {
					//Not OBJ, writes are not ignored
					address &= ~1;
					common::u16 new_val = value * 0x101;
					*reinterpret_cast<common::u16*>(m_vram + address) = new_val;
				}
			}
			else {
				reinterpret_cast<Type*>(m_vram)[address] = value;
			}
		}

		template <typename Type>
		Type ReadOAM(common::u32 address) {
			address /= sizeof(Type);

			return reinterpret_cast<Type*>(m_oam)[address];
		}

		template <typename Type>
		void WriteOAM(common::u32 address, Type value) {
			address /= sizeof(Type);

			if constexpr (sizeof(Type) != 1) {
				reinterpret_cast<Type*>(m_oam)[address] = value;
			}
			//Else ignore writes
		}

		bool HasFrame() {
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

		common::u8* DebuggerGetPalette();
		common::u8* DebuggerGetVRAM();

	private:
		void InitHandlers(memory::MMIO* mmio);

		void ResetFrameData();

		std::array<Pixel, 240> DrawSprites(int lcd_y);

#include "ModeUtils.inl"

	private :
#pragma pack(push, 1)
		union PPUContext {
			struct {
				common::u16 m_control;
				common::u16 m_green_swap;
				common::u16 m_status;
				common::u16 m_vcount;
				common::u16 m_bg0_cnt;
				common::u16 m_bg1_cnt;
				common::u16 m_bg2_cnt;
				common::u16 m_bg3_cnt;
			};
			
			common::u8 array[0x58];
		};
#pragma pack(pop)
		
		PPUContext m_ctx;

		common::u32 m_mode_cycles;
		
		Mode m_curr_mode;

		common::u8* m_palette_ram;
		common::u8* m_vram;
		common::u8* m_oam;

		float* m_framebuffer;

		common::u32 m_internal_reference_x[2];
		common::u32 m_internal_reference_y[2];

		bool m_frame_ok;

		memory::InterruptController* m_int_control;
		memory::EventScheduler* m_sched;

		uint64_t m_last_event_timestamp;

		memory::Bus* m_bus;

		common::u16 line_sprites_ids[128];
		common::u8 line_sprites_count;

		static constexpr common::u32 CYCLES_PER_PIXEL = 4;
		static constexpr common::u32 CYCLES_PER_SCANLINE = 960;
		static constexpr common::u32 CYCLES_BEFORE_HBLANK_FLAG = 46;
		static constexpr common::u32 CYCLES_PER_HBLANK = 272;
		static constexpr common::u32 TOTAL_CYCLES_PER_LINE =
			CYCLES_PER_SCANLINE + CYCLES_PER_HBLANK;

		static constexpr common::u32 VISIBLE_LINES = 160;
		static constexpr common::u32 TOTAL_LINES = 228;

		static constexpr common::u32 PALETTE_SIZE = 512;

		static constexpr common::u32 BG_PALETTE_START = 0x0;
		static constexpr common::u32 OBJ_PALETTE_START = 0x200;
	};
}