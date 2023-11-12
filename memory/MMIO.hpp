#pragma once

#include "../common/Logger.hpp"
#include "../common/BitManip.hpp"
#include "../common/Defs.hpp"

#include <functional>
#include <array>
#include <optional>

namespace GBA::memory {
	/*
	TODO: Rewrite this mess

	Why?? Well... The runtime cost
	is not a problem. The problem
	is another one, and it is
	big

	As an interface it is very nice and
	easy to use. The implementation, 
	however, is the opposite. 
	Look at the PPU implementation
	for details

	You may ask: How can we improve this?
	Isn't this implementation
	needed for accuracy (e.g. writes
	to parts of a register should not have
	effect or similar things)?

	The answer is yes and no. 

	We could achieve a similar result
	by storing pointers to the
	registers themselves, and providing
	a callback only if needed

	For example:
	- Writes to the first 3 bits of the
	  dispstat register are ignored.
	  So, we register a write handler
	- The other registers (or most 
	  of them) can be accessed freely.
	  So we register a pointer 
	  to each register. 
	To verify if a callback is registered,
	just create an array of std::optional, 
	and if the optional contains a value,
	we use the callback. Otherwise,
	we go directly to the pointer

	In Extreme cases, even this can become
	messy. But it still should be better
	than the current implementation
	*/

	using IOWriteFunction = std::function<void(common::u8, common::u16)>;

	static constexpr common::u32 IO_SIZE = 0xFFF;

	static constexpr std::array<bool, 0xFFF> UNUSED_REGISTERS_MAP = []() {
		std::array<bool, 0xFFF> unused_map{};

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

		for (unsigned i = 0x20A; i <= 0x301; i++)
			unused_map[i] = true;

		//unused_map[0x800] = true;

		return unused_map;
	} ();

	struct IORegister {
		bool readable;
		bool writeable;
		common::u8* pointer;
		common::u8 mask;
		std::optional<IOWriteFunction> write_callback;
	};

	class MMIO {
	public :
		MMIO();

		template <typename Type>
		void AddRegister(common::u16 offset, bool readable, 
			bool writeable, common::u8* pointer, Type mask, 
			std::optional<IOWriteFunction> const& callback = std::nullopt) {
			for (uint8_t pos = 0; pos < sizeof(Type); pos++) {
				m_registers[offset + pos] = IORegister{
					readable, writeable, pointer + pos, (common::u8)mask, callback
				};

				mask >>= 8;
			}
		}

		template <typename Type>
		Type Read(common::u16 offset) {
			if constexpr (sizeof(Type) == 1) {
				auto& reg = m_registers[offset];

				if (!reg.pointer || !reg.readable)
					return (Type)~0;

				return *(reg.pointer);
			}
			else {
				Type value = 0;

				for (uint8_t pos = 0; pos < sizeof(Type); pos++) {
					auto& reg = m_registers[offset + pos];

					if (reg.pointer && reg.readable)
						value |= (*(reg.pointer) << (8 * pos));
				}

				return value;
			}
		}

		template <typename Type>
		void Write(common::u16 offset, Type value) {
			if constexpr (sizeof(Type) == 1) {
				auto& reg = m_registers[offset];

				if (!reg.pointer || !reg.writeable)
					return;

				if (reg.write_callback.has_value()) {
					reg.write_callback.value()(value, offset);
					return;
				}

				common::u8 mask = reg.mask;
				common::u8 curr_value = *(reg.pointer);
				
				value &= mask;
				value |= curr_value & ~mask;
				*(reg.pointer) = value;
			}
			else {
				for (uint8_t pos = 0; pos < sizeof(Type); pos++) {
					auto& reg = m_registers[offset + pos];

					if (!reg.pointer || !reg.writeable)
						continue;

					if (reg.write_callback.has_value()) {
						reg.write_callback.value()((common::u8)value, offset + pos);
						value >>= 8;
						continue;
					}

					common::u8 mask = reg.mask;
					common::u8 curr_value = *(reg.pointer);

					common::u8 set_val = (common::u8)value;

					set_val &= mask;
					set_val |= curr_value & ~mask;
					*(reg.pointer) = set_val;

					value >>= 8;
				}
			}
		}

		common::u8 DebuggerReadIO(common::u16 offset);

	private:
		IORegister m_registers[IO_SIZE];
	};
}