#include "../../gamepack/GamePack.hpp"

#include "../../gamepack/backups/EEPROM.hpp"

//#include "../../common/Logger.hpp"

namespace GBA::gamepack {

	//LOG_CONTEXT("Cartridge");

	GamePack::GamePack() :
		m_rom(nullptr), m_backup(nullptr),
		m_path(), m_info{}, m_head{},
		m_backup_address_start{} {}

	bool GamePack::LoadFrom(fs::path const& path) {
		if (!fs::exists(path))
			return false;

		if (!fs::is_regular_file(path))
			return false;

		m_path = path;

		if (!MapFile())
			return false;

		m_rom = reinterpret_cast<u8*>(m_info.map_address);

		std::copy_n(m_rom, sizeof(GamePackHeader),
			reinterpret_cast<u8*>(&m_head));

		m_backup = new backups::EEPROM(m_info.file_size, 0x400);

		m_backup_address_start = m_backup->GetStartAddress();

		(void)m_backup->Load("test.save");

		return true;
	}

	bool GamePack::LoadBackup(fs::path const& from) {
		return true;
	}

	backups::BackupType GamePack::BackupType() const {
		return m_backup->GetBackupType();
	}

	u16 GamePack::Read(u32 address, u8 region) const {
		if (region == 0x5 && address + 0xC000000 >= m_backup_address_start) {
			return (u16)m_backup->Read(address);
		}

		return *reinterpret_cast<u16*>(m_rom + address);
	}

	void GamePack::Write(u32 address, u16 value, u8 region) {
		if (address >= m_backup_address_start)
			m_backup->Write(address - m_backup_address_start, value);
	}

	bool GamePack::MapFile() {
		auto ret = mapping::MapFileToMemory(m_path.generic_string());

		if (!ret.second) {
			UnMapFile();
			return false;
		}
		
		m_info = ret.first;

		return true;
	}

	bool GamePack::UnMapFile() {
		return mapping::UnmapFile(m_info);
	}

	u8 GamePack::ReadSRAM(u32 address) const {
		return 0x00;
	}

	u16 GamePack::DebuggerReadSRAM16(u32 address) const {
		return 0x00;
	}

	u32 GamePack::DebuggerReadSRAM32(u32 address) const {
		return 0x00;
	}

	void GamePack::WriteSRAM(u32 address, u8 value) {
		(void)address;
		(void)value;
	}

	GamePack::~GamePack() {
		UnMapFile();

		if (m_backup) {
			m_backup->Store("test.save");
			delete m_backup;
		}
	}
}