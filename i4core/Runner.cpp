#include "Runner.h"
#include <fstream>
#include <filesystem>
#include <iostream>
#include <system_error>

namespace {

void deleteFstreamAsIostream(std::iostream* p) {
    delete static_cast<std::fstream*>(p);
}

} // namespace

bool Runner::exists(const std::filesystem::path& path) const {
    // A path that can't be looked up (a name that's too long, say) doesn't exist either.
    std::error_code ec;
    return std::filesystem::exists(path, ec);
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

std::vector<std::filesystem::path> Runner::files_in(const std::filesystem::path& dir) const {
    std::vector<std::filesystem::path> files;
    std::error_code ec;
    for (const auto& entry : std::filesystem::directory_iterator(dir, std::filesystem::directory_options::skip_permission_denied))
        if (entry.is_regular_file(ec))
            files.push_back(entry.path());
    return files;
}