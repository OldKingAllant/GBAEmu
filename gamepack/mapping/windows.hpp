#pragma once

#include "../../common/Defs.hpp"

#include <utility>

namespace GBA::gamepack::mapping {
	using namespace common;

	struct FileMapInfo {
		void* file_handle;
		void* file_mapping_obj;
		std::size_t file_size;
		void* map_address;
	};

	std::pair<FileMapInfo, bool> MapFile_Windows(const char* fname);

	bool UnmapFile_Windows(FileMapInfo const& info);
}