#pragma once

#include "../common/Defs.hpp"

#include "backups/Backup.hpp"
#include "backups/Database.hpp"
#include "gpio/Gpio.hpp"
#include "mapping/FileMapping.hpp"
#include "Header.hpp"

#include <filesystem>

namespace GBA::gamepack {
	using namespace common;

	namespace fs = std::filesystem;

	class GamePack {
	public :
		GamePack();

		bool LoadFrom(fs::path const& path);
		backups::BackupType BackupType() const;
		bool LoadBackup(fs::path const& from);
		bool StoreBackup(fs::path const& to);

		u16 Read(u32 address, u8 region = 0) const;
		void Write(u32 address, u16 value, u8 region = 0);

		u8 ReadSRAM(u32 address) const;
		u16 DebuggerReadSRAM16(u32 address) const;
		u32 DebuggerReadSRAM32(u32 address) const;

		void WriteSRAM(u32 address, u8 value);

		GamePackHeader const& GetHeader() const {
			return m_head;
		}

		fs::path const& GetRomPath() const {
			return m_path;
		}

		~GamePack();

		template <typename Ar>
		void save(Ar& ar) const {
			if(m_gpio != nullptr)
				ar(*m_gpio);
		}

		template <typename Ar>
		void load(Ar& ar) {
			if (m_gpio != nullptr)
				ar(*m_gpio);
		}

		u16 Patch(u32 address, u16 value);

		std::unordered_map<std::string, backups::CheatEntry> const& GetCheats() const {
			return m_cheats;
		}

	private :
		bool MapFile();
		bool UnMapFile();

		u8* m_rom;
		backups::Backup* m_backup;
		fs::path m_path;
		mapping::FileMapInfo m_info;
		GamePackHeader m_head;
		u32 m_backup_address_start;
		gpio::Gpio* m_gpio;

		std::unordered_map<std::string, backups::CheatEntry> m_cheats;
	};
}