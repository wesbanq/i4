#pragma once

#include "IFileSystem.h"
#include "ReturnCode.h"
#include <filesystem>
#include <iosfwd>
#include <string>
#include <string_view>
#include <vector>

/// Runs programs on top of the filesystem operations a subclass provides,
/// so -box and -safe work the same on disk and in memory.
class IRunner : public IFileSystem {
public:
    static constexpr unsigned SafeCodeFilesLimitMb = 16;
    static constexpr std::string_view BoxDirectorySuffix = ".i4box";

    ~IRunner() override = default;

    ReturnCode Start(std::filesystem::path mainFile,
                     unsigned char options,
                     std::ostream& output,
                     const std::vector<std::string>& programArgs) const;

private:
    void EnforceSafeCodeFileBudget(const std::filesystem::path& mainFile) const;
};
