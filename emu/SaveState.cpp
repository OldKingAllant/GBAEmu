#include "SaveState.hpp"

#include "../thirdparty/cereal/include/cereal/archives/binary.hpp"
#include "../thirdparty/cereal/include/cereal/types/vector.hpp"
#include "../thirdparty/cereal/include/cereal/types/array.hpp"
#include "../thirdparty/cereal/include/cereal/types/string.hpp"

namespace GBA::savestate {

	void LoadFromFile(std::ifstream& fd, emulation::Emulator* emu) {
		cereal::BinaryInputArchive ar{ fd };

		SaveState state{ emu };
		ar(state);
	}

	void StoreToFile(std::ofstream& fd, emulation::Emulator* emu) {
		cereal::BinaryOutputArchive ar{ fd };

		auto const& title = emu->GetContext().pack.GetHeader().title;

		SaveState state{ emu };
		std::copy_n(title, state.game_name.size(), state.game_name.data());
		ar(state);
	}
}