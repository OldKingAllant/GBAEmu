#pragma once

#include "../common/Defs.hpp"

namespace GBA::memory {
	using namespace common;

	class Bus {
	public :
		Bus();

		/*
		* Implementations are:
		* 1 byte
		* 1 halfword
		* 1 word
		*/
		template <typename Type>
		Type Read(u32 address) const;

		template <typename Type>
		void Write(u32 address, Type value);

	private :
		//
	};
}