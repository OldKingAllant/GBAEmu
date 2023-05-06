#pragma once

#include "../common/Logger.hpp"
#include "../common/BitManip.hpp"
#include "../common/Defs.hpp"

#include <functional>
#include <array>

namespace GBA::memory {
	using IOReadFunction8 = std::function<common::u8()>;
	using IOReadFunction16 = std::function<common::u16()>;
	using IOReadFunction32 = std::function<common::u32()>;

	using IOWriteFunction8 = std::function<void(common::u8)>;
	using IOWriteFunction16 = std::function<void(common::u16)>;
	using IOWriteFunction32 = std::function<void(common::u32)>;

	static constexpr common::u32 IO_SIZE = 0x3FF;

	static constexpr std::array<bool, 0x3FF> UNUSED_REGISTERS_MAP = []() {
		std::array<bool, 0x3FF> unused_map{};

		unused_map[0x4E] = true;
		unused_map[0x4F] = true;
		unused_map[0x56] = true;
		unused_map[0x57] = true;

		unused_map[0x66] = true;
		unused_map[0x67] = true;
		unused_map[0x6A] = true;
		unused_map[0x6B] = true;
		unused_map[0x6E] = true;
		unused_map[0x6F] = true;
		unused_map[0x76] = true;
		unused_map[0x77] = true;
		unused_map[0x7E] = true;
		unused_map[0x7F] = true;
		unused_map[0x86] = true;
		unused_map[0x87] = true;
		unused_map[0x8A] = true;
		unused_map[0x8B] = true;
		unused_map[0xA8] = true;
		unused_map[0xA9] = true;

		unused_map[0xE0] = true;
		unused_map[0xE1] = true;

		unused_map[0x110] = true;
		unused_map[0x111] = true;

		unused_map[0x12C] = true;
		unused_map[0x12D] = true;

		unused_map[0x136] = true;
		unused_map[0x137] = true;
		unused_map[0x138] = true;
		unused_map[0x139] = true;
		unused_map[0x142] = true;
		unused_map[0x143] = true;
		unused_map[0x15A] = true;
		unused_map[0x15B] = true;

		unused_map[0x206] = true;
		unused_map[0x207] = true;
		unused_map[0x20A] = true;
		unused_map[0x20B] = true;

		for (unsigned i = 0x58; i < 0x60; i++)
			unused_map[i] = true;

		for (unsigned i = 0xAA; i < 0xB0; i++)
			unused_map[i] = true;

		for (unsigned i = 0xE2; i < 0x100; i++)
			unused_map[i] = true;

		for (unsigned i = 0x112; i < 0x120; i++)
			unused_map[i] = true;

		unused_map[0x12E] = true;
		unused_map[0x12F] = true;

		for (unsigned i = 0x142; i < 0x150; i++)
			unused_map[i] = true;

		for (unsigned i = 0x15A; i < 0x200; i++)
			unused_map[i] = true;

		for (unsigned i = 0x20A; i < 0x300; i++)
			unused_map[i] = true;

		return unused_map;
	} ();

	class MMIO {
	public :
		MMIO();

		template <typename Type, typename Handler>
		void RegisterRead(Handler&& handler, common::u16 offset) {
			if (offset > 0x3FE)
				return;

			if constexpr (sizeof(Type) == 1)
				m_read8[offset] = IOReadFunction8(std::forward<Handler>(handler));
			else if constexpr (sizeof(Type) == 2) {
				offset /= 2;
				m_read16[offset] = IOReadFunction16(std::forward<Handler>(handler));
			}
			else {
				offset /= 4;
				m_read32[offset] = IOReadFunction32(std::forward<Handler>(handler));
			}
		}

		template <typename Type, typename Handler>
		void RegisterWrite(Handler&& handler, common::u16 offset) {
			if (offset > 0x3FE)
				return;

			if constexpr (sizeof(Type) == 1)
				m_write8[offset] = IOWriteFunction8(std::forward<Handler>(handler));
			else if constexpr (sizeof(Type) == 2) {
				offset /= 2;
				m_write16[offset] = IOWriteFunction16(std::forward<Handler>(handler));
			}
			else {
				offset /= 4;
				m_write32[offset] = IOWriteFunction32(std::forward<Handler>(handler));
			}
		}

		template <typename Type>
		Type Read(common::u16 offset) {
			if constexpr (sizeof(Type) == 1)
				return (Type)m_read8[offset]();
			else if constexpr (sizeof(Type) == 2) {
				offset /= 2;
				return (Type)m_read16[offset]();
			}
			else {
				offset /= 4;
				return (Type)m_read32[offset]();
			}
		}

		template <typename Type>
		void Write(common::u16 offset, Type value) {
			if constexpr (sizeof(Type) == 1)
				m_write8[offset](value);
			else if constexpr (sizeof(Type) == 2) {
				offset /= 2;
				offset &= ~1;
				m_write16[offset](value);
			}
			else {
				offset /= 4;
				offset &= ~3;
				m_write32[offset](value);
			}
		}

	private:
		IOReadFunction8 m_read8[IO_SIZE];
		IOReadFunction16 m_read16[IO_SIZE / 2];
		IOReadFunction32 m_read32[IO_SIZE / 4];

		IOWriteFunction8 m_write8[IO_SIZE];
		IOWriteFunction16 m_write16[IO_SIZE / 2];
		IOWriteFunction32 m_write32[IO_SIZE / 4];
	};
}