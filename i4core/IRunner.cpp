#include "IRunner.h"
#include "Interpreter.h"
#include "Option.h"
#include "StackFile.h"
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>

#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#endif

namespace {

#ifdef _WIN32
constexpr const char* TerminalDevice = "CONIN$";
bool IsTerminal(std::FILE* stream) { return _isatty(_fileno(stream)) != 0; }
#else
constexpr const char* TerminalDevice = "/dev/tty";
bool IsTerminal(std::FILE* stream) { return isatty(fileno(stream)) != 0; }
#endif

/// Where -step waits for Enter. That's stdin, unless stdin is redirected while the prompts
/// show on a terminal: then the terminal is read directly, so stepping doesn't eat the lines
/// meant for the program's ?.
class StepPrompt {
    std::ifstream Terminal;

public:
    StepPrompt() {
        if (!IsTerminal(stdin) && IsTerminal(stdout))
            Terminal.open(TerminalDevice);
    }

    void WaitForEnter() {
        std::istream& in = Terminal.is_open() ? static_cast<std::istream&>(Terminal) : std::cin;
        std::string line;
        std::cout << '*' << std::flush;
        std::getline(in, line);
    }
};

} // namespace

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
        StepPrompt stepPrompt;
        while (!interpreter.Finished()) {
            if (Interpreter::HasOption(options, Option::LIMIT)
                && interpreter.FilesSize() > std::uintmax_t{SafeFilesLimitMb} * 1024 * 1024)
                throw std::runtime_error("SAFE: the program's files exceed " + std::to_string(SafeFilesLimitMb) + " MiB");

            if (Interpreter::HasOption(options, Option::STEP))
                stepPrompt.WaitForEnter();

            interpreter.Step();
        }
        return interpreter.PopResult();
    }
    return interpreter.Run(programArgs);
}
