#pragma once

#include "IRunner.h"
#include <cstdint>
#include <filesystem>
#include <ios>
#include <map>
#include <string>
#include <vector>

/// Runs programs in memory. A file is read from disk the first time it's used,
/// and every change stays in memory, so nothing is ever written to disk.
class DebugRunner final : public IRunner {
public:
    /// Reads every i4 file (program, stack, definition and label files) directly in dir
    /// right away, so the run sees them as they were at startup.
    void Preload(const std::filesystem::path& dir);

    bool exists(const std::filesystem::path& path) const override;
    void resize_file(const std::filesystem::path& path, std::uintmax_t size) const override;
    FileSystemOpenStream open(const std::filesystem::path& path, std::ios::openmode mode) const override;
    std::uintmax_t file_size(const std::filesystem::path& path) const override;
    void create_directories(const std::filesystem::path& path) const override;
    void remove_all(const std::filesystem::path& path) const override;
    std::vector<std::filesystem::path> files_under(const std::filesystem::path& dir) const override;

private:
    /// Files read from disk or written so far, keyed by absolute, normalized path.
    mutable std::map<std::filesystem::path, std::string> Files;
    /// Directories removed in memory; the disk files under them stay hidden.
    mutable std::vector<std::filesystem::path> Removed;

    bool IsHidden(const std::filesystem::path& key) const;
    bool IsOnDisk(const std::filesystem::path& key) const;
    /// The file's content, read from disk on first use, or nullptr if there's no such file.
    std::string* Load(const std::filesystem::path& key) const;
};
