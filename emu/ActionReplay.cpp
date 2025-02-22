#include "ActionReplay.hpp"
#include "../common/Error.hpp"

#include <fmt/format.h>

#include <array>

namespace GBA::cheats {
	constexpr std::array g_AR_init_seeds = {
		uint32_t(0x7AA9648F), uint32_t(0x7FAE6994),
		uint32_t(0xC0EFAAD5), uint32_t(0x42712C57)
	};

	/*
	T1 and T2 tables, taken from mGBA at
	https://github.com/mgba-emu/mgba/blob/master/src/gba/cheats/parv3.c
	*/

	constexpr uint8_t _AR_T1[256] = {
		0xD0, 0xFF, 0xBA, 0xE5, 0xC1, 0xC7, 0xDB, 0x5B, 0x16, 0xE3, 0x6E, 0x26, 0x62, 0x31, 0x2E, 0x2A,
		0xD1, 0xBB, 0x4A, 0xE6, 0xAE, 0x2F, 0x0A, 0x90, 0x29, 0x90, 0xB6, 0x67, 0x58, 0x2A, 0xB4, 0x45,
		0x7B, 0xCB, 0xF0, 0x73, 0x84, 0x30, 0x81, 0xC2, 0xD7, 0xBE, 0x89, 0xD7, 0x4E, 0x73, 0x5C, 0xC7,
		0x80, 0x1B, 0xE5, 0xE4, 0x43, 0xC7, 0x46, 0xD6, 0x6F, 0x7B, 0xBF, 0xED, 0xE5, 0x27, 0xD1, 0xB5,
		0xD0, 0xD8, 0xA3, 0xCB, 0x2B, 0x30, 0xA4, 0xF0, 0x84, 0x14, 0x72, 0x5C, 0xFF, 0xA4, 0xFB, 0x54,
		0x9D, 0x70, 0xE2, 0xFF, 0xBE, 0xE8, 0x24, 0x76, 0xE5, 0x15, 0xFB, 0x1A, 0xBC, 0x87, 0x02, 0x2A,
		0x58, 0x8F, 0x9A, 0x95, 0xBD, 0xAE, 0x8D, 0x0C, 0xA5, 0x4C, 0xF2, 0x5C, 0x7D, 0xAD, 0x51, 0xFB,
		0xB1, 0x22, 0x07, 0xE0, 0x29, 0x7C, 0xEB, 0x98, 0x14, 0xC6, 0x31, 0x97, 0xE4, 0x34, 0x8F, 0xCC,
		0x99, 0x56, 0x9F, 0x78, 0x43, 0x91, 0x85, 0x3F, 0xC2, 0xD0, 0xD1, 0x80, 0xD1, 0x77, 0xA7, 0xE2,
		0x43, 0x99, 0x1D, 0x2F, 0x8B, 0x6A, 0xE4, 0x66, 0x82, 0xF7, 0x2B, 0x0B, 0x65, 0x14, 0xC0, 0xC2,
		0x1D, 0x96, 0x78, 0x1C, 0xC4, 0xC3, 0xD2, 0xB1, 0x64, 0x07, 0xD7, 0x6F, 0x02, 0xE9, 0x44, 0x31,
		0xDB, 0x3C, 0xEB, 0x93, 0xED, 0x9A, 0x57, 0x05, 0xB9, 0x0E, 0xAF, 0x1F, 0x48, 0x11, 0xDC, 0x35,
		0x6C, 0xB8, 0xEE, 0x2A, 0x48, 0x2B, 0xBC, 0x89, 0x12, 0x59, 0xCB, 0xD1, 0x18, 0xEA, 0x72, 0x11,
		0x01, 0x75, 0x3B, 0xB5, 0x56, 0xF4, 0x8B, 0xA0, 0x41, 0x75, 0x86, 0x7B, 0x94, 0x12, 0x2D, 0x4C,
		0x0C, 0x22, 0xC9, 0x4A, 0xD8, 0xB1, 0x8D, 0xF0, 0x55, 0x2E, 0x77, 0x50, 0x1C, 0x64, 0x77, 0xAA,
		0x3E, 0xAC, 0xD3, 0x3D, 0xCE, 0x60, 0xCA, 0x5D, 0xA0, 0x92, 0x78, 0xC6, 0x51, 0xFE, 0xF9, 0x30
	};

	constexpr uint8_t _AR_T2[256] = {
		0xAA, 0xAF, 0xF0, 0x72, 0x90, 0xF7, 0x71, 0x27, 0x06, 0x11, 0xEB, 0x9C, 0x37, 0x12, 0x72, 0xAA,
		0x65, 0xBC, 0x0D, 0x4A, 0x76, 0xF6, 0x5C, 0xAA, 0xB0, 0x7A, 0x7D, 0x81, 0xC1, 0xCE, 0x2F, 0x9F,
		0x02, 0x75, 0x38, 0xC8, 0xFC, 0x66, 0x05, 0xC2, 0x2C, 0xBD, 0x91, 0xAD, 0x03, 0xB1, 0x88, 0x93,
		0x31, 0xC6, 0xAB, 0x40, 0x23, 0x43, 0x76, 0x54, 0xCA, 0xE7, 0x00, 0x96, 0x9F, 0xD8, 0x24, 0x8B,
		0xE4, 0xDC, 0xDE, 0x48, 0x2C, 0xCB, 0xF7, 0x84, 0x1D, 0x45, 0xE5, 0xF1, 0x75, 0xA0, 0xED, 0xCD,
		0x4B, 0x24, 0x8A, 0xB3, 0x98, 0x7B, 0x12, 0xB8, 0xF5, 0x63, 0x97, 0xB3, 0xA6, 0xA6, 0x0B, 0xDC,
		0xD8, 0x4C, 0xA8, 0x99, 0x27, 0x0F, 0x8F, 0x94, 0x63, 0x0F, 0xB0, 0x11, 0x94, 0xC7, 0xE9, 0x7F,
		0x3B, 0x40, 0x72, 0x4C, 0xDB, 0x84, 0x78, 0xFE, 0xB8, 0x56, 0x08, 0x80, 0xDF, 0x20, 0x2F, 0xB9,
		0x66, 0x2D, 0x60, 0x63, 0xF5, 0x18, 0x15, 0x1B, 0x86, 0x85, 0xB9, 0xB4, 0x68, 0x0E, 0xC6, 0xD1,
		0x8A, 0x81, 0x2B, 0xB3, 0xF6, 0x48, 0xF0, 0x4F, 0x9C, 0x28, 0x1C, 0xA4, 0x51, 0x2F, 0xD7, 0x4B,
		0x17, 0xE7, 0xCC, 0x50, 0x9F, 0xD0, 0xD1, 0x40, 0x0C, 0x0D, 0xCA, 0x83, 0xFA, 0x5E, 0xCA, 0xEC,
		0xBF, 0x4E, 0x7C, 0x8F, 0xF0, 0xAE, 0xC2, 0xD3, 0x28, 0x41, 0x9B, 0xC8, 0x04, 0xB9, 0x4A, 0xBA,
		0x72, 0xE2, 0xB5, 0x06, 0x2C, 0x1E, 0x0B, 0x2C, 0x7F, 0x11, 0xA9, 0x26, 0x51, 0x9D, 0x3F, 0xF8,
		0x62, 0x11, 0x2E, 0x89, 0xD2, 0x9D, 0x35, 0xB1, 0xE4, 0x0A, 0x4D, 0x93, 0x01, 0xA7, 0xD1, 0x2D,
		0x00, 0x87, 0xE2, 0x2D, 0xA4, 0xE9, 0x0A, 0x06, 0x66, 0xF8, 0x1F, 0x44, 0x75, 0xB5, 0x6B, 0x1C,
		0xFC, 0x31, 0x09, 0x48, 0xA3, 0xFF, 0x92, 0x12, 0x58, 0xE9, 0xFA, 0xAE, 0x4F, 0xE2, 0xB4, 0xCC
	};

	static std::pair<uint32_t, uint32_t> _AR_Decrypt_Code(uint32_t l, uint32_t r,
		std::array<uint32_t, 4> const& seeds) {
		constexpr uint32_t MAGIC = 0x9E3779B9;
		uint32_t sum = 0xC6EF3720;

		for (uint32_t idx = 0; idx < 32; idx++) {
			r -= ((l << 4) + seeds[2]) ^ (l + sum) ^ ((l >> 5) + seeds[3]);
			l -= ((r << 4) + seeds[0]) ^ (r + sum) ^ ((r >> 5) + seeds[1]);
			sum -= MAGIC;
		}

		return { l, r };
	}

	static void _AR_Change_Seeds(uint32_t param, std::array<uint32_t, 4>& seeds) {
		uint32_t xx = (param >> 8) & 0xFF;
		uint32_t yy = param & 0xFF;

		for (uint32_t y = 0; y < 4; y++) {
			for (uint32_t x = 0; x < 4; x++) {
				uint32_t z = _AR_T1[(xx + x) & 0xFF] +
					uint32_t(_AR_T2[(yy + y) & 0xFF]);
				seeds[y] = (seeds[y] << 8) + (z & 0xFF);
			}
		}
	}

	template <typename It>
	static bool _AR_Cond(std::list<CheatDirective>& list, It& directive_iter,
		std::array<uint32_t, 4>& seeds, 
		uint32_t address,
		uint32_t value) {
		uint32_t cond_descriptor = (address >> 24) & 0xFF;

		auto condition = AR_Cond(cond_descriptor & AR_COND_MASK);
		auto size      = AR_CondSize(cond_descriptor & AR_COND_SIZE_MASK);
		auto action    = AR_CondAction(cond_descriptor & AR_COND_ACTION_MASK);

		if (action == AR_CondAction::OFF) {
			fmt::println("[CHEATS] Unimplemented AR action OFF");
			return false;
		}

		directive_detail::IfBlock if_block{};

		switch (condition)
		{
		case AR_Cond::EQ:
			if_block.cond = Condition::EQ;
			break;
		case AR_Cond::NE:
			if_block.cond = Condition::NE;
			break;
		case AR_Cond::LT:
			if_block.cond = Condition::LT;
			break;
		case AR_Cond::GT:
			if_block.cond = Condition::GT;
			break;
		case AR_Cond::LTU:
			if_block.cond = Condition::LTU;
			break;
		case AR_Cond::GTU:
			if_block.cond = Condition::GTU;
			break;
		case AR_Cond::AND:
			if_block.cond = Condition::AND;
			break;
		default:
			break;
		}

		auto operand_size_bytes{ uint32_t(0) };

		switch (size)
		{
		case AR_CondSize::SIZE_8:
			if_block.operand_size = ConditionOperand::SIZE_8;
			operand_size_bytes = 1;
			break;
		case AR_CondSize::SIZE_16:
			if_block.operand_size = ConditionOperand::SIZE_16;
			operand_size_bytes = 2;
			break;
		case AR_CondSize::SIZE_32:
			if_block.operand_size = ConditionOperand::SIZE_32;
			operand_size_bytes = 4;
			break;
		case AR_CondSize::FALSE:
			if_block.operand_size = ConditionOperand::ALWAYS_FALSE;
			break;
		default:
			break;
		}

		if_block.has_else = false;
		if_block.else_location = 0;
		if_block.else_size = 0;

		switch (action)
		{
		case AR_CondAction::NEXT_1:
			if_block.then_size = 1;
			break;
		case AR_CondAction::NEXT_2:
			if_block.then_size = 2;
			break;
		case AR_CondAction::BLOCK:
			fmt::println("[CHEATS] Unimplemented AR if action: BLOCK");
			error::DebugBreak();
			break;
		case AR_CondAction::OFF:
			break;
		default:
			break;
		}

		auto high_address_nibble = address & 0x00F0'0000;
		auto low_address = address & 0x000F'FFFF;

		if_block.address = (high_address_nibble << 4) | low_address;
		if_block.operand = value & (0xFF'FF'FF'FF >> ((4 - operand_size_bytes) * 8));

		CheatDirective directive{};

		directive.ty = DirectiveType::IF_BLOCK;
		directive.cmd.if_block = if_block;

		list.push_back(directive);
		
		return true;
	}

	template <typename It>
	static bool _AR_Special(std::list<CheatDirective>& list, It& directive_iter,
		std::array<uint32_t, 4>& seeds,
		uint32_t address,
		uint32_t value) {
		auto special_match = AR_OpcodeMatchSpecial(value >> 24);

		switch (special_match)
		{
		case AR_OpcodeMatchSpecial::ROM_PATCH_1:
		case AR_OpcodeMatchSpecial::ROM_PATCH_2:
		case AR_OpcodeMatchSpecial::ROM_PATCH_3:
		case AR_OpcodeMatchSpecial::ROM_PATCH_4: {
			uint32_t enc_value = *directive_iter++;
			uint32_t enc_zero = *directive_iter++;
			auto decrypted = _AR_Decrypt_Code(
				enc_value, enc_zero, seeds
			);

			auto patch_value = decrypted.first;
			auto zero = decrypted.second;

			if (zero != 0x0) {
				fmt::println("[CHEATS] Warning! Encountered AR patch without zero");
			}

			auto offset = value & 0xFF'FF'FF;

			CheatDirective directive{};

			directive.ty = DirectiveType::ROM_PATCH;
			directive.cmd.patch = {
				.applied = false,
				.offset  = offset << 1,
				.value   = uint16_t(patch_value)
			};

			list.push_back(directive);
		}
			break;
		case AR_OpcodeMatchSpecial::SLIDE_32: {
			uint32_t enc_value = *directive_iter++;
			uint32_t enc_step_inc = *directive_iter++;
			auto decrypted = _AR_Decrypt_Code(
				enc_value, enc_step_inc, seeds
			);

			auto high_address_nibble = value & 0x00F0'0000;
			auto low_address = value & 0x000F'FFFF;

			auto base_address = (high_address_nibble << 4) | low_address;
			auto init_value = decrypted.first;

			auto step_inc = decrypted.second;

			auto value_inc = (step_inc >> 24) & 0xFF;
			auto repeat = (step_inc >> 16) & 0xFF;
			auto address_inc = step_inc & 0xFFFF;

			CheatDirective directive{};

			directive.ty = DirectiveType::SLIDE_32;
			directive.cmd.slide32 = {
				.base        = base_address,
				.init_value  = init_value,
				.address_inc = address_inc,
				.value_inc   = value_inc,
				.repeat      = repeat
			};

			list.push_back(directive);
		}
			break;
		default:
			fmt::println("[CHEATS] Unimplemented AR special: {:#x}",
				uint32_t(special_match));
			return false;
		}

		return true;
	}

	template <typename It>
	static bool _AR_ListAppend(std::list<CheatDirective>& list, It& directive_iter, 
		std::array<uint32_t, 4>& seeds) {
		uint32_t enc_l{}, enc_r{};

		enc_l = *directive_iter++;
		enc_r = *directive_iter++;

		auto opcodes = _AR_Decrypt_Code(
			enc_l, enc_r, seeds
		);

		auto address = opcodes.first;
		auto value = opcodes.second;

		CheatDirective directive{};

		auto value_match = AR_OpcodeMatchSpecial(value);
		auto address_match = AR_OpcodeMatchSpecial(address);

		switch (value_match)
		{
		case AR_OpcodeMatchSpecial::ID_CODE:
			directive.ty = DirectiveType::ID_CODE;
			directive.cmd.idcode = {
				.code = address
			};
			list.push_back(directive);
			return true;
		default:
			break;
		}

		switch (address_match)
		{
		case AR_OpcodeMatchSpecial::SEEDS:
			fmt::println("[CHEATS] Changing AR seeds");
			_AR_Change_Seeds(value, seeds);
			return true;
		case AR_OpcodeMatchSpecial::SPECIAL:
			if (value != 0x0) {
				return _AR_Special(list, directive_iter, 
					seeds, address, value);
			}
			return true;
		default:
			break;
		}

		auto opcode = AR_OpcodeMatch(address >> 24);

		auto high_address_nibble = address & 0x00F0'0000;
		auto low_address = address & 0x000F'FFFF;

		if (uint32_t(opcode) & uint32_t(AR_OpcodeMatch::IF_COND)) {
			return _AR_Cond(list, directive_iter, seeds,
				address, value);
		}

		switch (opcode)
		{
		case AR_OpcodeMatch::HOOK:
			directive.ty = DirectiveType::HOOK;
			directive.cmd.hook = {
				.hook_address = address & 0xFF'FF'FF,
				.params = uint16_t(value)
			};
			break;
		case AR_OpcodeMatch::WRITE_8:
			directive.ty = DirectiveType::WRITE_RAM_8;
			directive.cmd.ram_write8 = { 
				.address = (high_address_nibble << 4) | low_address,
				.offset = (value >> 8) & 0xFF'FF'FF,
				.value = uint8_t(value)
			};
			break;
		case AR_OpcodeMatch::WRITE_16:
			directive.ty = DirectiveType::WRITE_RAM_16;
			directive.cmd.ram_write16 = {
				.address = (high_address_nibble << 4) | low_address,
				.offset = ((value >> 16) & 0xFFFF) << 1,
				.value = uint16_t(value)
			};
			break;
		case AR_OpcodeMatch::WRITE_32:
			directive.ty = DirectiveType::WRITE_RAM_32;
			directive.cmd.ram_write32 = {
				.address = (high_address_nibble << 4) | low_address,
				.value = value
			};
			break;
		case AR_OpcodeMatch::INDIRECT_16:
			directive.ty = DirectiveType::INDIRECT_WRITE_16;
			directive.cmd.indirect16 = {
				.address = (high_address_nibble << 4) | low_address,
				.offset  = ((value >> 16) & 0xFF'FF) << 1,
				.value   = uint16_t(value)
			};
			break;
		default:
			fmt::println("[CHEATS] Unimplemented AR opcode {:#x}", uint32_t(opcode));
			return false;
		}

		list.push_back(directive);

		return true;
	}

	std::list<CheatDirective> ParseActionReplayDirectives(std::vector<uint32_t> const& directives) {
		std::list<CheatDirective> parsed_list{};

		if (directives.size() & 1) {
			fmt::println("[CHEATS] Unexpected number of directives, must be even");
			return parsed_list;
		}

		auto seeds = g_AR_init_seeds;

		auto iter = directives.cbegin();

		while (iter != directives.end()) {
			if (!_AR_ListAppend(parsed_list, iter, seeds))
				return {};
		}

		return parsed_list;
	}
}