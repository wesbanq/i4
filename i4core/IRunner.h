#pragma once

#include "IFileSystem.h"
#include "ReturnCode.h"
#include <filesystem>
#include <iosfwd>
#include <string_view>
#include <vector>

class IRunner : public IFileSystem {
public:
    static constexpr unsigned SafeCodeFilesLimitMb = 16;
    static constexpr std::string_view BoxDirectorySuffix = ".i4box";

    ~IRunner() override = default;

    virtual ReturnCode Start(std::filesystem::path mainFile,
                             unsigned char options,
                             std::ostream& output,
                             const std::vector<std::string>& programArgs) const = 0;
};
