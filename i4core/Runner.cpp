#include "Runner.h"
#include "Interpreter.h"
#include <filesystem>

std::string Runner::Start(std::filesystem::path mainFile, 
                          unsigned char options, 
                          std::ostream& output,
                          const std::vector<std::string>& programArgs) const {
    std::filesystem::path runPath = std::move(mainFile);
    if (Interpreter::HasOption(options, Interpreter::Args::BOX)) {
        const std::filesystem::path parent = runPath.parent_path();
        const std::filesystem::path boxDir = parent / runPath.stem();
        std::filesystem::create_directories(boxDir);
        const std::filesystem::path boxedMain = boxDir / runPath.filename();
        std::filesystem::copy_file(runPath, boxedMain, std::filesystem::copy_options::overwrite_existing);
        runPath = std::move(boxedMain);
    }

    Interpreter interpreter(*this, std::move(runPath), output, options);
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