#include "BiosHle.hpp"

#include "../emu/Emulator.hpp"
#include "../common/Error.hpp"

#include "BiosMath.hpp"

#include <unordered_map>
#include <string>
#include <vector>
#include <sstream>

#include <fmt/format.h>

namespace GBA::hle {
	using namespace common;

	enum class ArgType {
		INT32
	};

	struct FunctionDescriptor {
		std::string name = {};
		std::vector<std::pair<std::string, ArgType>> arguments = {};
	};

	static ArgType GetDatatypeFromString(std::string const& type_str) {
		static const std::unordered_map<std::string, ArgType> types = {
			{"INT32", ArgType::INT32}
		};

		auto iter = types.find(type_str);

		if (iter == types.cend()) {
			fmt::println("[HLE] Could not find datatype for \"{}\"",
				type_str);
			return ArgType::INT32;
		}

		return iter->second;
	}

	static void InsertFunction(std::unordered_map<u8, FunctionDescriptor>& table, 
		u8 id, std::string const& signature) {
		auto open_bracket_loc = signature.find_first_of('(');
		auto closed_bracket_loc = signature.find_first_of(')');

		if (open_bracket_loc == std::string::npos ||
			closed_bracket_loc == std::string::npos)
			return;

		auto fname = signature.substr(0, open_bracket_loc);

		FunctionDescriptor descriptor{};

		descriptor.name = fname;
		descriptor.arguments = {};

		auto comma_pos = signature.find_first_of(',');

		while (comma_pos != std::string::npos) {
			auto argument_str = signature.substr(open_bracket_loc + 1,
				comma_pos - (open_bracket_loc + 1));

			auto equal_pos = argument_str.find_first_of('=');
			auto argument_name = argument_str.substr(0, equal_pos);
			auto arg_type = argument_str.substr(equal_pos + 1);

			ArgType ty = GetDatatypeFromString(arg_type);

			descriptor.arguments.emplace_back(argument_name, ty);

			open_bracket_loc = comma_pos;
			comma_pos = signature.find_first_of(',', comma_pos + 1);
		}

		table.insert({ id, descriptor  });
	}

	static std::unordered_map<u8, FunctionDescriptor> GenerateDescriptorTable() {
		std::unordered_map<u8, FunctionDescriptor> ftable{};

		std::unordered_map<std::string, u8> signatures = {
			{ "Div(num=INT32,denom=INT32,)", 0x6 }
		};

		for (auto const& [signature, id] : signatures) {
			InsertFunction(ftable, id, signature);
		}

		return ftable;
	}

	static const auto DESCRIPTOR_TABLE = GenerateDescriptorTable();

	static void LogParameter(u8 param_pos,
		std::pair<std::string, ArgType> const& param, memory::Bus* bus, 
		cpu::CPUContext& ctx, std::ostringstream& os) {
		if (param_pos >= 4) {
			fmt::println("[HLE] Unimplemented, logging more than 4 parameters");
			error::DebugBreak();
		}

		os << param.first << "=";

		auto param_value{ ctx.m_regs.GetReg(param_pos) };

		switch (param.second)
		{
		case ArgType::INT32: {
			auto param_as_int = int32_t(param_value);
			os << param_as_int;
		}
			break;
		default:
			os << "UNIMPLEMENTED";
			break;
		}
	}

	static void LogFunctionCall(uint8_t id, memory::Bus* bus, cpu::CPUContext& ctx) {
		auto descriptor_iter = DESCRIPTOR_TABLE.find(id);

		if (descriptor_iter == DESCRIPTOR_TABLE.cend()) {
			fmt::println("[HLE] Function {:#04x} called", id);
			return;
		}

		auto const& descriptor = descriptor_iter->second;

		std::ostringstream os{};

		os << descriptor.name << "(";

		for (u8 pos = 0; auto const& param : descriptor.arguments) {
			LogParameter(pos, param, bus, ctx, os);
			os << ",";
			pos++;
		}

		os << ")";

		fmt::println("[HLE] {}", os.str());
	}

	static FunctionTable CreateFunctionTable() {
		FunctionTable ftable{};

		std::fill_n(ftable.begin(), ftable.size(), nullptr);

		math::RegisterMath(ftable);

		return ftable;
	}

	static auto FUNCTION_TABLE = CreateFunctionTable();

	void RegisterFunction(FunctionTable& table, u8 id, FunctionHandler handler) {
		table[id] = handler;
	}

	bool HleBiosRoutine(uint8_t id, memory::Bus* bus, cpu::CPUContext& ctx, bool& branch) {
		if (ctx.m_emu->IsHleLogEnabled()) {
			LogFunctionCall(id, bus, ctx);
		}

		if (FUNCTION_TABLE[id] != nullptr) {
			bool handled = FUNCTION_TABLE[id](bus, ctx, branch);

			if (handled) {
				bus->LoadBiosSWIOpcode();
				return true;
			}
		}

		return false;
	}
}