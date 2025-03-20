#include "../../ppu/PPU.hpp"
#include "../../common/Error.hpp"
#include "../../common/SSE.hpp"

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

	void PPU::Mode3(u16 lcd_y) {
		bool bg2_enable   = (m_ctx_saved.m_control >> 10) & 1;
		bool forced_blank = (m_ctx_saved.m_control >> 7) & 1;

		if (!bg2_enable)
			return;

		unsigned curr_line = lcd_y;

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

		bool obj_enable = (m_ctx_saved.m_control >> 12) & 1;

		m_line_data[4] = {};

		if (obj_enable) {
			DrawSprites(curr_line);
			bg2 = MergeBitmap(lcd_y, bg2, m_line_data[4]);
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

	void PPU::Mode3_Batch(u16 first_line, u16 last_line) {
		bool bg2_enable = (m_ctx_saved.m_control >> 10) & 1;
		bool forced_blank = (m_ctx_saved.m_control >> 7) & 1;

		if (!bg2_enable)
			return;

		unsigned framebuffer_pos = first_line * 240 * 3;

		if (forced_blank) {
			for (; first_line != last_line; first_line++) {
				std::fill_n(&m_framebuffer[framebuffer_pos], 240 * 3, 1.0f);
				framebuffer_pos += 240 * 3;
			}

			return;
		}

		for (; first_line != last_line; first_line++) {
			if constexpr (has_sse42()) {
				auto vram_pos = (first_line * 240 * mode3::PIXEL_SIZE);
				u16 packed_colors[8] = {};

				for (unsigned x = 0; x < 240; x += 8) {
					auto abs_vram_pos = vram_pos + x * mode3::PIXEL_SIZE;
					
					auto mm_packed_colors = _mm_loadu_epi16(&m_vram[abs_vram_pos]);
					_mm_storeu_epi16(packed_colors, mm_packed_colors);

					for (auto pixel = 0; pixel < 8; ++pixel) {
						u16 color_packed = packed_colors[pixel];

						u8 r = color_packed & 0x1F;
						u8 g = (color_packed >> 5) & 0x1F;
						u8 b = (color_packed >> 10) & 0x1F;

						m_framebuffer[framebuffer_pos + 0] = (float)r / 0x1F;
						m_framebuffer[framebuffer_pos + 1] = (float)g / 0x1F;
						m_framebuffer[framebuffer_pos + 2] = (float)b / 0x1F;
						framebuffer_pos += 3;
					}
				}
			}
			else {
				for (unsigned x = 0; x < 240; x++) {
					int tex_x = x;
					int tex_y = first_line;

					u32 vram_pos = tex_x * mode3::PIXEL_SIZE
						+ (tex_y * 240 * mode3::PIXEL_SIZE);

					u16 color_packed = *reinterpret_cast<u16*>(m_vram + vram_pos);

					u8 r = color_packed & 0x1F;
					u8 g = (color_packed >> 5) & 0x1F;
					u8 b = (color_packed >> 10) & 0x1F;

					m_framebuffer[framebuffer_pos + 0] = (float)r / 0x1F;
					m_framebuffer[framebuffer_pos + 1] = (float)g / 0x1F;
					m_framebuffer[framebuffer_pos + 2] = (float)b / 0x1F;
					framebuffer_pos += 3;
				}
			}
		}
	}
}