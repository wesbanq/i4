#pragma once

#include "IRunner.h"
#include <filesystem>
#include <ostream>
#include <string>

class Runner final : public IRunner {
public:
    bool exists(const std::filesystem::path& path) const override;
    void resize_file(const std::filesystem::path& path, std::uintmax_t size) const override;
    std::fstream open(const std::filesystem::path& path, std::ios::openmode mode) const override;
    std::uintmax_t file_size(const std::filesystem::path& path) const override;

    std::string Start(std::filesystem::path mainFile, 
                      unsigned char options, 
                      std::ostream& output,
                      const std::vector<std::string>& programArgs) const;
};
