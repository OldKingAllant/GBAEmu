#include "../../../gamepack/backups/SRAM.hpp"

#include <fstream>

namespace GBA::gamepack::backups {
	SRAM::SRAM(std::streamsize rom_sz) : Backup(rom_sz), m_data{nullptr} 
	{
		m_type = BackupType::SRAM;
		m_bus_mask = 0xFF;
		m_start_address = 0xE000000;

		m_data = new u8[0x8000];

		std::fill_n(m_data, 0x8000, 0x00);
	}

	bool SRAM::Load(std::filesystem::path const& from) {
		std::streamsize req_size = 0x8000;

		if (!std::filesystem::exists(from) || !std::filesystem::is_regular_file(from))
			return false;

		if (std::filesystem::file_size(from) != req_size)
			return false;

		std::ifstream save_file(from, std::ios::in | std::ios::binary);

		if (!save_file.is_open())
			return false;

		save_file.read(reinterpret_cast<char*>(m_data), req_size);

		return true;
	}

	bool SRAM::Store(std::filesystem::path const& to) {
		std::ofstream save_file(to, std::ios::out);

		if (!save_file.is_open())
			return false;

		save_file.close();

		save_file.open(to, std::ios::out | std::ios::binary);

		save_file.write(reinterpret_cast<char*>(m_data), 0x8000);

		return true;
	}

	u32 SRAM::Read(u32 address) {
		return m_data[address & 0x7FFF];
	}

	void SRAM::Write(u32 address, u32 value) {
		m_data[address & 0x7FFF] = (u8)value;
	}

	SRAM::~SRAM() {
		if (m_data)
			delete[] m_data;
	}
}