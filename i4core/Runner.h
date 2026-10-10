#pragma once

#include "IRunner.h"
#include <cstdint>
#include <filesystem>
#include <ios>
#include <vector>

/// Runs programs on the real filesystem.
class Runner final : public IRunner {
public:
    bool exists(const std::filesystem::path& path) const override;
    void resize_file(const std::filesystem::path& path, std::uintmax_t size) const override;
    FileSystemOpenStream open(const std::filesystem::path& path, std::ios::openmode mode) const override;
    std::uintmax_t file_size(const std::filesystem::path& path) const override;
    void create_directories(const std::filesystem::path& path) const override;
    void remove_all(const std::filesystem::path& path) const override;
    std::vector<std::filesystem::path> files_under(const std::filesystem::path& dir) const override;
};
