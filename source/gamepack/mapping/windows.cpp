#include "../../../gamepack/mapping/windows.hpp"
#include <Windows.h>

namespace GBA::gamepack::mapping {
	std::pair<FileMapInfo, bool> MapFile_Windows(const char* fname) {
		FileMapInfo info{};

		info.file_handle = CreateFileA(
			fname, GENERIC_READ, 0, NULL,
			OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,
			NULL
		);

		LARGE_INTEGER sz;

		GetFileSizeEx(info.file_handle,
			&sz);

		info.file_size = *reinterpret_cast<std::size_t*>(&sz);

		if (!info.file_handle)
			return { info, false };

		info.file_mapping_obj = CreateFileMappingA(
			info.file_handle, NULL,
			PAGE_READONLY, 0, 0, NULL
		);

		if (!info.file_mapping_obj)
			return { info, false };

		info.map_address = MapViewOfFile(
			info.file_mapping_obj,
			FILE_MAP_READ, 0, 0,
			0
		);

		if (!info.map_address)
			return { info, false };

		return { info, true };
	}

	bool UnmapFile_Windows(FileMapInfo const& info) {
		if (info.map_address) {
			if (!UnmapViewOfFile(info.map_address))
				return false;
		}

		if (info.file_mapping_obj && info.file_handle) {
			if (!CloseHandle(info.file_mapping_obj) ||
				!CloseHandle(info.file_handle))
				return false;
		}

		return true;
	}
}