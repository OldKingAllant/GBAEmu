#include "../../../gamepack/backups/Database.hpp"

#include <filesystem>
#include <sstream>

#include "../../../thirdparty/mini/ini.h"

namespace GBA::gamepack::backups {
	BackupTypeSize BackupDatabase::GetBackupTypeAndSize(std::string_view db_path, std::string rom_name) {
		if (!std::filesystem::exists(db_path) || !std::filesystem::is_regular_file(db_path))
			return BackupTypeSize::NONE;

		mINI::INIFile db_file(db_path.data());

		mINI::INIStructure database;

		db_file.read(database);

		if (!database.has(rom_name) || !database.get(rom_name).has("backup_type"))
			return BackupTypeSize::NONE;

		std::string const& type_string = database
			.get(rom_name)
			.get("backup_type");

		if (type_string == "EEPROM_8K")
			return BackupTypeSize::EEPROM_8K;
		else if (type_string == "EEPROM_512")
			return BackupTypeSize::EEPROM_512;
		else if (type_string == "FLASH_512")
			return BackupTypeSize::FLASH_512;
		else if (type_string == "FLASH_1M")
			return BackupTypeSize::FLASH_1M;
		else if (type_string == "SRAM")
			return BackupTypeSize::SRAM;

		return BackupTypeSize::NONE;
	}

	std::map<GpioDevices, PinConnections> BackupDatabase::GetGpioDevices(std::string_view db_path, std::string rom_name) {
		if (!std::filesystem::exists(db_path) || !std::filesystem::is_regular_file(db_path))
		{
			return {};
		}

		mINI::INIFile db_file(db_path.data());

		mINI::INIStructure database;

		db_file.read(database);

		if (!database.has(rom_name) || !database.get(rom_name).has("gpio"))
			return {};

		auto const& rom_data = database.get(rom_name);

		std::string gpios = rom_data
			.get("gpio");

		gpios.erase(gpios.find_first_of('\"'), 1);
		gpios.erase(gpios.find_last_of('\"'), 1);

		std::istringstream stream{ gpios };
		std::string dev = "";

		std::map<GpioDevices, PinConnections> devs{};

		while (std::getline(stream, dev, ';')) {
			if(dev == "RTC") {
				std::string connections = rom_data.get("rtc");

				PinConnections conns{};

				for (uint8_t pos = 0; pos < 4; pos++) {
					char pin = connections.at(pos);

					conns.conns[pos] = pin == '4' ? -1 : (pin - '0');
				}

				devs.insert({ GpioDevices::RTC, conns });
			}
		}

		return devs;
	}

	std::unordered_map<std::string, CheatEntry> BackupDatabase::GetCheats(std::string_view db_path,
		std::string rom_name) {
		std::unordered_map<std::string, CheatEntry> game_cheats{};

		if (!std::filesystem::exists(db_path) || !std::filesystem::is_regular_file(db_path))
		{
			return game_cheats;
		}

		mINI::INIFile db_file(db_path.data());
		mINI::INIStructure database;
		db_file.read(database);

		if (!database.has(rom_name)) {
			return game_cheats;
		}

		uint32_t remaining_lines{ 0 };
		std::string				 curr_cheat_name{};
		std::vector<std::string> curr_cheat_lines{};
		cheats::CheatType        curr_cheat_type{};

		for (auto const& entry : database[rom_name]) {
			bool is_cheat_complete = false;

			if (remaining_lines > 0) {
				auto& cheat = entry.second;
				curr_cheat_lines.push_back(cheat);
				is_cheat_complete = (--remaining_lines) == 0;
			}
			else {
				auto& name = entry.first;
				auto& cheat = entry.second;

				auto visible_name_end = cheat.find_first_of(':');
				auto count_end = cheat.find_first_of(':', visible_name_end + 1);
				auto type_end = cheat.find_first_of(':', count_end + 1);

				if (visible_name_end == std::string::npos ||
					count_end == std::string::npos)
					continue;

				auto visible_name = cheat.substr(0, visible_name_end);
				auto count_str = cheat.substr(visible_name_end + 1, count_end - (visible_name_end + 1));
				
				if (count_str == "0" && type_end == std::string::npos)
					continue;
				
				auto type_str = (type_end == std::string::npos) ?
					cheat.substr(count_end + 1) :
					cheat.substr(count_end + 1, type_end - (count_end + 1));

				if (type_str == "AR") {
					curr_cheat_type = cheats::CheatType::ACTION_REPLAY;
				}
				else if (type_str == "GS") {
					curr_cheat_type = cheats::CheatType::GAMESHARK;
				} 
				else if (type_str == "CB") {
					curr_cheat_type = cheats::CheatType::CODE_BREAKER;
				}
				else {
					continue;
				}

				curr_cheat_name = visible_name;

				if (count_str != "0") {
					try {
						remaining_lines = uint32_t(std::stoi(count_str));
					}
					catch (std::exception&) {
						continue;
					}
				}
				else {
					auto lines = cheat.substr(type_end + 1);

					std::istringstream parser{ lines };
					std::string curr_line{};

					while (std::getline(parser, curr_line, ',')) {
						curr_cheat_lines.push_back(std::move(curr_line));
					}

					is_cheat_complete = true;
				}
			}

			if (is_cheat_complete) {
				CheatEntry entry{
					.ty = curr_cheat_type,
					.lines = curr_cheat_lines
				};

				game_cheats[curr_cheat_name] = entry;

				curr_cheat_lines.clear();
			}
		}

		return game_cheats;
	}
}