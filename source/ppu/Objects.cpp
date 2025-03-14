#include "../../ppu/PPU.hpp"

#include "../../common/Error.hpp"

#include <iostream>

namespace GBA::ppu {
	using namespace common;

	namespace detail {
		//Index 1: shape
		//Index 2: size id
		//Index 3: x/y
		static constexpr u16 obj_sizes[][4][2] = {
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

#define READ_16(arr, index) *reinterpret_cast<u16*>(arr + index)
#define READ_32(arr, index) *reinterpret_cast<u32*>(arr + index)

	void PPU::DrawSprites(int lcd_y) {
		line_sprites_count = 0;

		/*
		We can select and draw immediately the
		sprites, but I won't do that, since
		putting them in a list based on position
		in OAM means that sprites that overlap
		can be managed easily by drawing 
		before the sprites with lower
		priority
		*/
		constexpr u32 OAM_SIZE       = 0x400;
		constexpr u32 OBJ_VRAM_BASE  = 0x10000;
		constexpr u32 OAM_OBJ_STRIDE = 0x8;
		constexpr u32 OAM_OBJ_SHIFT  = 0x3;
		constexpr u32 TOTAL_OBJS	 = OAM_SIZE / OAM_OBJ_STRIDE;

		auto oam_objs = std::bit_cast<OAMEntry*>(m_oam);

		for (u16 index = 0; index < TOTAL_OBJS; index++) {
			//Just copy the object, it is only 8 bytes
			auto curr_obj = oam_objs[index];

			auto y_coord = int(curr_obj.coord_y);
			auto shape   = curr_obj.shape;

			if (shape == OAMEntryShape::INVALID) [[unlikely]]
				continue;

			//If the object is not affine and
			//the double size bit is one, then
			//the object is disabled
			bool disabled = !curr_obj.is_affine && curr_obj.double_sz;

			if (disabled)
				continue;

			auto size_type = curr_obj.obj_size_type;

			//Get Y size from table
			auto y_size = detail::obj_sizes[u8(shape)][size_type][1];
			//Compute last scanline
			auto end_y = y_coord + y_size;

			if (curr_obj.is_affine && curr_obj.double_sz) {
				end_y = y_coord + y_size * 2;
			}

			//If the object is double size, it might
			//go out of bounds, which results in
			//wraparound
			if (end_y >= 256 || y_coord >= 255) {
				y_coord -= 256;
				end_y -= 256;
			}

			//Is the current scanline inside the object?
			if (lcd_y >= y_coord && lcd_y < end_y) {
				//It is, add the object to the list
				//of objects to render
				line_sprites_ids[line_sprites_count++] = index << OAM_OBJ_SHIFT;
			}
		}

		//Tiles are accessed in two different ways:
		//1. A simple array, one after the other
		//2. 2D matrix
		bool addressing_mode = CHECK_BIT(m_ctx_saved.m_control, 6);

		u32 mos_cnt = ReadSavedRegister32(0x4C / 4);

		//Compute mosaic X/Y size
		u32 mos_h = ((mos_cnt >> 8) & 0xF) + 1;
		u32 mos_v = ((mos_cnt >> 12) & 0xF) + 1;

		for (int pos = line_sprites_count - 1; pos >= 0; pos--) {
			u16 index = line_sprites_ids[pos];
			auto curr_obj = oam_objs[index >> OAM_OBJ_SHIFT];

			auto mode       = curr_obj.type;
			bool obj_window = mode == OAMEntryType::WINDOW;

			auto shape     = curr_obj.shape;
			auto size_type = curr_obj.obj_size_type;

			auto x_start = int(curr_obj.coord_x_low);
			x_start		|= int(curr_obj.coord_x_high) << 8;

			//Wraparound
			if (x_start >= 256)
				x_start = x_start - 512;

			u32 x_size = detail::obj_sizes[u8(shape)][size_type][0];
			u32 y_size = detail::obj_sizes[u8(shape)][size_type][1];

			auto rot_scaling = curr_obj.is_affine;

			//non-affine objects that are out of bounds
			//have no chance of coming back inside
			//the visible lines
			if (x_start >= 240 && !rot_scaling)
				continue;

			auto tile_id = u32(curr_obj.tile_number_low);
			tile_id		|= u32(curr_obj.tile_number_high) << 8;

			u8 ppu_mode = (m_ctx_saved.m_control & 0x7);

			//In bitmap modes, cannot use first 512 tiles
			if (ppu_mode >= 3 && tile_id < 512) [[unlikely]]
				continue;

			auto prio_to_bg = curr_obj.priority_to_bg;
			auto pal_number = curr_obj.palette_num;

			auto y_coord = int(curr_obj.coord_y);

			auto mosaic   = curr_obj.is_mosaic;
			auto pal_mode = curr_obj.pal;

			const u32 PAL_STRIDE = pal_mode == OAMEntryPalette::PAL_256 ? 16 : 32;

			//Size of a normal tile in VRAM
			constexpr u32 TILE_SIZE_DEFAULT  = 32;
			//Size of a 256 colors tile in VRAM
			constexpr u32 TILE_SIZE_DOUBLE   = 64;
			//Size of a line of pixels in VRAM
			constexpr u32 TILE_LINE_SIZE_016 = 0x4;
			//Size of a 256 colors line of pixels in VRAM
			constexpr u32 TILE_LINE_SIZE_256 = 0x8;

			u32 tile_size = TILE_SIZE_DEFAULT;
			u32 line_size = TILE_LINE_SIZE_016;

			if (pal_mode == OAMEntryPalette::PAL_256) {
				tile_size = TILE_SIZE_DOUBLE;
				line_size = TILE_LINE_SIZE_256;
			}

			constexpr u32 TILE_X_SIZE_PIXELS = 8;
			constexpr u32 TILE_Y_SIZE_PIXELS = 8;
	

			//Total x/Y tiles spanned by the object

			u32 total_x_tiles = x_size / TILE_X_SIZE_PIXELS;
			u32 total_y_tiles = y_size / TILE_Y_SIZE_PIXELS;

			//Start offset in VRAM (address of first tile)
			u32 start_offset = tile_id * TILE_SIZE_DEFAULT;

			//Last scanline of object
			int end_y = y_coord + y_size;

			if (rot_scaling && curr_obj.double_sz) {
				end_y = y_coord + y_size * 2;
			}

			//If out of bounds, wraparound
			if (end_y >= 256 || y_coord >= 255) {
				end_y -= 256;
				y_coord -= 256;
			}

			u32 mos_h_size = 1;
			u32 mos_v_size = 1;

			if (mosaic) {
				mos_h_size = mos_h;
				mos_v_size = mos_v;
			}

			//The screen is divided in blocks of size NxM
			u32 transformed_y = lcd_y - (lcd_y % mos_v_size);

			if (!rot_scaling) {
				//Y coordinate in 'texture' (in the object)
				u32 tex_y = transformed_y - y_coord;

				//This should not be possible
				//but we still consider it
				if (tex_y > y_size)
					tex_y = 0;

				bool h_flip = CHECK_BIT(curr_obj.rotation, 3);
				bool v_flip = CHECK_BIT(curr_obj.rotation, 4);

				if (v_flip)
					tex_y = end_y - transformed_y - 1;

				//Tile Y position, inside
				//the tiles of the object
				u32 tile_y   = tex_y / TILE_Y_SIZE_PIXELS;
				//Offset Y in tile
				u32 y_offset = tex_y & (TILE_Y_SIZE_PIXELS - 1);

				//Last X coordinate
				u32 end = x_start + x_size;

				u32 vram_tile_y_id = 0;

				if (addressing_mode) {
					//1D addressing mode, tiles
					//for the same object are
					//one line after the other
					vram_tile_y_id = tile_y * total_x_tiles;
				}
				else {
					//2D addressing mode, the entire
					//VRAM tile set is a 2-dimension
					//matrix. Lines of the same
					//object are not one after the other
					vram_tile_y_id = tile_y * PAL_STRIDE;
				}

				//Start Y offset in VRAM
				u32 vram_y_offset = (vram_tile_y_id * tile_size) +
					(y_offset * line_size);

				if ((int)end >= 0) {
					for (u32 x = x_start < 0 ? 0 : x_start; x < end &&
						x < 240; x++) {
						//X coordinate aggiusted for mosaic
						u32 transformed_x = x - (x % mos_h_size);
						//X coordinate inside object
						u32 tex_x = transformed_x - x_start;

						if (tex_x > x_size)
							tex_x = 0;

						if (h_flip)
							tex_x = end - transformed_x - 1;

						//Tile X position inside object's
						//tiles
						u32 tile_x   = tex_x / TILE_X_SIZE_PIXELS;
						//Offset X inside tile
						u32 x_offset = tex_x & (TILE_X_SIZE_PIXELS - 1);

						//If palette is 16/16 (16 palettes, 16 colors)
						//each byte contains the color information
						//for two pixels
						if (pal_mode == OAMEntryPalette::PAL_016)
							x_offset >>= 1;

						//Final offset inside the VRAM (relative
						//to the tiles dedicated to objects)
						u32 vram_offset = start_offset + vram_y_offset
							+ (tile_x * tile_size)
							+ x_offset;

						vram_offset &= 0x7FFF;
						u16 color_id = m_vram[OBJ_VRAM_BASE
							+ vram_offset];

						//Final packed RGB color
						u16 color = 0;

						if (pal_mode == OAMEntryPalette::PAL_256) {
							//Direct color
							color = READ_16(m_palette_ram,
								(u32(color_id) << 1) + OBJ_PALETTE_START);
						}
						else {
							u16 pixel_id = 0;

							//Each nibble (4 bits) gives the
							//color id for each pixel. The color
							//itself is taken from the palette

							if (tex_x & 1) {
								pixel_id = (color_id >> 4) & 0xF;
							}
							else {
								pixel_id = color_id & 0xF;
							}

							//Save for later, to keep
							//track of the palette id
							color_id = pixel_id;

							//Each palette is 16 colors, each color
							//is 16 bits

							pixel_id <<= 1;
							pixel_id += u16(pal_number) << 5;

							color = READ_16(m_palette_ram,
								pixel_id + OBJ_PALETTE_START);
						}

						if (color_id) {
							if (obj_window) {
								//Object is not rendered, the pixel is marked
								//as part of the object window
								m_obj_window_pixels[x] = true;
							}
							else if (!m_line_data[4][x].is_present || m_line_data[4][x].priority >= prio_to_bg) {
								m_line_data[4][x].is_present = true;
								m_line_data[4][x].priority = prio_to_bg;
								m_line_data[4][x].palette_id = color_id;
								m_line_data[4][x].color = color;
								m_line_data[4][x].is_bld_enabled = mode == OAMEntryType::TRANSP;
							}
						}
					}
				}
			}
			else {
				//Is object double size?
				auto double_size = curr_obj.double_sz;

				//Which affine parameters to use
				//inside OAM
				u32 parameter_sel = curr_obj.rotation;
				u32 group_offset  = parameter_sel << 5;

				//Delta X inside texture for each X step on the screen
				i16 dx  = READ_16(m_oam, group_offset + 0x6);
				//Delta X inside texture for each Y step on the screen
				i16 dmx = READ_16(m_oam, group_offset + 0xE);
				i16 dy  = READ_16(m_oam, group_offset + 0x16);
				i16 dmy = READ_16(m_oam, group_offset + 0x1E);

				auto orig_x_size = x_size;
				auto orig_y_size = y_size;

				if (double_size) {
					y_size <<= 1;
					x_size <<= 1;
				}

				//Center is in the middle of the object

				auto y_center = i32(y_size >> 1);
				auto x_center = i32(x_size >> 1);

				//Compute initial coordinates inside texture

				auto local_x = i32(x_start < 0 ? -x_center + -x_start : -x_center);
				auto local_y = i32(transformed_y - (y_coord + y_center));


				auto tex_y_base = i32(local_y * dmy);
				auto tex_x_base = i32(local_y * dmx);

				u32 end_x = x_start + x_size;

				if ((int)end_x >= 0) {
					for (u32 x = x_start < 0 ? 0 : x_start; x < end_x && x < 240; x++) {
						auto curr_x = i32(tex_x_base + dx * local_x + (x_size << 7));
						auto curr_y = i32(tex_y_base + dy * local_x + (y_size << 7));

						local_x++;

						auto tex_x = i32(curr_x >> 8);
						auto tex_y = i32(curr_y >> 8);

						if (double_size) {
							tex_x -= orig_x_size >> 1;
							tex_y -= orig_y_size >> 1;
						}

						if (tex_x < 0 || tex_y < 0) {
							continue;
						}

						if (tex_x >= i32(orig_x_size) || tex_y >= i32(orig_y_size)) {
							continue;
						}

						u32 tile_y   = tex_y >> 3;
						u32 y_offset = tex_y & (TILE_Y_SIZE_PIXELS - 1);

						u32 vram_tile_y_id = 0;

						if (addressing_mode)
							vram_tile_y_id = tile_y * total_x_tiles;
						else
							vram_tile_y_id = tile_y * PAL_STRIDE;

						u32 vram_y_offset = (vram_tile_y_id * tile_size) +
							(y_offset * line_size);

						u32 tile_x   = tex_x >> 3;
						u32 x_offset = tex_x & (TILE_X_SIZE_PIXELS - 1);

						if (pal_mode == OAMEntryPalette::PAL_016)
							x_offset >>= 1;

						u32 vram_offset = start_offset + vram_y_offset
							+ (tile_x * tile_size)
							+ x_offset;

						vram_offset &= 0x7FFF;
						u16 color_id = m_vram[OBJ_VRAM_BASE
							+ vram_offset];

						u16 color = 0;

						if (pal_mode == OAMEntryPalette::PAL_256) {
							color = READ_16(m_palette_ram,
								color_id * 2 + OBJ_PALETTE_START);
						}
						else {
							u16 pixel_id = 0;

							if (tex_x & 1)
								pixel_id = (color_id >> 4) & 0xF;
							else
								pixel_id = color_id & 0xF;

							color_id = pixel_id;

							pixel_id <<= 1;
							pixel_id += u16(pal_number) << 5;

							color = READ_16(m_palette_ram,
								pixel_id + OBJ_PALETTE_START);
						}

						if (color_id) {
							if (obj_window) {
								m_obj_window_pixels[x] = true;
							}
							else if (!m_line_data[4][x].is_present || m_line_data[4][x].priority >= prio_to_bg) {
								m_line_data[4][x].is_present = true;
								m_line_data[4][x].priority = prio_to_bg;
								m_line_data[4][x].palette_id = color_id;
								m_line_data[4][x].color = color;
								m_line_data[4][x].is_bld_enabled = mode == OAMEntryType::TRANSP;
							}
						}
					}
				}
			}
		}
	}
}