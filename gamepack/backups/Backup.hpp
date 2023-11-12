#pragma once

#include "../../common/Defs.hpp"
#include <filesystem>

namespace GBA::gamepack::backups {
	using namespace common;

	enum class BackupType {
		SRAM,
		EEPROM,
		FLASH,
		DACS
	};

	class Backup {
	public :
		Backup(std::streamsize rom_sz) 
			: m_type{}, m_rom_size{ rom_sz }, m_bus_mask{},
			m_start_address{} {}

		BackupType GetBackupType() {
			return m_type;
		}

		virtual bool Load(std::filesystem::path const& from) = 0;
		virtual bool Store(std::filesystem::path const& to) = 0;

		virtual u32 Read(u32 address) = 0;
		virtual void Write(u32 address, u32 value) = 0;
		
		u32 GetStartAddress() {
			return m_start_address;
		}

		u32 GetBusMask() {
			return m_bus_mask;
		}

		virtual ~Backup() {}

		//Other things
	protected :
		BackupType m_type;
		std::streamsize m_rom_size;
		u32 m_bus_mask;
		u32 m_start_address;
	};
}