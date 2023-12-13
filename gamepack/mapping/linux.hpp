#include <stdint.h>
#include <utility>

namespace GBA::gamepack::mapping {
    struct FileMapInfo {
        uint64_t file_size;
        int fd;
        int8_t* map_address;
    };

    std::pair<FileMapInfo, bool> MapFile_Linux(const char* fname);
    bool UnmapFile_Linux(FileMapInfo const& info);
}