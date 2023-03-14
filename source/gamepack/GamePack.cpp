#include "../../gamepack/GamePack.hpp"

namespace GBA::gamepack {

	GamePack::GamePack() :
		m_rom(nullptr), m_backup(nullptr),
		m_path(), m_info{} {}

	bool GamePack::LoadFrom(fs::path const& path) {
		if (!fs::exists(path))
			return false;

		if (!fs::is_regular_file(path))
			return false;

		m_backup = nullptr; /*Not implemented*/

		m_path = path;

		if (!MapFile())
			return false;

		m_rom = reinterpret_cast<u16*>(m_info.map_address);

		std::copy_n(m_rom, sizeof(GamePackHeader) / 2,
			reinterpret_cast<u16*>(&m_head));

		return true;
	}

	bool GamePack::LoadBackup(fs::path const& from) {
		return true;
	}

	backups::BackupType GamePack::BackupType() const {
		return {};
	}

	u16 GamePack::Read(u32 address) const {
		return 0xFFFF;
	}

	void GamePack::Write(u32 address, u16 value) {}

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

	GamePack::~GamePack() {
		UnMapFile();
	}
}