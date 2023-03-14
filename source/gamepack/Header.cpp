#include "../../gamepack/Header.hpp"

namespace GBA::gamepack {
	bool VerifyHeader(GamePackHeader const* header) {
		if (!VerifyChecksum(header))
			return false;

		if (header->fixed != 0x96)
			return false;

		if (header->unitCode != 0x00)
			return false;

		return true;
	}

	bool VerifyChecksum(GamePackHeader const* header) {
		u8 const* ptr = reinterpret_cast<u8 const*>(header);

		u16 chk = 0;

		for (int i = 0x0A0; i < 0x0BC; i++) {
			chk -= ptr[i];
		}

		chk -= 0x19;
		chk &= 0xFF;

		return chk == header->headerChecksum;
	}
}