#pragma once

#include <cstdint>
#include <filesystem>
#include <iostream>
#include <memory>
#include <vector>

using FileSystemOpenStream = std::unique_ptr<std::iostream, void (*)(std::iostream*)>;

class IFileSystem {
public:
    virtual ~IFileSystem() = default;

    virtual bool exists(const std::filesystem::path& path) const = 0;
    virtual void resize_file(const std::filesystem::path& path, std::uintmax_t size) const = 0;
    virtual FileSystemOpenStream open(const std::filesystem::path& path,
                                  std::ios::openmode mode) const = 0;
    virtual std::uintmax_t file_size(const std::filesystem::path& path) const = 0;
    virtual void create_directories(const std::filesystem::path& path) const = 0;
    virtual void remove_all(const std::filesystem::path& path) const = 0;
    /// Every regular file under dir, including subdirectories.
    virtual std::vector<std::filesystem::path> files_under(const std::filesystem::path& dir) const = 0;
};