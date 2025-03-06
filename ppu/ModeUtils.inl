#pragma once

void ProcessNormalBackground(int bg_id, int lcd_y);
void ProcessAffineBackground(int bg_id, int lcd_y);

std::array<Pixel, 240> MergeBackrounds(int lcd_y);

std::array<Pixel, 240> MergeBitmap(
	int lcd_y,
	std::array<Pixel, 240> const& bg2,
	std::array<Pixel, 240> const& sprites
);