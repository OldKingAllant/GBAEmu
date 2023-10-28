#include "../../ppu/PPU.hpp"
#include "../../common/Error.hpp"

namespace GBA::ppu {
	using namespace common;

	namespace mode3 {
		static constexpr u32 REG_REF_X = 0x28;
		static constexpr u32 REG_REF_Y = 0x2C;
		static constexpr u32 PIXEL_SIZE = 2;
		static constexpr u32 FRAME_SIZE = 0x12BFF;
		static constexpr u32 BG2_CNT = 0xC;
		static constexpr u32 FRAME_W = 240;
		static constexpr u32 FRAME_H = 160;
	}

	void PPU::Mode3() {
		bool bg2_enable = (m_ctx.m_control >> 10) & 1;
		bool forced_blank = (m_ctx.m_control >> 7) & 1;

		if (!bg2_enable)
			return;

		unsigned curr_line = m_ctx.m_vcount;

		if (curr_line >= 160) {
			error::DebugBreak();
		}

		unsigned framebuffer_y = curr_line * 240 * 3;

		if (forced_blank) {
			for (unsigned x = 0; x < 240; x++) {
				m_framebuffer[framebuffer_y + x * 3] = 1.0f;
				m_framebuffer[framebuffer_y + x * 3 + 1] = 1.0f;
				m_framebuffer[framebuffer_y + x * 3 + 2] = 1.0f;
			}

			return;
		}

		std::array<Pixel, 240> bg2{};

		for (unsigned x = 0; x < 240; x++) {
			int tex_x = x;
			int tex_y = curr_line;

			if (tex_x < mode3::FRAME_W && tex_y < mode3::FRAME_H &&
				tex_x >= 0 && tex_y >= 0) {
				u32 vram_pos = tex_x * mode3::PIXEL_SIZE
					+ (tex_y * 240 * mode3::PIXEL_SIZE);

				bg2[x].color = *reinterpret_cast<u16*>(m_vram + vram_pos);
				bg2[x].palette_id = 1;
			}
		}

		bool obj_enable = (m_ctx.m_control >> 12) & 1;

		if (obj_enable) {
			auto sprites = DrawSprites(curr_line);
			bg2 = MergeBitmap(bg2, sprites);
		}

		for (unsigned x = 0; x < 240; x++) {
			u16 color_packed = bg2[x].color;

			u8 r = color_packed & 0x1F;
			u8 g = (color_packed >> 5) & 0x1F;
			u8 b = (color_packed >> 10) & 0x1F;

			m_framebuffer[framebuffer_y + x * 3] = (float)r / 0x1F;
			m_framebuffer[framebuffer_y + x * 3 + 1] = (float)g / 0x1F;
			m_framebuffer[framebuffer_y + x * 3 + 2] = (float)b / 0x1F;
		}
	}
}