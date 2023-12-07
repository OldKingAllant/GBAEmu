#pragma once

#include "../../common/Defs.hpp"
#include "Backup.hpp"

namespace GBA::gamepack::backups {
	using namespace common;

	class SRAM : public Backup {
	public:
		SRAM(std::streamsize rom_sz);

		bool Load(std::filesystem::path const& from) override;
		bool Store(std::filesystem::path const& to) override;

		u32 Read(u32 address) override;
		void Write(u32 address, u32 value) override;

		~SRAM();

	private:
		u8* m_data;
	};
}