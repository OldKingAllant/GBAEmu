#pragma once

#include "../common/Defs.hpp"
#include "./Emulator.hpp"

#include <fmt/format.h>

#include <fstream>
#include <string>

namespace GBA::savestate {
	using namespace GBA::common;

	//First thing in the savestate
	static constexpr u32 MAGIC = 0xdeadbeef;
	//Current savestate version
	static constexpr u32 VERSION = 2;

	static constexpr std::size_t STATE_UPPER_BOUND_SIZE = std::size_t(1024) * 1024;

	/// <summary>
	/// Savestate struct
	/// </summary>
	struct SaveState {
		SaveState(emulation::Emulator* emu_ptr) : emu(emu_ptr) {
			game_name.resize(12, '\0');
		}

		u32 magic{MAGIC};
		u32 version{VERSION};
		std::string game_name{};

		emulation::Emulator* emu;

		template <class Ar>
		void save(Ar& ar) const {
			ar(magic);
			ar(version);
			ar(game_name);

			ar(*emu);
		}

		template <class Ar>
		void load(Ar& ar) {
			ar(magic);
			ar(version);
			ar(game_name);

			if (magic != MAGIC || version != VERSION) {
				fmt::println("Loading savestate failed, invalid magic or version");
				return;
			}

			std::string curr_name( std::size_t(game_name.size()), char('\0') );
			auto const& title = emu->GetContext().pack.GetHeader().title;
			std::copy_n(title, game_name.size(), curr_name.data());

			if (curr_name != game_name) {
				fmt::println("Loading savestate failed, game id does not match");
				return;
			}

			ar(*emu);
		}
	};

	void LoadFromFile(std::ifstream& fd, emulation::Emulator* emu);
	void StoreToFile(std::ofstream& fd, emulation::Emulator* emu);

	void StoreToBuffer(std::string& buf, emulation::Emulator* emu);
	void LoadFromBuffer(std::string const& buf, emulation::Emulator* emu);
}