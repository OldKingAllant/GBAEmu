#include "../../../gamepack/mapping/FileMapping.hpp"

namespace GBA::gamepack::mapping {
	std::pair<FileMapInfo, bool> MapFileToMemory(std::string const& path) {
#ifdef WINDOWS_MAPPING
		return MapFile_Windows(path.c_str());
#else
		return MapFile_Linux(path.c_str());
#endif // WINDOWS_MAPPING
	}

	bool UnmapFile(FileMapInfo const& info) {
#ifdef WINDOWS_MAPPING
		return UnmapFile_Windows(info);
#else
		return UnmapFile_Linux(info);
#endif // WINDOWS_MAPPING
	}
}