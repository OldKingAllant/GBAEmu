#include "../../memory/MMIO.hpp"

namespace GBA::memory {
	using namespace common;

	MMIO::MMIO() :
		m_read8{}, m_read16{}, m_read32{},
		m_write8{}, m_write16{}, m_write32{}
	{
		auto read_dummy8 = []() -> u8 {
			return 0xFF;
		};

		auto read_dummy16 = []() -> u16 {
			return (u16)~0;
		};

		auto read_dummy32 = []() -> u32 {
			return (u32)~0;
		};

		auto write_dummy8 = [](u8 dummy) {
			(void)dummy;
		};

		auto write_dummy16 = [](u16 dummy) {
			(void)dummy;
		};

		auto write_dummy32 = [](u32 dummy) {
			(void)dummy;
		};

		for (unsigned i = 0; i < IO_SIZE; i++) {
			m_read8[i] = read_dummy8;
			m_write8[i] = write_dummy8;
		}

		for (unsigned i = 0; i < IO_SIZE / 2; i++) {
			m_read16[i] = read_dummy16;
			m_write16[i] = write_dummy16;
		}
		
		for (unsigned i = 0; i < IO_SIZE / 4; i++) {
			m_read32[i] = read_dummy32;
			m_write32[i] = write_dummy32;
		}
	}
}