#pragma once

#include "IRunner.h"
#include <filesystem>
#include <vector>

class Runner final : public IRunner {
private:
    void enforceSafeCodeFileBudget(const std::filesystem::path& mainFile) const;
public:
    bool exists(const std::filesystem::path& path) const override;
    void resize_file(const std::filesystem::path& path, std::uintmax_t size) const override;
    FileSystemOpenStream open(const std::filesystem::path& path, std::ios::openmode mode) const override;
    std::uintmax_t file_size(const std::filesystem::path& path) const override;

    ReturnCode Start(std::filesystem::path mainFile,
                     unsigned char options,
                     std::ostream& output,
                     const std::vector<std::string>& programArgs) const override;
};
