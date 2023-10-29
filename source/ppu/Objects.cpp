#include "../../ppu/PPU.hpp"

#include "../../common/Error.hpp"

namespace GBA::ppu {
	using namespace common;

	namespace detail {
		//Index 1: shape
		//Index 2: size id
		//Index 3: x/y
		u16 obj_sizes[][4][2] = {
			{
				{ 8, 8 },
				{ 16, 16 },
				{ 32, 32 },
				{ 64, 64 }
			}, 
			{
				{ 16, 8 },
				{ 32, 8 },
				{ 32, 16 },
				{ 64, 32 }
			},
			{
				{ 8, 16 },
				{ 8, 32 },
				{ 16, 32 },
				{ 32, 64 }
			}
		};
	}

	u16 line_sprites_ids[128] = {};
	u8 line_sprites_count = 0;

#define READ_16(arr, index) *reinterpret_cast<u16*>(arr + index)
#define READ_32(arr, index) *reinterpret_cast<u32*>(arr + index)

	std::array<Pixel, 240> PPU::DrawSprites(int lcd_y) {
		line_sprites_count = 0;

		std::array<Pixel, 240> obj_data{};

		/*
		We can select and draw immediately the
		sprites, but I won't do that, since
		putting them in a list based on position
		in OAM means that sprites that overlap
		can be managed easily by drawing 
		before the sprites with lower
		priority
		*/
		for (u16 index = 0; index < 0x3FF; index += 8) {
			u16 attr_0 = READ_16(m_oam, index);

			int y_coord = attr_0 & 0xFF;
			u8 shape = (attr_0 >> 14) & 0x3;

			if (shape == 3)
				continue;

			bool disabled = !CHECK_BIT(attr_0, 8) && CHECK_BIT(attr_0, 9);

			if (disabled)
				continue;

			u16 attr_1 = READ_16(m_oam, index + 2);

			u8 size_type = (attr_1 >> 14) & 0x3;

			u16 y_size = detail::obj_sizes[shape][size_type][1];

			u16 end_y = y_coord + y_size;

			if (CHECK_BIT(attr_0, 8) && CHECK_BIT(attr_0, 9)) {
				end_y = y_coord + y_size * 2;
			}

			if (end_y >= 256) {
				y_coord -= 256;
				end_y -= 256;
			}

			if (lcd_y >= y_coord && lcd_y < end_y) {
				line_sprites_ids[line_sprites_count++] = index;
			}
		}

		u32 OBJ_VRAM_BASE = 0x10000;

		bool addressing_mode = CHECK_BIT(m_ctx.m_control, 6);

		u32 mos_cnt = ReadRegister32(0x4C / 4);

		u32 mos_h = ((mos_cnt >> 8) & 0xF) + 1;
		u32 mos_v = ((mos_cnt >> 12) & 0xF) + 1;

		for (int pos = line_sprites_count - 1; pos >= 0; pos--) {
			u16 index = line_sprites_ids[pos];

			u16 attr_0 = READ_16(m_oam, index);
			u16 attr_1 = READ_16(m_oam, index + 2);
			u16 attr_2 = READ_16(m_oam, index + 4);

			u8 mode = (attr_0 >> 10) & 0x3;

			if (mode > 1) {
				error::DebugBreak();
			}

			u8 shape = (attr_0 >> 14) & 0x3;
			u8 size_type = (attr_1 >> 14) & 0x3;

			int x_start = attr_1 & 0x1FF;

			if (x_start >= 256)
				x_start = x_start - 512;

			u32 x_size = detail::obj_sizes[shape][size_type][0];
			u32 y_size = detail::obj_sizes[shape][size_type][1];

			bool rot_scaling = CHECK_BIT(attr_0, 8);

			if (x_start >= 240 && !rot_scaling)
				continue;

			u32 tile_id = attr_2 & 1023;

			u8 ppu_mode = (m_ctx.m_control & 0x7);

			if (ppu_mode >= 3 && tile_id < 512)
				continue;

			u8 prio_to_bg = (attr_2 >> 10) & 0x3;
			u8 pal_number = (attr_2 >> 12) & 0xF;

			int y_coord = attr_0 & 0xFF;

			bool mosaic = CHECK_BIT(attr_0, 12);
			bool pal_mode = CHECK_BIT(attr_0, 13);

			u32 tile_size = 0x20;
			u32 line_size = 0x4;

			if (pal_mode) {
				tile_size = 0x40;
				line_size = 0x8;
			}

			u32 vram_y_offset = 0x0;
			u32 total_x_tiles = x_size / 8;
			u32 total_y_tiles = y_size / 8;

			u32 start_offset = tile_id * 0x20;

			u32 start_tile_y_id = tile_id / 32;
			u32 start_tile_x_id = tile_id % 32;

			int end_y = y_coord + y_size;

			if (end_y >= 256) {
				end_y -= 256;
				y_coord -= 256;
			}

			u32 mos_h_size = 1;
			u32 mos_v_size = 1;

			if (mosaic) {
				mos_h_size = mos_h;
				mos_v_size = mos_v;
			}

			u32 transformed_y = lcd_y - (lcd_y % mos_v_size);

			if (!rot_scaling) {
				u32 tex_y = transformed_y - y_coord;

				if (tex_y > y_size)
					tex_y = 0;

				bool h_flip = CHECK_BIT(attr_1, 12);
				bool v_flip = CHECK_BIT(attr_1, 13);

				if (v_flip)
					tex_y = end_y - transformed_y - 1;

				u32 tile_y = tex_y / 8;
				u32 y_offset = tex_y % 8;

				u32 end = x_start + x_size;

				u32 vram_tile_y_id = 0;

				if (addressing_mode)
					vram_tile_y_id = tile_y * total_x_tiles;
				else
					vram_tile_y_id = tile_y * 32;

				u32 vram_y_offset = (vram_tile_y_id * 0x20) +
					(y_offset * line_size);

				for (u32 x = x_start < 0 ? 0 : x_start; x < end &&
					x < 240; x++) {
					u32 transformed_x = x - (x % mos_h_size);

					u32 tex_x = transformed_x - x_start;

					if (tex_x > x_size)
						tex_x = 0;

					if (h_flip)
						tex_x = end - transformed_x - 1;

					u32 tile_x = tex_x / 8;
					u32 x_offset = tex_x % 8;

					if (!pal_mode)
						x_offset /= 2;

					u32 vram_offset = start_offset + vram_y_offset
						+ (tile_x * 0x20)
						+ x_offset;

					vram_offset %= 0x8000;

					u16 color_id = m_vram[OBJ_VRAM_BASE
						+ vram_offset];
					

					u16 color = 0;

					if (pal_mode) {
						color = READ_16(m_palette_ram,
							color_id * 2 + OBJ_PALETTE_START);
					}
					else {
						u16 pixel_id = 0;

						if (tex_x % 2)
							pixel_id = (color_id >> 4) & 0xF;
						else
							pixel_id = color_id & 0xF;

						color_id = pixel_id;

						pixel_id *= 2;
						pixel_id += pal_number * 32;

						color = READ_16(m_palette_ram,
							pixel_id + OBJ_PALETTE_START);
					}

					if (color_id) {
						obj_data[x].is_obj = true;
						obj_data[x].priority = prio_to_bg;
						obj_data[x].palette_id = color_id;
						obj_data[x].color = color;
						obj_data[x].is_bld_enabled = mode == 1;
					}
				}
			}
			else {
				bool double_size = CHECK_BIT(attr_0, 9);

				u32 parameter_sel = (attr_1 >> 9) & 0x1F;

				u32 group_offset = parameter_sel * 0x20;

				i16 dx = READ_16(m_oam, group_offset + 0x6);
				i16 dmx = READ_16(m_oam, group_offset + 0xE);
				i16 dy = READ_16(m_oam, group_offset + 0x16);
				i16 dmy = READ_16(m_oam, group_offset + 0x1E);

				u32 orig_x_size = x_size;
				u32 orig_y_size = y_size;

				if (double_size) {
					y_size *= 2;
					x_size *= 2;
				}

				i32 y_center = y_size / 2;
				i32 x_center = x_size / 2;

				i32 local_x = x_start <0 ? -x_center + -x_start : -x_center;
				i32 local_y = transformed_y - (y_coord + y_center);

				i32 tex_y_base = local_y * dmy;
				i32 tex_x_base = local_y * dmx;

				u32 end_x = x_start + x_size;

				for (u32 x = x_start < 0 ? 0 : x_start; x < end_x && x < 240; x++) {
					i32 curr_x = tex_x_base + dx * local_x + (x_size << 7);
					i32 curr_y = tex_y_base + dy * local_x + (y_size << 7);

					local_x++;

					i32 tex_x = curr_x >> 8;
					i32 tex_y = curr_y >> 8;

					if (double_size) {
						tex_x -= orig_x_size / 2;
						tex_y -= orig_y_size / 2;
					}

					if (tex_x < 0 || tex_y < 0) {
						continue;
					}

					if (tex_x >= (i32)orig_x_size || tex_y >= (i32)orig_y_size) {
						continue;
					}

					u32 tile_y = tex_y / 8;
					u32 y_offset = tex_y % 8;

					u32 vram_tile_y_id = 0;

					if (addressing_mode)
						vram_tile_y_id = tile_y * total_x_tiles;
					else
						vram_tile_y_id = tile_y * 32;

					u32 vram_y_offset = (vram_tile_y_id * 0x20) +
						(y_offset * line_size);

					u32 tile_x = tex_x / 8;
					u32 x_offset = tex_x % 8;

					if (!pal_mode)
						x_offset /= 2;

					u32 vram_offset = start_offset + vram_y_offset
						+ (tile_x * 0x20)
						+ x_offset;

					vram_offset %= 0x8000;

					u16 color_id = m_vram[OBJ_VRAM_BASE
						+ vram_offset];

					u16 color = 0;

					if (pal_mode) {
						color = READ_16(m_palette_ram,
							color_id * 2 + OBJ_PALETTE_START);
					}
					else {
						u16 pixel_id = 0;

						if (tex_x % 2)
							pixel_id = (color_id >> 4) & 0xF;
						else
							pixel_id = color_id & 0xF;

						color_id = pixel_id;

						pixel_id *= 2;
						pixel_id += pal_number * 32;

						color = READ_16(m_palette_ram,
							pixel_id + OBJ_PALETTE_START);
					}

					if (color_id) {
						obj_data[x].is_obj = true;
						obj_data[x].priority = prio_to_bg;
						obj_data[x].palette_id = color_id;
						obj_data[x].color = color;
						obj_data[x].is_bld_enabled = mode == 1;
					}
				}
			}
		}

		return obj_data;
	}
}