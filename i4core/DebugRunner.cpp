#include "DebugRunner.h"
#include "Interpreter.h"
#include "Option.h"
#include "Runner.h"
#include "StackFile.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <map>
#include <sstream>
#include <stdexcept>
#include <system_error>

namespace {

std::filesystem::path normalizePath(const std::filesystem::path& path) {
    return path.lexically_normal();
}

void deleteDebugStream(std::iostream* p);

class DebugStream final : public std::iostream {
    std::stringbuf buf_;
    std::filesystem::path key_;
    std::map<std::filesystem::path, std::string>& store_;
    const bool writeBack_;

public:
    DebugStream(std::map<std::filesystem::path, 
                std::string>& store, 
                std::filesystem::path key,
                std::ios::openmode mode)
        : std::iostream(&buf_),
          key_(normalizePath(std::move(key))),
          store_(store),
          writeBack_([&] {
              const bool wantIn = (mode & std::ios::in) != 0;
              const bool wantOut = (mode & std::ios::out) != 0;
              return !(wantIn && !wantOut);
          }()) {
        const bool wantIn = (mode & std::ios::in) != 0;
        const bool wantOut = (mode & std::ios::out) != 0;
        const bool app = (mode & std::ios::app) != 0;

        auto it = store_.find(key_);
        const bool exists = it != store_.end();

        if (wantIn && !wantOut && !exists) {
            setstate(std::ios::failbit);
            return;
        }

        std::string initial;
        if (mode & std::ios::trunc) {
            initial.clear();
        } else if (app && exists) {
            initial = it->second;
        } else if (exists && wantIn) {
            initial = it->second;
        } else if (!exists && wantOut) {
            initial.clear();
        }

        buf_.str(initial);

        if ((mode & std::ios::ate) != 0)
            buf_.pubseekoff(0, std::ios::end, std::ios::in | std::ios::out);
        if (app)
            buf_.pubseekoff(0, std::ios::end, std::ios::out);
    }

    ~DebugStream() {
        if (writeBack_)
            store_[key_] = buf_.str();
    }
};

void deleteDebugStream(std::iostream* p) {
    delete static_cast<DebugStream*>(p);
}

void removeVirtualFilesUnder(std::map<std::filesystem::path, std::string>& files,
                             const std::filesystem::path& boxDir) {
    const auto boxNorm = normalizePath(boxDir);
    const std::string boxStr = boxNorm.generic_string();
    const std::string prefix = boxStr.empty() ? boxStr : boxStr + '/';

    for (auto it = files.begin(); it != files.end();) {
        const std::string k = normalizePath(it->first).generic_string();
        if (k == boxStr || (k.size() > prefix.size() && k.compare(0, prefix.size(), prefix) == 0))
            it = files.erase(it);
        else
            ++it;
    }
}

} // namespace

DebugRunner::DebugRunner(std::map<std::filesystem::path, std::string> initialFiles) {
    for (auto& [k, v] : initialFiles)
        files[normalizePath(k)] = std::move(v);
}

DebugRunner DebugRunner::fromFilesystemAround(const std::filesystem::path& mainFile) {
    const std::filesystem::path cwd = std::filesystem::current_path();
    const std::filesystem::path mainAbs = std::filesystem::weakly_canonical(mainFile);
    std::filesystem::path dir = mainAbs.parent_path();
    if (dir.empty())
        dir = cwd;

    std::map<std::filesystem::path, std::string> m;
    for (const std::filesystem::directory_entry& entry :
         std::filesystem::recursive_directory_iterator(
             dir, std::filesystem::directory_options::skip_permission_denied)) {
        if (!entry.is_regular_file())
            continue;
        const std::filesystem::path& p = entry.path();
        std::ifstream in(p, std::ios::binary);
        if (!in)
            continue;
        std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
        std::error_code relEc;
        std::filesystem::path rel = std::filesystem::relative(p, cwd, relEc);
        if (relEc)
            rel = p.lexically_normal();
        m[normalizePath(rel)] = std::move(content);
    }
    return DebugRunner(std::move(m));
}

bool DebugRunner::exists(const std::filesystem::path& path) const {
    return files.find(normalizePath(path)) != files.end();
}

void DebugRunner::resize_file(const std::filesystem::path& path, std::uintmax_t size) const {
    const std::filesystem::path key = normalizePath(path);
    auto it = files.find(key);
    if (it == files.end()) {
        throw std::filesystem::filesystem_error(
            "resize_file",
            path,
            std::make_error_code(std::errc::no_such_file_or_directory));
    }
    std::string& s = it->second;
    if (size > s.max_size())
        throw std::filesystem::filesystem_error(
            "resize_file",
            path,
            std::make_error_code(std::errc::file_too_large));
    const auto newSize = static_cast<std::size_t>(size);
    if (newSize < s.size())
        s.resize(newSize);
    else if (newSize > s.size())
        s.append(newSize - s.size(), '\0');
}

RunnerOpenStream DebugRunner::open(const std::filesystem::path& path, std::ios::openmode mode) const {
    return RunnerOpenStream(new DebugStream(files, path, mode), deleteDebugStream);
}

std::uintmax_t DebugRunner::file_size(const std::filesystem::path& path) const {
    const std::filesystem::path key = normalizePath(path);
    auto it = files.find(key);
    if (it == files.end()) {
        return 0;
        //throw std::filesystem::filesystem_error(
        //    "file_size",
        //    path,
        //    std::make_error_code(std::errc::no_such_file_or_directory));
    }
    return it->second.size();
}

void DebugRunner::enforceSafeCodeFileBudget(const std::filesystem::path& mainFile) const {
    const std::uintmax_t limitBytes =
        static_cast<std::uintmax_t>(SafeCodeFilesLimitMb) * 1024 * 1024;
    std::uintmax_t total = 0;

    const std::filesystem::path dirNorm = normalizePath(mainFile.parent_path());
    const std::string dirKey = dirNorm.generic_string();
    const bool restrictToDir = !dirKey.empty();

    for (const auto& [key, content] : files) {
        if (restrictToDir) {
            const std::string k = key.generic_string();
            const std::string prefix = dirKey.back() == '/' ? dirKey : dirKey + '/';
            if (k.rfind(prefix, 0) != 0)
                continue;
        } else {
            if (key.has_parent_path())
                continue;
        }
        const std::string ext = key.extension().string();
        if (ext.find(StackFile::CodeExtension) == std::string::npos)
            continue;
        total += content.size();
        if (total > limitBytes) {
            throw std::runtime_error(
                "SAFE: total size of files with \"" + std::string(StackFile::CodeExtension) +
                "\" in the extension exceeds " + std::to_string(SafeCodeFilesLimitMb) + " MiB");
        }
    }
}

std::string DebugRunner::Start(std::filesystem::path mainFile,
                               unsigned char options,
                               std::ostream& output,
                               const std::vector<std::string>& programArgs) const {
    std::filesystem::path runPath = std::move(mainFile);
    if (Interpreter::HasOption(options, Option::BOX)) {
        const std::filesystem::path parent = runPath.parent_path();
        const std::filesystem::path boxDir =
            parent / (runPath.stem().string() + std::string(Runner::BoxDirectorySuffix));
        removeVirtualFilesUnder(files, boxDir);
        const std::filesystem::path boxedMain = boxDir / runPath.filename();
        const std::filesystem::path fromKey = normalizePath(runPath);
        auto it = files.find(fromKey);
        if (it == files.end())
            throw std::runtime_error("BOX: source file not in virtual filesystem: " + fromKey.string());
        files[normalizePath(boxedMain)] = it->second;
        runPath = std::move(boxedMain);
    }

    Interpreter interpreter(*this, std::move(runPath), output, options);
    if ((options & static_cast<unsigned char>(Option::DONTRUN)) > 0) {
        interpreter.PushProgramArgs(programArgs);
        while (!interpreter.Finished()) {
            if (Interpreter::HasOption(options, Option::LIMIT))
                enforceSafeCodeFileBudget(runPath);

            if (Interpreter::HasOption(options, Option::STEP)) {
                std::string line;
                std::printf("*");
                std::getline(std::cin, line);
            }

            interpreter.Step();
        }
        return interpreter.PopFinalResult();
    }
    return interpreter.Run(programArgs);
}
