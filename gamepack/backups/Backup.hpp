#pragma once

#include "../../common/Defs.hpp"

namespace GBA::gamepack::backups {
	enum class BackupType {
		SRAM,
		EEPROM,
		FLASH,
		DACS
	};

	class Backup {
	public :
		Backup();

		virtual BackupType GetBackupType() = 0;
		//Other things
	};
}