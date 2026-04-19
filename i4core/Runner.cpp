#include "Runner.h"
#include "Interpreter.h"
#include <filesystem>

std::string Runner::Start(std::filesystem::path mainFile, unsigned char options, std::ostream& output,
                          const std::vector<std::string>& programArgs) const {
    Interpreter interpreter(*this, std::move(mainFile), output, options);
    return interpreter.Run(programArgs);
}

bool Runner::exists(const std::filesystem::path& path) const {
    return std::filesystem::exists(path);
}

void Runner::resize_file(const std::filesystem::path& path, std::uintmax_t size) const {
    std::filesystem::resize_file(path, size);
}

std::fstream Runner::open(const std::filesystem::path& path, std::ios::openmode mode) const {
    return std::fstream(path, mode);
}

std::uintmax_t Runner::file_size(const std::filesystem::path& path) const {
    return std::filesystem::file_size(path);
}