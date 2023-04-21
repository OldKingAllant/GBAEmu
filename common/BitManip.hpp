#pragma once

#define CHECK_BIT(value, n) ((value >> n) & 1)
#define CLEAR_BIT(value, n) (value & ~(1 << n))
#define SET_BIT(value, n) (value | (1 << n))
#define INVERT_BIT(value, n) (value ^ (1 << n))
#define SET_BIT_TO(value, n, bit) ((value & ~(1 << n)) | (bit << n))