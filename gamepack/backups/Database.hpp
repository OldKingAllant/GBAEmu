#pragma once

#include <string>

namespace GBA::gamepack::backups {
	enum class BackupTypeSize {
		NONE,
		EEPROM_8K,
		EEPROM_512,
		FLASH_512,
		FLASH_1M
	};

	class BackupDatabase {
	public :
		static BackupTypeSize GetBackupTypeAndSize(std::string_view db_path, std::string rom_name);
	};
}