#pragma once

#include "ReturnCode.h"
#include "IRunner.h"
#include <cstdint>
#include <filesystem>
#include <iosfwd>
#include <map>
#include <string>
#include <vector>

class DebugRunner final : public IRunner {
public:
    /// Keys are lexically normalized paths (see lookups in the implementation).
    mutable std::map<std::filesystem::path, std::string> files;

    explicit DebugRunner(std::map<std::filesystem::path, std::string> initialFiles = {});

    /// Load all regular files under the main file's parent directory (or cwd if none) into the map,
    /// using paths relative to the current directory as keys (matches interpreter path lookups).
    static DebugRunner fromFilesystemAround(const std::filesystem::path& mainFile);

    bool exists(const std::filesystem::path& path) const override;
    void resize_file(const std::filesystem::path& path, std::uintmax_t size) const override;
    RunnerOpenStream open(const std::filesystem::path& path, std::ios::openmode mode) const override;
    std::uintmax_t file_size(const std::filesystem::path& path) const override;

    ReturnCode Start(std::filesystem::path mainFile,
                      unsigned char options,
                      std::ostream& output,
                      const std::vector<std::string>& programArgs) const;

private:
    static constexpr unsigned SafeCodeFilesLimitMb = 16;
    void enforceSafeCodeFileBudget(const std::filesystem::path& mainFile) const;
};
