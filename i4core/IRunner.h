#pragma once

#include <filesystem>
#include <fstream>

class IRunner {
public:
    virtual ~IRunner() = default;

    virtual bool exists(const std::filesystem::path& path) const = 0;
    virtual void resize_file(const std::filesystem::path& path, std::uintmax_t size) const = 0;
    virtual std::fstream open(const std::filesystem::path& path, std::ios::openmode mode) const = 0;
    virtual std::uintmax_t file_size(const std::filesystem::path& path) const = 0;
};