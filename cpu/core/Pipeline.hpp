#pragma once

#include "../../common/Defs.hpp"
#include "./Register.hpp"
#include "../../memory/Bus.hpp"

namespace GBA::cpu {
	using namespace common;

	class Pipeline {
	public :
		Pipeline(memory::Bus* bus);

		u32 GetFetchPC() const;

		template <InstructionMode InstrSet>
		void Bubble(u32 address) {
			m_fetch_pc = address;

			Fetch<InstrSet>();
			Fetch<InstrSet>();
		}

		template <InstructionMode InstrSet>
		void Fetch() {
			m_decoded = m_fetched;

			if constexpr (InstrSet == InstructionMode::ARM) {
				m_fetched = m_bus->Read<u32>(m_fetch_pc);
				m_fetch_pc += 4;
			}
			else {
				m_fetched = m_bus->Read<u16>(m_fetch_pc);
				m_fetch_pc += 2;
			}
		}

		template <InstructionMode InstrSet>
		auto Pop() {
			if constexpr (InstrSet == InstructionMode::ARM) {
				return static_cast<u32>(m_decoded);
			}
			else {
				return static_cast<u16>(m_decoded & 0xFFFF);
			}
		}

	private :
		u32 m_fetch_pc;
		u32 m_fetched;
		u32 m_decoded;

		memory::Bus* m_bus;
	};
}