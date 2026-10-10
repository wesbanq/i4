#include "IRunner.h"
#include "Interpreter.h"
#include "Option.h"
#include "StackFile.h"
#include <cstdint>
#include <cstdio>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>

void IRunner::EnforceSafeCodeFileBudget(const std::filesystem::path& mainFile) const {
    std::uintmax_t total = 0;
    for (const auto& p : files_under(mainFile.has_parent_path() ? mainFile.parent_path() : ".")) {
        if (p.extension().string().find(StackFile::CodeExtension) != std::string::npos)
            total += file_size(p);
    }
    if (total > std::uintmax_t{SafeCodeFilesLimitMb} * 1024 * 1024)
        throw std::runtime_error("SAFE: total size of files with \"" + std::string(StackFile::CodeExtension) +
                                 "\" in the extension exceeds " + std::to_string(SafeCodeFilesLimitMb) + " MiB");
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
        StackFile(*this, boxedMain) << StackFile(*this, runPath);
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
