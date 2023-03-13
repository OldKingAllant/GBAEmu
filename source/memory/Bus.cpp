#include "../../memory/Bus.hpp"

namespace GBA::memory {
	Bus::Bus() {}

	/*
		template <typename Type>
		Type Read(u32 address) const;

		template <typename Type>
		void Write(u32 address, Type value);*/

	template <>
	u8 Bus::Read(u32 address) const {
		return static_cast<u8>(~0);
	}

	template <>
	u16 Bus::Read(u32 address) const {
		return static_cast<u16>(~0);
	}

	template <>
	u32 Bus::Read(u32 address) const {
		return static_cast<u32>(~0);
	}

	template <>
	void Bus::Write(u32 address, u8 value) {
		(void)address;
		(void)value;
	}

	template <>
	void Bus::Write(u32 address, u16 value) {
		(void)address;
		(void)value;
	}

	template <>
	void Bus::Write(u32 address, u32 value) {
		(void)address;
		(void)value;
	}
}