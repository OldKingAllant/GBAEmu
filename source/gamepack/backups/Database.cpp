#include "../../../gamepack/backups/Database.hpp"

#include <filesystem>

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

		return BackupTypeSize::NONE;
	}
}