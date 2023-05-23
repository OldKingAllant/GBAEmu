#pragma once

struct Pixel {
	bool is_obj;
	GBA::common::i16 palette_id;
	GBA::common::u16 color;
	GBA::common::u8 priority;
};

void CalculateMosaicBG(GBA::common::i32& x, GBA::common::i32& y);

std::array<Pixel, 240> ProcessNormalBackground(int bg_id, int lcd_y);