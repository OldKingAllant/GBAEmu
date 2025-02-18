#pragma once

#include "../../common/Defs.hpp"
#include "./Register.hpp"
#include "../../memory/Bus.hpp"


namespace GBA::cpu {
	using namespace common;
	using memory::Access;

	class Pipeline {
	public :
		Pipeline();

		void AttachBus(memory::Bus* bus) {
			m_bus = bus;
		}

		u32 GetFetchPC() const;
		u32 GetFetched() const;
		u32 GetDecoded() const;

		template <InstructionMode InstrSet>
		void Bubble(u32 address) {
			m_fetch_pc = address;

			m_bus->m_time.access = Access::NonSeq;

			Fetch<InstrSet>();

			m_bus->m_time.access = Access::Seq;

			Fetch<InstrSet>();
		}

		template <InstructionMode InstrSet>
		void Fetch() {
			m_decoded = m_fetched;

			if constexpr (InstrSet == InstructionMode::ARM) {
				m_fetched = m_bus->Read<u32>(m_fetch_pc, true);
				m_fetch_pc += 4;
			}
			else {
				m_fetched = m_bus->Read<u16>(m_fetch_pc, true);
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

		template <typename Ar>
		void save(Ar& ar) const {
			ar(m_fetch_pc);
			ar(m_fetched);
			ar(m_decoded);
		}

		template <typename Ar>
		void load(Ar& ar) {
			ar(m_fetch_pc);
			ar(m_fetched);
			ar(m_decoded);
		}

	private :
		u32 m_fetch_pc;
		u32 m_fetched;
		u32 m_decoded;

		memory::Bus* m_bus;
	};
}