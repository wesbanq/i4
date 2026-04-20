#include "Runner.h"
#include "Interpreter.h"
#include "Option.h"
#include "StackFile.h"
#include <fstream>
#include <filesystem>
#include <stdexcept>
#include <string>

namespace {

void deleteFstreamAsIostream(std::iostream* p) {
    delete static_cast<std::fstream*>(p);
}

} // namespace

void Runner::enforceSafeCodeFileBudget(const std::filesystem::path& mainFile) const {
    std::filesystem::path dir = mainFile.parent_path();
    if (dir.empty())
        dir = std::filesystem::current_path();

    const std::uintmax_t limitBytes =
        static_cast<std::uintmax_t>(SafeCodeFilesLimitMb) * 1024 * 1024;
    std::uintmax_t total = 0;

    std::filesystem::recursive_directory_iterator it(
        dir, std::filesystem::directory_options::skip_permission_denied);
    for (const std::filesystem::directory_entry& entry : it) {
        if (!entry.is_regular_file())
            continue;
        const std::filesystem::path& p = entry.path();
        const std::string ext = p.extension().string();
        if (ext.find(StackFile::CodeExtension) == std::string::npos)
            continue;
        total += file_size(p);
        if (total > limitBytes) {
            throw std::runtime_error(
                "SAFE: total size of files with \"" + std::string(StackFile::CodeExtension) +
                "\" in the extension exceeds " + std::to_string(SafeCodeFilesLimitMb) + " MiB");
        }
    }
}

std::string Runner::Start(std::filesystem::path mainFile, 
                          unsigned char options, 
                          std::ostream& output,
                          const std::vector<std::string>& programArgs) const {
    std::filesystem::path runPath = std::move(mainFile);
    if (Interpreter::HasOption(options, Option::BOX)) {
        const std::filesystem::path parent = runPath.parent_path();
        const std::filesystem::path boxDir = parent / runPath.stem();
        std::filesystem::create_directories(boxDir);
        const std::filesystem::path boxedMain = boxDir / runPath.filename();
        std::filesystem::copy_file(runPath, boxedMain, std::filesystem::copy_options::overwrite_existing);
        runPath = std::move(boxedMain);
    }

    Interpreter interpreter(*this, std::move(runPath), output, options);
    if (Interpreter::HasOption(options, Option::DONTRUN)) {
        interpreter.PushProgramArgs(programArgs);
        while (!interpreter.Finished()) {
            if (Interpreter::HasOption(options, Option::LIMIT))
                enforceSafeCodeFileBudget(runPath);

            interpreter.Step();
        }
        return interpreter.PopFinalResult();
    }
    else
        return interpreter.Run(programArgs);
}

bool Runner::exists(const std::filesystem::path& path) const {
    return std::filesystem::exists(path);
}

void Runner::resize_file(const std::filesystem::path& path, std::uintmax_t size) const {
    std::filesystem::resize_file(path, size);
}

RunnerOpenStream Runner::open(const std::filesystem::path& path, std::ios::openmode mode) const {
    return RunnerOpenStream(new std::fstream(path, mode), deleteFstreamAsIostream);
}

std::uintmax_t Runner::file_size(const std::filesystem::path& path) const {
    return std::filesystem::file_size(path);
}