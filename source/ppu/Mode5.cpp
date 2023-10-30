#include "../../ppu/PPU.hpp"
#include "../../common/Error.hpp"

namespace GBA::ppu {
	using namespace common;

	namespace mode5 {
		static constexpr u32 REG_REF_X = 0x28;
		static constexpr u32 REG_REF_Y = 0x2C;
		static constexpr u32 PIXEL_SIZE = 2;
		static constexpr u32 FRAME_0_START = 0x0;
		static constexpr u32 FRAME_1_START = 0xA000;
		static constexpr u32 FRAME_SIZE = 0x9FFF;
		static constexpr u32 BG2_CNT = 0xC;
		static constexpr u32 FRAME_W = 160;
		static constexpr u32 FRAME_H = 128;
	}

	void PPU::Mode5() {
		bool bg2_enable = (m_ctx.m_control >> 10) & 1;
		bool forced_blank = (m_ctx.m_control >> 7) & 1;
		bool frame_select = (m_ctx.m_control >> 4) & 1;

		bool mosaic = (ReadRegister16(mode5::BG2_CNT / 2) >> 6) & 1;

		if (!bg2_enable)
			return;

		unsigned curr_line = m_ctx.m_vcount;

		if (curr_line >= 160) {
			error::DebugBreak();
		}

		unsigned framebuffer_y = curr_line * 240 * 3;

		u32 vram_offset = 0;

		if (frame_select)
			vram_offset = mode5::FRAME_1_START;

		if (forced_blank) {
			for (unsigned x = 0; x < 240; x++) {
				m_framebuffer[framebuffer_y + x * 3] = 1.0f;
				m_framebuffer[framebuffer_y + x * 3 + 1] = 1.0f;
				m_framebuffer[framebuffer_y + x * 3 + 2] = 1.0f;
			}

			return;
		}

		for (unsigned x = 0; x < 240; x++) {
			int tex_x = x;
			int tex_y = curr_line;

			if (mosaic)
				CalculateMosaicBG(tex_x, tex_y);

			u16 color_packed = m_palette_ram[BG_PALETTE_START];
			color_packed |= ((u16)m_palette_ram[BG_PALETTE_START + 1] << 8);

			if (tex_x < mode5::FRAME_W && tex_y < mode5::FRAME_H &&
				tex_x >= 0 && tex_y >= 0) {
				u32 vram_pos = tex_x * mode5::PIXEL_SIZE
					+ (tex_y * 160 * mode5::PIXEL_SIZE);

				color_packed = *reinterpret_cast<u16*>(m_vram + vram_pos);
			}

			u8 r = color_packed & 0x1F;
			u8 g = (color_packed >> 5) & 0x1F;
			u8 b = (color_packed >> 10) & 0x1F;

			m_framebuffer[framebuffer_y + x * 3] = (float)r / 0x1F;
			m_framebuffer[framebuffer_y + x * 3 + 1] = (float)g / 0x1F;
			m_framebuffer[framebuffer_y + x * 3 + 2] = (float)b / 0x1F;
		}
	}
}