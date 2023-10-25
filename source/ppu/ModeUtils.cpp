#include "../../ppu/PPU.hpp"

#include "../../common/Error.hpp"

#include <algorithm>

namespace GBA::ppu {
	using namespace common;

	namespace detail {
		constexpr u16 bg_control_reg[] = {
			0x8, 0xA, 0xC, 0xE
		};

		constexpr u16 bg_rot_scale_reg[][2] = {
			{ }, 
			{ }, 
			{ 0x28, 0x2C },
			{ 0x38, 0x3C }
		};

		constexpr u16 bg_aff_param_reg[][4] = {
			{ },
			{ },
			{ 0x20, 0x22, 0x24, 0x26 },
			{ 0x30, 0x32, 0x34, 0x36 }
		};

		constexpr u16 bg_scroll_reg[][2] = {
			{ 0x10, 0x12 },
			{ 0x14, 0x16 },
			{ 0x18, 0x1A },
			{ 0x1C, 0x1E }
		};

		constexpr u16 bg_tile_size[] = {
			0x20, 0x40
		};

		constexpr u16 bg_size[][2] = {
			{ 256, 256 },
			{ 512, 256 },
			{ 256, 512 },
			{ 512, 512 }
		};

		constexpr u16 bg_aff_size[][2] = {
			{ 128, 128 }, 
			{ 256, 256 },
			{ 512, 512 },
			{ 1024, 1024 }
		};

		constexpr u16 bg_tilemap_size[][2] = {
			{ 32, 32 },
			{ 64, 32 },
			{ 32, 64 },
			{ 64, 64 }
		};

		constexpr u16 bg_aff_tilemap_size[][2] = {
			{ 16, 16 },
			{ 32, 32 },
			{ 64, 64 },
			{ 128, 128 }
		};

		constexpr u16 TILE_REGION_X_SIZE = 32;
		constexpr u16 TILE_REGION_Y_SIZE = 32;

		constexpr u16 TILE_X_SIZE = 8;
		constexpr u16 TILE_Y_SIZE = 8;
	}

	void PPU::CalculateMosaicBG(i32& x, i32& y) {
		u32 mosaic_reg = ReadRegister32(0x4C / 4);

		u8 h_size = (mosaic_reg & 0xF) + 1;
		u8 v_size = ((mosaic_reg >> 4) & 0xF) + 1;

		if (!h_size || !v_size)
			return;

		x -= (x % h_size);
		y -= (y % v_size);
	}

	std::array<Pixel, 240> PPU::ProcessNormalBackground(int bg_id, int lcd_y) {
		std::array<Pixel, 240> backround_data{};

		u16 bg_control = ReadRegister16(detail::bg_control_reg[bg_id] / 2);

		u32 char_base_block = (bg_control >> 2) & 0x3;
		char_base_block *= 16 * 1024;

		bool mosaic = (bg_control >> 6) & 1;
		bool pal_mode = (bg_control >> 7) & 1;

		u32 mos_h_size = 1;
		u32 mos_v_size = 1;

		if (mosaic) {
			u32 mos_cnt = ReadRegister32(0x4C / 4);

			mos_h_size = (mos_cnt & 0xF) + 1;
			mos_v_size = ((mos_cnt >> 4) & 0xF) + 1;
		}

		u32 map_base_block = (bg_control >> 8) & 0x1F;
		map_base_block *= 2 * 1024;

		u8 screen_sz_type = (bg_control >> 14) & 0x3;

		u32 bg_size_x = detail::bg_size[screen_sz_type][0];
		u32 bg_size_y = detail::bg_size[screen_sz_type][1];

		u32 scroll_x = ReadRegister16(detail::bg_scroll_reg[bg_id][0] / 2) & 511;
		u32 scroll_y = ReadRegister16(detail::bg_scroll_reg[bg_id][1] / 2) & 511;

		u32 bg_x = 0;
		u32 bg_y = scroll_y + (u32)lcd_y;
		bg_y %= bg_size_y;
		bg_y -= (bg_y % mos_v_size);

		if ((i32)bg_y < 0)
			bg_y = 0;

		u32 y_offset_inside_tile = bg_y % detail::TILE_Y_SIZE;

		u32 tile_y = bg_y / detail::TILE_Y_SIZE;
		u32 x_offset_inside_tile = 0;
		u32 tile_x = 0;
		u32 tile_region_y = tile_y / detail::TILE_REGION_Y_SIZE;
		u32 tile_region_x = 0;

		u32 total_tile_regions_x = detail::bg_tilemap_size[screen_sz_type][0] / 32;

		u32 tile_row_sz = 4;
		u32 tile_data_size = 0x20;

		if (pal_mode) {
			tile_row_sz = 8;
			tile_data_size = 0x40;
		}
			

		for (int x = 0; x < 240; x++) {
			/*Find tilemap entry*/
			bg_x = scroll_x + x;
			bg_x %= bg_size_x;
			bg_x -= (bg_x % mos_h_size);

			if ((i32)bg_x < 0)
				bg_x = 0;

			x_offset_inside_tile = bg_x % detail::TILE_X_SIZE;
			tile_x = bg_x / detail::TILE_X_SIZE;

			tile_region_x = tile_x / detail::TILE_REGION_X_SIZE;

			u32 tile_region_id = total_tile_regions_x * tile_region_y
				+ tile_region_x;

			tile_x %= detail::TILE_REGION_X_SIZE;
			tile_y %= detail::TILE_REGION_Y_SIZE;

			u32 vram_offset = map_base_block + (tile_region_id * 2048) + 
				(tile_y * 64) + tile_x * 2;

			u16 tile_entry = *reinterpret_cast<u16*>(m_vram + vram_offset);

			u32 tile_id = tile_entry & 1023;
			bool hflip = (tile_entry >> 10) & 1;
			bool vflip = (tile_entry >> 11) & 1;
			u32 pal_id = (tile_entry >> 12) & 0xF;

			u32 real_y_offset_in_tile = y_offset_inside_tile;

			if (hflip)
				x_offset_inside_tile = 7 - x_offset_inside_tile;
			
			if (vflip)
				real_y_offset_in_tile = 7 - y_offset_inside_tile;

			vram_offset = char_base_block + tile_id * tile_data_size
				+ (tile_row_sz * real_y_offset_in_tile);

			if (pal_mode)
				vram_offset += x_offset_inside_tile;
			else
				vram_offset += x_offset_inside_tile / 2;

			u16 color_id = m_vram[vram_offset];

			u16 color = 0;

			if (pal_mode) {
				color = *reinterpret_cast<u16*>(m_palette_ram + color_id * 2);
				backround_data[x].color = color;
				backround_data[x].palette_id = color_id;
			}
			else {
				pal_id *= 32;

				u16 color = 0;

				if(bg_x % 2)
					color = ((color_id >> 4) & 0xF) * 2;
				else 
					color = (color_id & 0xF) * 2;

				backround_data[x].color = *reinterpret_cast<u16*>(m_palette_ram + pal_id + color);
				backround_data[x].palette_id = color;
			}
		}

		return backround_data;
	}

	std::array<Pixel, 240> PPU::ProcessAffineBackground(int bg_id, int lcd_y) {
		std::array<Pixel, 240> backround_data{};

		u16 bg_control = ReadRegister16(detail::bg_control_reg[bg_id] / 2);

		u32 ch_base_block = (bg_control >> 2) & 3;
		ch_base_block *= 16 * 1024;

		bool mosaic = (bg_control >> 6) & 1;
		bool pal_mode = (bg_control >> 7) & 1;

		u32 map_base_block = (bg_control >> 8) & 0x1F;
		map_base_block *= 2 * 1024;

		u8 screen_sz_type = (bg_control >> 14) & 0x3;

		u32 map_x_size = detail::bg_aff_size[screen_sz_type][0];
		u32 map_y_size = detail::bg_aff_size[screen_sz_type][1];

		bool area_overflow = (bg_control >> 13) & 1;

		i32 ref_x = (i32)m_internal_reference_x[bg_id - 2];
		i32 ref_y = (i32)m_internal_reference_y[bg_id - 2];

		ref_x <<= 4;
		ref_x >>= 4;

		ref_y <<= 4;
		ref_y >>= 4;

		i16 dx = (i16)ReadRegister16(detail::bg_aff_param_reg[bg_id][0] / 2);
		i16 dmx = (i16)ReadRegister16(detail::bg_aff_param_reg[bg_id][1] / 2);
		i16 dy = (i16)ReadRegister16(detail::bg_aff_param_reg[bg_id][2] / 2);
		i16 dmy = (i16)ReadRegister16(detail::bg_aff_param_reg[bg_id][3] / 2);


		//Y position changes with x incresing,
		//we cannot precalculate tile y index

		i32 curr_y = ref_y;
		i32 curr_x = ref_x;

		i32 tex_x = 0;
		i32 tex_y = 0;

		u32 tiles_per_line = detail::bg_aff_tilemap_size[screen_sz_type][0];

		u32 tile_row_sz = 8;
		u32 tile_data_size = 0x40;

		for (int x = 0; x < 240; x++) {
			tex_x = curr_x >> 8;
			tex_y = curr_y >> 8;

			if (tex_x < 0 || tex_y < 0) {
				if (area_overflow) {
					tex_x = map_x_size + tex_x;
					tex_y = map_y_size + tex_y;
				}
				else {
					curr_x += dx;
					curr_y += dy;
					continue;
				}
			}
				

			if (((u32)tex_x > map_x_size || (u32)tex_y > map_y_size)
				&& !area_overflow) {
				curr_x += dx;
				curr_y += dy;
				continue;
			}

			tex_x %= map_x_size;
			tex_y %= map_y_size;

			u32 y_offset_inside_tile = tex_y % detail::TILE_Y_SIZE;

			u32 tile_y = tex_y / detail::TILE_Y_SIZE;
			u32 x_offset_inside_tile = 0;
			u32 tile_x = 0;

			x_offset_inside_tile = tex_x % detail::TILE_X_SIZE;
			tile_x = tex_x / detail::TILE_X_SIZE;

			u32 vram_offset = map_base_block +
				(tile_y * tiles_per_line) + tile_x;

			u8 tile_number = m_vram[vram_offset];

			vram_offset = ch_base_block + tile_number * tile_data_size
				+ (tile_row_sz * y_offset_inside_tile);

			vram_offset += x_offset_inside_tile;

			//No palette mode can be selected
			//only 256/1 is allowed

			u8 color_id = m_vram[vram_offset];

			u16 color = *reinterpret_cast<u16*>(m_palette_ram + color_id * 2);

			backround_data[x].color = color;
			backround_data[x].palette_id = color_id;

			curr_x += dx;
			curr_y += dy;
		}
		
		m_internal_reference_x[bg_id - 2] = (i32)m_internal_reference_x[bg_id - 2] + dmx;
		m_internal_reference_y[bg_id - 2] = (i32)m_internal_reference_y[bg_id - 2] + dmy;

		return backround_data;
	}

	std::array<std::array<Pixel, 240>, 5> backgrounds{};

	std::array<Pixel, 240> PPU::MergeBackrounds(
		std::array<Pixel, 240> const& bg1,
		std::array<Pixel, 240> const& bg2,
		std::array<Pixel, 240> const& bg3,
		std::array<Pixel, 240> const& bg4,
		std::array<Pixel, 240>& sprites
	) {
		std::array<Pixel, 240> merged{};

		backgrounds = {
			bg1, bg2, bg3, bg4
		};

		u16 bg1_cnt = ReadRegister16(0x8 / 2);
		u16 bg2_cnt = ReadRegister16(0xA / 2);
		u16 bg3_cnt = ReadRegister16(0xC / 2);
		u16 bg4_cnt = ReadRegister16(0xE / 2);

		u8 mode = m_ctx.m_control & 0x7;

		struct Priority {
			u8 value;
			u8 bg_id;
		};

		Priority priorities[] = {
			{ (u8)(bg1_cnt & 0x3), (u8)0}, { (u8)(bg2_cnt & 0x3), (u8)1},
			{ (u8)(bg3_cnt & 0x3), (u8)2}, { (u8)(bg4_cnt & 0x3), (u8)3 }
		};

		//Priority goes from 0 to 3 with 0 highest
		//Between same priority, lower bg id wins

		//Extract bg with highest priority

		std::sort(std::begin(priorities), std::end(priorities),
			[](Priority p1, Priority p2) {
				return p1.value > p2.value;
		});

		//////////////////

		bool bg0_en = ((m_ctx.m_control >> 8) & 1)
			&& mode < 2;
		bool bg1_en = ((m_ctx.m_control >> 9) & 1)
			&& mode < 2;
		bool bg2_en = (m_ctx.m_control >> 10) & 1;
		bool bg3_en = ((m_ctx.m_control >> 11) & 1)
			&& (mode == 0 || mode == 2);
		bool obj_en = (m_ctx.m_control >> 12) & 1;

		bool win0_en = (m_ctx.m_control >> 13) & 1;
		bool win1_en = (m_ctx.m_control >> 14) & 1;

		bool win_enabled = win0_en || win1_en;

		u16 winin_cnt = ReadRegister16(0x48 / 2);
		u16 winout_cnt = ReadRegister16(0x4A / 2);

		int win0_bg_enable[] = {
			winin_cnt & 1, (winin_cnt >> 1) & 1,
			(winin_cnt >> 2) & 1, (winin_cnt >> 3) & 1
		};

		int win1_bg_enable[] = {
			(winin_cnt >> 8) & 1, (winin_cnt >> 9) & 1,
			(winin_cnt >> 10) & 1, (winin_cnt >> 11) & 1
		};

		int winout_bg_enable[] = {
			winout_cnt & 1, (winout_cnt >> 1) & 1,
			(winout_cnt >> 2) & 1, (winout_cnt >> 3) & 1
		};

		bool win0_obj_enable = (winin_cnt >> 4) & 1;
		bool win1_obj_enable = (winin_cnt >> 12) & 1;
		bool winout_obj_enable = (winout_cnt >> 4) & 1;

		u16 win0_h = ReadRegister16(0x40 / 2);
		u16 win1_h = ReadRegister16(0x42 / 2);

		u16 win0_v = ReadRegister16(0x44 / 2);
		u16 win1_v = ReadRegister16(0x46 / 2);

		u16 win0_left = (win0_h >> 8) & 0xFF;
		u16 win0_right = (win0_h & 0xFF) - 1;

		u16 win0_top = (win0_v >> 8) & 0xFF;
		u16 win0_bottom = (win0_v & 0xFF) - 1;

		u16 win1_left = (win1_h >> 8) & 0xFF;
		u16 win1_right = (win1_h & 0xFF) - 1;

		u16 win1_top = (win1_v >> 8) & 0xFF;
		u16 win1_bottom = (win1_v & 0xFF) - 1;

		////////////////

		u8 main_bg_id = priorities[0].bg_id;

		auto is_gb_completely_disabled = [&](u8 bg_id) {
			if (win0_en || win1_en) {
				if ((win0_en && !win0_bg_enable[bg_id]) &&
					(win1_en && !win1_bg_enable[bg_id]) &&
					!winout_bg_enable[bg_id]) {
					//Disabled both in winin and winout
					return true;
				}
			}

			return false;
		};

		u16 curr_line = m_ctx.m_vcount;

		auto curr_window_id = [&](u16 x) {
			if (win0_en &&
				(curr_line >= win0_top && curr_line <= win0_bottom) &&
				(x >= win0_left && x <= win0_right)) {
				return 0;
			}

			if (win1_en &&
				(curr_line >= win1_top && curr_line <= win1_bottom) &&
				(x >= win1_left && x <= win1_right)) {
				return 1;
			}

			return 2;
		};

		auto is_gb_window_disabled = [&](u8 bg_id, u8 win_id) {
			//Window 0 has priority

			if (win0_en && win_id == 0)
				return !win0_bg_enable[bg_id];

			if (win1_en && win_id == 1)
				return !win1_bg_enable[bg_id];

			return false;
		};

		auto is_obj_window_disabled = [&](u8 win_id) {
			if (win0_en && win_id == 0)
				return !win0_obj_enable;

			if (win1_en && win_id == 1)
				return !win1_obj_enable;

			return false;
		};

		bool gb_disabled[] = {
			is_gb_completely_disabled(0) || !bg0_en,
			is_gb_completely_disabled(1) || !bg1_en,
			is_gb_completely_disabled(2) || !bg2_en,
			is_gb_completely_disabled(3) || !bg3_en
		};

		//////////////////////
		//Apply color special effects
		u16 bldcnt = ReadRegister16(0x50 / 2);
		bool win0_bld_en = (winin_cnt >> 5) & 1;
		bool win1_bld_en = (winin_cnt >> 13) & 1;
		bool winout_bld_en = (winout_cnt >> 5) & 1;

		auto blend_enabled_window = [&](u8 win_id) {
			if (win0_en && win_id == 0)
				return win0_bld_en;

			if (win1_en && win_id == 1)
				return win1_bld_en;

			return false;
		};

		u8 effect = (bldcnt >> 6) & 3;

		u8 top_layers = bldcnt & 0x1F;

		switch (effect)
		{
		case 0x1: {
			u8 bottom_layers = (bldcnt >> 8) & 0x1F;

			bool top_layers_obj = (top_layers >> 4) & 1;

			u8 pos = 0;

			u16 bldalpha = ReadRegister16(0x52 / 2);

			u8 eva = bldalpha & 0x1F;
			u8 evb = (bldalpha >> 8) & 0x1F;

			//We cannot select top/bottom pairs at the
			//beginning, since transparent pixels
			//can cause a fallback to other layers

			for (u32 x = 0; x < 240; x++) {
				Pixel* top_pixel = nullptr;
				Pixel* bottom_pixel = nullptr;

				
			}
		}
		break;

		case 0x2: {
			u16 bldy = ReadRegister16(0x54 / 2);

			u8 evy = bldy & 0x1F;

			bool backdrop_changed = CHECK_BIT(bldcnt, 5);

			if (backdrop_changed) {
				u16 backdrop_color = *reinterpret_cast<u16*>(m_palette_ram);

				//Save the backdrop color somewhere 
			}

			u8 pos = 0;

			while (pos < backgrounds.size()) {
				if (!CHECK_BIT(top_layers, pos)) {
					pos++;
					continue;
				}

				for (int x = 0; x < 240; x++) {
					u8 window_id = curr_window_id(x);

					if (win_enabled) {
						if ((window_id == 2 && !winout_bld_en) ||
							(window_id != 2 && !blend_enabled_window(window_id)))
							continue;
						// We could
						//skip directly to the 
						//end of the window
					}

					u16 color = backgrounds[pos][x].color;

					if (!backgrounds[pos][x].palette_id)
						continue;

					u8 r = color & 0x1F;
					u8 g = (color >> 5) & 0x1F;
					u8 b = (color >> 10) & 0x1F;

					r = (r + (31 - r) * evy) & 0x1F;
					g = (g + (31 - g) * evy) & 0x1F;
					b = (b + (31 - b) * evy) & 0x1F;

					color = (b << 10) | (g << 5) | r;

					backgrounds[pos][x].color = color;
				}

				pos++;
			}
		}
		break;

		case 0x3: {
			u16 bldy = ReadRegister16(0x54 / 2);

			u8 evy = bldy & 0x1F;

			bool backdrop_changed = CHECK_BIT(bldcnt, 5);

			if (backdrop_changed) {
				u16 backdrop_color = *reinterpret_cast<u16*>(m_palette_ram);

				//Save the backdrop color somewhere 
			}

			u8 pos = 0;

			while (pos < backgrounds.size()) {
				if (!CHECK_BIT(top_layers, pos)) {
					pos++;
					continue;
				}

				for (int x = 0; x < 240; x++) {
					u8 window_id = curr_window_id(x);

					if (win_enabled) {
						if((window_id == 2 && !winout_bld_en) ||
							(window_id != 2 && !blend_enabled_window(window_id)))
							continue; 
						// We could
						//skip directly to the 
						//end of the window
					}

					u16 color = backgrounds[pos][x].color;
					
					if (!backgrounds[pos][x].palette_id)
						continue;

					u8 r = color & 0x1F;
					u8 g = (color >> 5) & 0x1F;
					u8 b = (color >> 10) & 0x1F;

					r = (r - r * evy) & 0x1F;
					g = (g - g * evy) & 0x1F;
					b = (b - b * evy) & 0x1F;

					color = (b << 10) | (g << 5) | r;

					backgrounds[pos][x].color = color;
				}

				pos++;
			}
		}
		break;

		default:
			break;
		}

		/////////////////////

		for (int x = 0; x < 240; x++) {
			Pixel result = backgrounds[main_bg_id][x];

			result.priority = priorities[main_bg_id].value;

			u8 window_id = curr_window_id(x);

			bool bg_disabled_window = gb_disabled[main_bg_id];

			if (win_enabled) {
				bg_disabled_window = bg_disabled_window ||
					(window_id != 2 && is_gb_window_disabled(main_bg_id, window_id))
					|| (window_id == 2 && !winout_bg_enable[main_bg_id]);
			}

			if (!result.palette_id || bg_disabled_window) {
				u8 curr_pos = 1;

				while ((!result.palette_id || bg_disabled_window) 
					&& curr_pos < 4) {
					u8 bg_id = priorities[curr_pos].bg_id;

					result = backgrounds[bg_id][x];

					bg_disabled_window = gb_disabled[bg_id];

					if (win_enabled) {
						bg_disabled_window = bg_disabled_window ||
							(window_id != 2 && is_gb_window_disabled(bg_id, window_id))
							|| (window_id == 2 && !winout_bg_enable[bg_id]);
					}

					curr_pos++;
				}

				if(curr_pos < 4)
					result.priority = priorities[curr_pos].value;

				if (curr_pos == 4 && bg_disabled_window) {
					//Also BG 3 is disabled, put transparent color at pixel X
					result = Pixel{};
					result.palette_id = 0;
					result.priority = 3;
				}
			}

			if (obj_en && sprites[x].is_obj && sprites[x].palette_id) {
				bool obj_allowed = true;

				if (win_enabled) {
					obj_allowed = (window_id != 2 && !is_obj_window_disabled(window_id))
						|| (window_id == 2 && winout_obj_enable);
				}

				if (obj_allowed) {
					if (sprites[x].priority <= result.priority
						|| !result.palette_id)
						result = sprites[x];
				}
			}

			merged[x] = result;
		}

		return merged;
	}
}