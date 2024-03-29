#pragma once

#include "../../common/Defs.hpp"
#include "../ProcessorDefs.hpp"

#include <limits>

namespace GBA::cpu {
	using namespace common;

	//This could be a problem
	//But probably not,
	//everything is aligned
	//to a correct boundary

	//The CPSR register
#pragma pack(push, 1)
	struct CPSR {
		Mode mode : 5;
		InstructionMode instr_state : 1;
		bool fiq_disable : 1;
		bool irq_disable : 1;
		u8 : 8;
		u8 : 8;
		u8 : 3;
		bool sticky_ov : 1;
		bool overflow : 1;
		bool carry : 1;
		bool zero : 1;
		bool sign : 1;

		/*
		* Almost every ARM instruction
		* has a suffix representing
		* the condition required
		* for execution.
		* 
		* Bits 31-28
		*/
		bool CheckCondition(u8 cond_type) {
			switch (cond_type)
			{
			case 0x0: //EQ
				return zero;
			case 0x1: //NE
				return !zero;
			case 0x2: //CS
				return carry;
			case 0x3: //CC
				return !carry;
			case 0x4: //MI
				return sign;
			case 0x5: //PL
				return !sign;
			case 0x6: //VS
				return overflow;
			case 0x7: //VC
				return !overflow;
			case 0x8: //HI
				return carry && !zero;
			case 0x9: //LS
				return !carry || zero;
			case 0xA: //GE
				return sign == overflow;
			case 0xB: //LT
				return sign != overflow;
			case 0xC: //GT
				return !zero && sign == overflow;
			case 0xD: //LE
				return zero || sign != overflow;
			case 0xE: //ALWAYS
				return true;
			default:
				//FATAL
				return false;
				break;
			}
		}

		operator u32() {
			return *reinterpret_cast<u32*>(this);
		}

		CPSR& operator=(u32 value) {
			*reinterpret_cast<u32*>(this) = value;
			return *this;
		}

		void CarryAdd(uint64_t first, uint64_t second) {
			carry = (first + second) >> 32;
		}

		void OverflowAdd(u32 first, u32 second) {
			u32 res = first + second;

			overflow = (~(first ^ second) & (second ^ res)) >> 31;
		}

		void CarrySubtract(uint64_t first, uint64_t second) {
			carry = first >= second;
		}

		void OverflowSubtract(u32 first, u32 second) {
			u32 res = first - second;

			overflow = ((first ^ second) & (first ^ res)) >> 31;
		}
	};

	static_assert(sizeof(CPSR) == 4, "Invalid CPSR size");
#pragma pack(pop)
}
