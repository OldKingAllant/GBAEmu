#include "../../../gamepack/mapping/FileMapping.hpp"

namespace GBA::gamepack::mapping {
	std::pair<FileMapInfo, bool> MapFileToMemory(std::string const& path) {
#ifdef WINDOWS_MAPPING
		return MapFile_Windows(path.c_str());
#else
		return {};
#endif // WINDOWS_MAPPING
	}

	bool UnmapFile(FileMapInfo const& info) {
#ifdef WINDOWS_MAPPING
		return UnmapFile_Windows(info);
#else
		return false;
#endif // WINDOWS_MAPPING
	}
}