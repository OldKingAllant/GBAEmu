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
}