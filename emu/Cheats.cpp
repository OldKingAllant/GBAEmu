#include "Cheats.hpp"

#include "GameShark.hpp"
#include "ActionReplay.hpp"
#include "CodeBreaker.hpp"

#include "Emulator.hpp"

#include "../common/Error.hpp"

#include <fmt/format.h>

namespace GBA::cheats {
	static std::vector<uint32_t> ParseHex(std::string const& line, bool alternate_size) {
		std::vector<uint32_t> directives{};
		uint32_t curr_digit = 0;
		uint32_t curr_val = 0;

		auto to_digit = [](char ch) {
			if (ch >= 'a' && ch <= 'f')
				return ch - 'a' + 0xA;
			return ch - '0';
		};

		for (char ch : line) {
			ch = tolower(ch);

			if (!isxdigit(ch))
				return {};

			uint32_t digit = to_digit(ch);
			curr_val |= (digit << (28 - curr_digit * 4));

			auto size_odd = bool(directives.size() & 1);
			uint32_t curr_limit = alternate_size ?
				(size_odd ? 4 : 8) : 8;

			if (++curr_digit == curr_limit) {
				directives.push_back(curr_val);
				curr_digit = 0;
				curr_val = 0;
			}
		}

		return directives;
	}

	CheatSet ParseCheat(std::vector<std::string> const& lines, CheatType ty) {
		std::vector<uint32_t> directives{};

		for (auto const& line : lines) {
			
			std::string new_line = line;
			new_line.erase(
				std::remove_if(
					new_line.begin(),
					new_line.end(),
					[](char ch) { return bool(isspace(ch)); }),
				new_line.end()
			);

			auto parsed_directives = ParseHex(new_line,
				ty == CheatType::CODE_BREAKER);
			directives.insert(directives.end(),
				parsed_directives.cbegin(),
				parsed_directives.cend());
		}

		CheatSet set{};

		set.ty = ty;

		switch (ty)
		{
		case CheatType::GAMESHARK:
			set.directives = ParseGameSharkDirectives(directives);
			break;
		case CheatType::ACTION_REPLAY:
			set.directives = ParseActionReplayDirectives(directives);
			break;
		case CheatType::CODE_BREAKER:
			set.directives = ParseCodeBreakerDirectives(directives);
			break;
		default:
			break;
		}

		bool has_hook = std::find_if(
			set.directives.cbegin(),
			set.directives.cend(),
			[](auto const& directive) {
				return directive.ty == DirectiveType::HOOK;
			}
		) != set.directives.cend();

		bool has_patch = std::find_if(
			set.directives.cbegin(),
			set.directives.cend(),
			[](auto const& directive) {
				return directive.ty == DirectiveType::ROM_PATCH;
			}
		) != set.directives.cend();

		set.contains_hook = has_hook;
		set.contains_pathces = has_patch;

		return set;
	}

	using DirectiveIt = std::list<CheatDirective>::const_iterator;
	
	static bool InterpretDirective(DirectiveIt& directive_iter, 
		emulation::Emulator* emu) {
		switch (directive_iter->ty)
		{
		case DirectiveType::NONE:
			break;
		case DirectiveType::WRITE_RAM_8: {
			auto& cmd = directive_iter->cmd.ram_write8;
			auto address = cmd.address + cmd.offset;
			auto value = cmd.value;
			emu->GetContext().bus.Write(address, value);
		}
			break;
		case DirectiveType::WRITE_RAM_16: {
			auto& cmd = directive_iter->cmd.ram_write16;
			auto address = cmd.address + cmd.offset;
			auto value = cmd.value;
			emu->GetContext().bus.Write(address, value);
		}
			break;
		case DirectiveType::WRITE_RAM_32: {
			auto& cmd = directive_iter->cmd.ram_write32;
			auto address = cmd.address;
			auto value = cmd.value;
			emu->GetContext().bus.Write(address, value);
		}
			break;
		case DirectiveType::IF_BLOCK: {
			uint32_t operand = directive_iter->cmd.if_block.operand;
			uint32_t compare{ 0 };
			uint32_t address = directive_iter->cmd.if_block.address;
			auto condition = directive_iter->cmd.if_block.cond;

			bool condition_is_always_false = false;

			auto& bus = emu->GetContext().bus;

			switch (directive_iter->cmd.if_block.operand_size)
			{
			case ConditionOperand::SIZE_8:
				compare = bus.Read<uint8_t>(address);
				break;
			case ConditionOperand::SIZE_16:
				compare = bus.Read<uint16_t>(address);
				break;
			case ConditionOperand::SIZE_32:
				compare = bus.Read<uint32_t>(address);
				break;
			case ConditionOperand::ALWAYS_FALSE:
				condition_is_always_false = true;
				break;
			}

			bool is_true = false;

			if (!condition_is_always_false) {
				switch (condition)
				{
				case Condition::EQ:
					is_true = operand == compare;
					break;
				case Condition::NE:
					is_true = operand != compare;
					break;
				case Condition::LT:
					is_true = int(compare) < int(operand);
					break;
				case Condition::GT:
					is_true = int(compare) > int(operand);
					break;
				case Condition::LTU:
					is_true = compare < operand;
					break;
				case Condition::GTU:
					is_true = compare > operand;
					break;
				case Condition::AND:
					is_true = operand && compare;
					break;
				default:
					break;
				}
			}

			auto& directive = directive_iter->cmd.if_block;

			if (is_true) {
				for (uint32_t curr_directive = 0;
					curr_directive < directive.then_size;
					curr_directive++) {
					directive_iter++;
					InterpretDirective(directive_iter, emu);
				}

				if (directive.has_else) {
					for(uint32_t skip = 0; skip < directive.else_size - 1;
						skip++, directive_iter++) {}
				}
			}
			else {
				for (uint32_t skip = 0; skip < directive.then_size - 1;
					skip++) {
					directive_iter++;
				}

				if (directive.has_else) {
					for (uint32_t curr_directive = 0;
						curr_directive < directive.else_size;
						curr_directive++) {
						directive_iter++;
						InterpretDirective(directive_iter, emu);
					}
				}
				else {
					directive_iter++;
				}
			}
		}
			break;
		case DirectiveType::INDIRECT_WRITE_16: {
			auto& directive = directive_iter->cmd.indirect16;
			auto address_indirect = directive.address;

			auto& bus = emu->GetContext().bus;

			auto effective_address = bus.Read<uint32_t>(address_indirect);
			effective_address += directive.offset;

			bus.Write(effective_address, directive.value);
		}
			break;
		case DirectiveType::ID_CODE:
		case DirectiveType::HOOK:
		case DirectiveType::ROM_PATCH:
		case DirectiveType::CRC:
			break;
		case DirectiveType::SLIDE_32: {
			auto& bus = emu->GetContext().bus;

			auto& directive = directive_iter->cmd.slide32;

			auto curr_address = directive.base;
			auto value = directive.init_value;
			auto address_inc = directive.address_inc << 2;
			auto value_inc = directive.value_inc;

			auto n_repeat = directive.repeat;

			while (n_repeat--) {
				bus.Write(curr_address, value);
				value += value_inc;
				curr_address += address_inc;
			}
		}
			break;
		case DirectiveType::SLIDE_16: {
			auto& bus = emu->GetContext().bus;

			auto& directive = directive_iter->cmd.slide16;

			auto curr_address = directive.base;
			auto value = directive.init_value;
			auto address_inc = directive.address_inc;
			auto value_inc = directive.value_inc;

			auto n_repeat = directive.repeat;

			while (n_repeat--) {
				bus.Write(curr_address, value);
				value += value_inc;
				curr_address += address_inc;
			}
		}
			break;
		default:
			fmt::println("[CHEATS] Encountered unknown instruction");
			error::DebugBreak();
			break;
		}

		return true;
	}

	bool RunCheatInterpreter(CheatSet& cheat_set, emulation::Emulator* emu) {
		auto directive_iter = cheat_set.directives.begin();

		while(directive_iter != cheat_set.directives.end()) {
			InterpretDirective(directive_iter, emu);
			directive_iter++;
		}

		return true;
	}

	bool ApplyCheatPatches(std::string const& name, CheatSet& cheat_set, emulation::Emulator* emu) {
		auto game_code =
			*std::bit_cast<uint32_t*>(
				&emu->GetContext().pack.GetHeader().gameMaker[0]
			);

		if (!cheat_set.contains_hook && !cheat_set.contains_pathces)
			return true;

		for (auto& directive : cheat_set.directives) {
			switch (directive.ty)
			{
			case DirectiveType::ID_CODE:
				if (directive.cmd.idcode.code != game_code)
					return false;
				break;
			case DirectiveType::HOOK: {
				auto address_base = memory::MEMORY_RANGE::ROM_REG_1;
				auto address = directive.cmd.hook.hook_address +
					(uint32_t(address_base) << 24);
				emu->AddHook(address, name);
			}
				break;
			case DirectiveType::ROM_PATCH: {
				auto& rom_patch = directive.cmd.patch;

				if (rom_patch.applied)
					break;

				rom_patch.applied = true;
				rom_patch.old_value = emu->GetContext()
					.pack.Patch(rom_patch.offset, rom_patch.value);
			}
				break;
			default:
				break;
			}
		}

		return true;
	}

	bool RestoreCheatPatches(std::string const& name, CheatSet& cheat_set, emulation::Emulator* emu) {
		if (!cheat_set.contains_hook && !cheat_set.contains_pathces)
			return true;

		for (auto& directive : cheat_set.directives) {
			switch (directive.ty)
			{
			case DirectiveType::HOOK: {
				emu->RemoveHook(name);
			}
				break;
			case DirectiveType::ROM_PATCH: {
				auto& rom_patch = directive.cmd.patch;

				if (!rom_patch.applied)
					break;

				rom_patch.applied = false;
				emu->GetContext()
					.pack.Patch(rom_patch.offset, rom_patch.old_value);
			}
				break;
			default:
				break;
			}
		}
		
		return true;
	}
}