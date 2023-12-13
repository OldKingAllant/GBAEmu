#include "../../../gamepack/mapping/linux.hpp"

#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/io.h>

#include <fcntl.h>

namespace GBA::gamepack::mapping {
    std::pair<FileMapInfo, bool> MapFile_Linux(const char* fname) {
        auto fd = open(fname, O_RDONLY);

        if(fd == -1) {
            return { {}, false };
        }

        struct stat file_status{};

        auto res = fstat(fd, &file_status);

        if(res == -1)
            return { {}, false };

        FileMapInfo info{};

        info.file_size = file_status.st_size;

        info.map_address = (int8_t*)mmap(nullptr, info.file_size,
        PROT_READ, MAP_PRIVATE, fd, 0);

        if(info.map_address == MAP_FAILED)
            return { {}, false };

        info.fd = fd;

        return { info, true };
    }

    bool UnmapFile_Linux(FileMapInfo const& info) {
        if(munmap(info.map_address, info.file_size) == -1)
            return false;

        return true;
    }
} 
