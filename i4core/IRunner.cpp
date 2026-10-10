#include "IRunner.h"
#include "Interpreter.h"
#include "Option.h"
#include "StackFile.h"
#include <cstdint>
#include <cstdio>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>

namespace {

void CopyFile(const IFileSystem& fs, const std::filesystem::path& from, const std::filesystem::path& to) {
    auto in = fs.open(from, std::ios::in | std::ios::binary);
    if (!in || !in->good())
        throw std::runtime_error("BOX: failed to read " + from.string());
    auto out = fs.open(to, std::ios::out | std::ios::binary | std::ios::trunc);
    if (!out || !out->good())
        throw std::runtime_error("BOX: failed to write " + to.string());
    *out << in->rdbuf();
}

} // namespace

void IRunner::EnforceSafeCodeFileBudget(const std::filesystem::path& mainFile) const {
    std::filesystem::path dir = mainFile.parent_path();
    if (dir.empty())
        dir = ".";

    const std::uintmax_t limitBytes =
        static_cast<std::uintmax_t>(SafeCodeFilesLimitMb) * 1024 * 1024;
    std::uintmax_t total = 0;

    for (const std::filesystem::path& p : files_under(dir)) {
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

ReturnCode IRunner::Start(std::filesystem::path mainFile,
                          unsigned char options,
                          std::ostream& output,
                          const std::vector<std::string>& programArgs) const {
    std::filesystem::path runPath = std::move(mainFile);
    if (Interpreter::HasOption(options, Option::BOX)) {
        const std::filesystem::path parent = runPath.parent_path();
        const std::filesystem::path boxDir =
            parent / (runPath.stem().string() + std::string(BoxDirectorySuffix));
        remove_all(boxDir);
        create_directories(boxDir);
        const std::filesystem::path boxedMain = boxDir / runPath.filename();
        CopyFile(*this, runPath, boxedMain);
        runPath = boxedMain;
    }

    Interpreter interpreter(*this, runPath, output, options);
    if ((options & static_cast<unsigned char>(Option::DONTRUN)) > 0) {
        interpreter.PushArgs(programArgs);
        while (!interpreter.Finished()) {
            if (Interpreter::HasOption(options, Option::LIMIT))
                EnforceSafeCodeFileBudget(runPath);

            if (Interpreter::HasOption(options, Option::STEP)) {
                std::string line;
                std::printf("*");
                std::getline(std::cin, line);
            }

            interpreter.Step();
        }
        return interpreter.PopResult();
    }
    return interpreter.Run(programArgs);
}
