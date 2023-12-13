#pragma once

#include <utility>
#include <string>

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
#include "windows.hpp"
#define WINDOWS_MAPPING
#else
#include "linux.hpp"
#define LINUX_MAPPING
#endif

namespace GBA::gamepack::mapping {
	std::pair<FileMapInfo, bool> MapFileToMemory(std::string const& path);
	bool UnmapFile(FileMapInfo const& info);
}


