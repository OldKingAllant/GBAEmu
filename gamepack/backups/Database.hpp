#pragma once

#include <string>
#include <map>
#include <unordered_map>
#include <vector>

#include "../../emu/Cheats.hpp"

namespace GBA::gamepack::backups {
	enum class BackupTypeSize {
		NONE,
		EEPROM_8K,
		EEPROM_512,
		FLASH_512,
		FLASH_1M,
		SRAM
	};

	enum class GpioDevices : uint16_t {
		NONE,
		RTC
	};

	struct PinConnections {
		int8_t conns[4];
	};

	struct CheatEntry {
		cheats::CheatType ty = {};
		std::vector<std::string> lines = {};
	};

	class BackupDatabase {
	public :
		static BackupTypeSize GetBackupTypeAndSize(std::string_view db_path, std::string rom_name);
		static std::map<GpioDevices, PinConnections> GetGpioDevices(std::string_view db_path, std::string rom_name);
		static std::unordered_map<std::string, CheatEntry> GetCheats(std::string_view db_path, std::string rom_name);
	};
}