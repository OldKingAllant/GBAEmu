#include "CodeBreaker.hpp"

#include <fmt/format.h>

namespace GBA::cheats {
	struct _CB_EncryptionContext {
		bool must_decrypt = false;
		uint32_t randomizer = 0;
		uint8_t swaplist[0x30] = {};
		uint32_t seedlist[4] = {};
		uint32_t master = {};
		static constexpr auto SWAP_SIZE = uint32_t(
				sizeof(swaplist) /
				sizeof(uint8_t)
		);
	};

	/*
	WARNING: Most of the code used
	for decryption has been copied
	directly from mGBA, since I could
	not find any other useful resource
	on how the decryption works
	*/

	static uint32_t _CB_Random(_CB_EncryptionContext& ctx) {
		uint32_t x{};

		constexpr uint32_t MULTIPLY_CONST{0x41C64E6D};
		constexpr uint32_t ADD_CONST{0x3039};

		ctx.randomizer = ctx.randomizer * MULTIPLY_CONST + ADD_CONST;
		x = (ctx.randomizer << 14) & 0xC0000000;
		ctx.randomizer = ctx.randomizer * MULTIPLY_CONST + ADD_CONST;
		x |= ((ctx.randomizer >> 1) & 0x3FFF8000);
		ctx.randomizer = ctx.randomizer * MULTIPLY_CONST + ADD_CONST;
		x |= ((ctx.randomizer >> 16) & 0x00007FFF);

		return x;
	}

	////////////////////////////////////////////////
	//////////////BEGIN MGBA STUFF//////////////////

	static size_t _CB_SwapIndex(_CB_EncryptionContext& ctx) {
		uint32_t roll = _CB_Random(ctx);
		uint32_t count = _CB_EncryptionContext::SWAP_SIZE;

		if (roll == count) {
			roll = 0;
		}

		if (roll < count) {
			return roll;
		}

		uint32_t bit = 1;

		while (count < 0x10000000 && count < roll) {
			count <<= 4;
			bit <<= 4;
		}

		while (count < 0x80000000 && count < roll) {
			count <<= 1;
			bit <<= 1;
		}

		uint32_t mask;
		while (true) {
			mask = 0;
			if (roll >= count) {
				roll -= count;
			}
			if (roll >= count >> 1) {
				roll -= count >> 1;
				mask |= std::rotr(bit, 1);
			}
			if (roll >= count >> 2) {
				roll -= count >> 2;
				mask |= std::rotr(bit, 2);
			}
			if (roll >= count >> 3) {
				roll -= count >> 3;
				mask |= std::rotr(bit, 3);
			}
			if (!roll || !(bit >> 4)) {
				break;
			}
			bit >>= 4;
			count >>= 4;
		}

		mask &= 0xE0000000;
		if (!mask || !(bit & 7)) {
			return roll;
		}

		if (mask & std::rotr(bit, 3)) {
			roll += count >> 3;
		}
		if (mask & std::rotr(bit, 2)) {
			roll += count >> 2;
		}
		if (mask & std::rotr(bit, 1)) {
			roll += count >> 1;
		}

		return roll;
	}

	static bool _CB_ChangeEncryption(uint32_t param1, uint32_t param2,
		_CB_EncryptionContext& enc_ctx) {
		
		enc_ctx.randomizer = (param2 & 0xFF) ^ 0x1111;
		size_t i;
		// Populate the initial seed table
		for (i = 0; i < _CB_EncryptionContext::SWAP_SIZE; ++i) {
			enc_ctx.swaplist[i] = uint8_t(i);
		}
		// Swap pseudo-random table entries based on the input code
		for (i = 0; i < 0x50; ++i) {
			size_t x = _CB_SwapIndex(enc_ctx);
			size_t y = _CB_SwapIndex(enc_ctx);
			uint8_t swap = enc_ctx.swaplist[x];
			enc_ctx.swaplist[x] = enc_ctx.swaplist[y];
			enc_ctx.swaplist[y] = swap;
		}

		// Spin the RNG some to make the initial seed
		enc_ctx.randomizer = 0x4EFAD1C3;
		for (i = 0; i < ((param1 >> 24) & 0xF); ++i) {
			enc_ctx.randomizer = _CB_Random(enc_ctx);
		}

		enc_ctx.seedlist[2] = _CB_Random(enc_ctx);
		enc_ctx.seedlist[3] = _CB_Random(enc_ctx);

		enc_ctx.randomizer = (param2 >> 8) ^ 0xF254;
		for (i = 0; i < (param2 >> 8); ++i) {
			enc_ctx.randomizer = _CB_Random(enc_ctx);
		}

		enc_ctx.seedlist[0] = _CB_Random(enc_ctx);
		enc_ctx.seedlist[1] = _CB_Random(enc_ctx);

		enc_ctx.master = param1;

		enc_ctx.must_decrypt = true;
		return true;
	}

	static void _CB_LoadByteswap(uint8_t* buffer, uint32_t op1, uint16_t op2) {
		buffer[0] = uint8_t(op1 >> 24);
		buffer[1] = uint8_t(op1 >> 16);
		buffer[2] = uint8_t(op1 >> 8);
		buffer[3] = uint8_t(op1);
		buffer[4] = uint8_t(op2 >> 8);
		buffer[5] = uint8_t(op2);
	}

	static void _CB_StoreByteSwap(uint8_t* buffer, uint32_t* op1, uint16_t* op2) {
		*op1 = (buffer[0] << 24) | (buffer[1] << 16) | (buffer[2] << 8) | buffer[3];
		*op2 = (buffer[4] << 8) | buffer[5];
	}

	static std::pair<uint32_t, uint32_t> _CB_Decrypt(uint32_t param1, uint16_t param2,
		_CB_EncryptionContext& ctx) {
		uint8_t buffer[6] = {};
		int i = {};

		_CB_LoadByteswap(buffer, param1, param2);

		for (i = _CB_EncryptionContext::SWAP_SIZE - 1; i >= 0; --i) {
			size_t offsetX = i >> 3;
			size_t offsetY = ctx.swaplist[i] >> 3;
			int bitX = i & 7;
			int bitY = ctx.swaplist[i] & 7;

			uint8_t x = (buffer[offsetX] >> bitX) & 1;
			uint8_t y = (buffer[offsetY] >> bitY) & 1;
			uint8_t x2 = buffer[offsetX] & ~(1 << bitX);
			if (y) {
				x2 |= 1 << bitX;
			}
			buffer[offsetX] = x2;

			// This can't be moved earlier due to pointer aliasing
			uint8_t y2 = buffer[offsetY] & ~(1 << bitY);
			if (x) {
				y2 |= 1 << bitY;
			}
			buffer[offsetY] = y2;
		}

		_CB_StoreByteSwap(buffer, &param1, &param2);

		param1 ^= ctx.seedlist[0];
		param2 ^= ctx.seedlist[1];

		_CB_LoadByteswap(buffer, param1, param2);

		uint32_t master = ctx.master;
		for (i = 0; i < 5; ++i) {
			buffer[i] ^= (master >> 8) ^ buffer[i + 1];
		}
		buffer[5] ^= master >> 8;

		for (i = 5; i > 0; --i) {
			buffer[i] ^= master ^ buffer[i - 1];
		}
		buffer[0] ^= master;

		_CB_StoreByteSwap(buffer, &param1, &param2);

		param1 ^= ctx.seedlist[2];
		param2 ^= ctx.seedlist[3];

		return { param1, uint32_t(param2) };
	}

	//////////////////END MGBA//////////////////////
	////////////////////////////////////////////////

	template <typename It>
	static bool _CB_ListAppend(std::list<CheatDirective>& list, It& directive_iter,
		_CB_EncryptionContext& enc_ctx) {
		uint32_t address = *directive_iter++;
		uint32_t value = *directive_iter++;

		value >>= 16;

		if (enc_ctx.must_decrypt) {
			auto decrypted = _CB_Decrypt(
				address, uint16_t(value), enc_ctx
			);

			address = decrypted.first;
			value = decrypted.second;
		}

		auto opcode = CB_OpcodeMatch(address >> 28);

		CheatDirective directive{};

		auto effective_address = address & 0x0FFF'FFFF;

		switch (opcode)
		{
		case CB_OpcodeMatch::GAME_ID: {
			auto has_crc = bool((value >> 3) & 1);

			if (!has_crc)
				return true;

			directive.ty = DirectiveType::CRC;
			directive.cmd.crc = {
				.crc = uint16_t(address),
				.disable_irq = bool(value & 2)
			};
		}
			break;
		case CB_OpcodeMatch::HOOK: {
			directive.ty = DirectiveType::HOOK;
			directive.cmd.hook = {
				.hook_address = effective_address,
				.params = 0
			};
		}
			break;
		case CB_OpcodeMatch::WRITE_8: {
			directive.ty = DirectiveType::WRITE_RAM_8;
			directive.cmd.ram_write8 = {
				.address = effective_address,
				.offset = 0,
				.value = uint8_t(value)
			};
		}
			break;
		case CB_OpcodeMatch::SLIDE_16: {
			uint32_t param1 = *directive_iter++;
			uint32_t param2 = *directive_iter++;
			param2 >>= 16;

			if (enc_ctx.must_decrypt) {
				auto decrypted = _CB_Decrypt(
					param1, param2, enc_ctx
				);

				param1 = decrypted.first;
				param2 = decrypted.second;
			}

			directive.ty = DirectiveType::SLIDE_16;
			directive.cmd.slide16 = {
				.base = effective_address,
				.init_value = uint16_t(value),
				.address_inc = uint16_t(param2),
				.value_inc = uint16_t(param2),
				.repeat = uint16_t(param1)
			};
		}
			break;
		case CB_OpcodeMatch::IF_EQ:
			directive.ty = DirectiveType::IF_BLOCK;
			directive.cmd.if_block = {
				.cond = Condition::EQ,
				.operand_size = ConditionOperand::SIZE_16,
				.has_else = false,
				.else_location = 0,
				.else_size = 0,
				.then_size = 1,
				.address = effective_address,
				.operand = value
			};
			break;
		case CB_OpcodeMatch::WRITE_16:
			directive.ty = DirectiveType::WRITE_RAM_16;
			directive.cmd.ram_write16 = {
				.address = effective_address,
				.offset = 0,
				.value = uint16_t(value)
			};
			break;
		case CB_OpcodeMatch::ENCRYPT:
			return _CB_ChangeEncryption(address, value, enc_ctx);
			break;
		default:
			fmt::println("[CHEATS] Unknown CB opcode: {:#x}",
				uint32_t(opcode));
			return false;
		}

		list.push_back(directive);

		return true;
	}

	std::list<CheatDirective> ParseCodeBreakerDirectives(std::vector<uint32_t> const& directives) {
		std::list<CheatDirective> parsed_directives{};
		_CB_EncryptionContext enc_ctx{};

		if (directives.size() & 1) {
			fmt::println("[CHEATS] Unexpected number of directives, must be even");
			return parsed_directives;
		}

		auto directive_iter = directives.cbegin();

		while (directive_iter != directives.cend()) {
			if (!_CB_ListAppend(parsed_directives, directive_iter, enc_ctx))
				return {};
		}

		return parsed_directives;
	}
}