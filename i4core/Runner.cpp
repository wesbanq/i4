#include "Runner.h"
#include <fstream>
#include <filesystem>
#include <iostream>

namespace {

void deleteFstreamAsIostream(std::iostream* p) {
    delete static_cast<std::fstream*>(p);
}

} // namespace

bool Runner::exists(const std::filesystem::path& path) const {
    return std::filesystem::exists(path);
}

void Runner::resize_file(const std::filesystem::path& path, std::uintmax_t size) const {
    std::filesystem::resize_file(path, size);
}

FileSystemOpenStream Runner::open(const std::filesystem::path& path, std::ios::openmode mode) const {
    return FileSystemOpenStream(new std::fstream(path, mode), deleteFstreamAsIostream);
}

std::uintmax_t Runner::file_size(const std::filesystem::path& path) const {
    return std::filesystem::file_size(path);
}

void Runner::create_directories(const std::filesystem::path& path) const {
    std::filesystem::create_directories(path);
}

void Runner::remove_all(const std::filesystem::path& path) const {
    std::filesystem::remove_all(path);
}

std::vector<std::filesystem::path> Runner::files_under(const std::filesystem::path& dir) const {
    std::vector<std::filesystem::path> files;
    for (const std::filesystem::directory_entry& entry : std::filesystem::recursive_directory_iterator(
             dir, std::filesystem::directory_options::skip_permission_denied)) {
        if (entry.is_regular_file())
            files.push_back(entry.path());
    }
    return files;
}