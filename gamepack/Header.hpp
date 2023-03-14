#pragma once

#include "../common/Defs.hpp"

/*
* 000h    4     ROM Entry Point  (32bit ARM branch opcode, eg. "B rom_start")
  004h    156   Nintendo Logo    (compressed bitmap, required!)
  0A0h    12    Game Title       (uppercase ascii, max 12 characters)
  0ACh    4     Game Code        (uppercase ascii, 4 characters)
  0B0h    2     Maker Code       (uppercase ascii, 2 characters)
  0B2h    1     Fixed value      (must be 96h, required!)
  0B3h    1     Main unit code   (00h for current GBA models)
  0B4h    1     Device type      (usually 00h) (bit7=DACS/debug related)
  0B5h    7     Reserved Area    (should be zero filled)
  0BCh    1     Software version (usually 00h)
  0BDh    1     Complement check (header checksum, required!)
  0BEh    2     Reserved Area    (should be zero filled)
*/

namespace GBA::gamepack {
	using namespace common;

	struct GamePackHeader {
		u32 entry;
		u8 logo[156];
		u8 title[12];
		char8_t gameMaker[4];
		char8_t makerCode[2];
		u8 fixed;
		u8 unitCode;
		u8 devType;
		u8 reserved[0x7];
		u8 version;
		u8 headerChecksum;
		u16 reserved2;
	};

	bool VerifyHeader(GamePackHeader const* header);

	bool VerifyChecksum(GamePackHeader const* header);
}