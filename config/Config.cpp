#include "Config.hpp"

#include <filesystem>

namespace GBA::config {
	Config::Config() :
		data{} {}

	bool Config::Load(std::string_view path) {
		if (!std::filesystem::exists(path))
			return false;

		mINI::INIFile file(path.data());

		return file.read(data);
	}

	bool Config::Store(std::string_view path) {
		mINI::INIFile file(path.data());
		return file.write(data, true);
	}

	void Config::Default() {
		mINI::INIMap<std::string> section{};

		section.set("skip", "true");
		section.set("file", "gba_bios.bin");

		data.set({ { "BIOS", section } });

		section.clear();

		section.set("start_paused", "true");
		section.set("scale", "4");
		section.set("quick_save_path", "./quick_state");
		section.set("rewind_buf_size", "30");
		section.set("rewind_interval_seconds", "1");
		section.set("rewind_enable", "true");
		section.set("game_save_path", "./saves");
		section.set("startup_load_save", "true");

		data.set({ { "EMU", section } });
	}

	bool Config::Change(std::string const& section, std::string const& key, std::string const& val) {
		if (!data.has(section))
			return false;

		if (!data.get(section).has(key))
			return false;

		data.get(section).set(key, val);

		return true;
	}
}