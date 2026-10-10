#include "DebugRunner.h"
#include "StackFile.h"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <map>
#include <set>
#include <sstream>
#include <string_view>
#include <system_error>
#include <utility>

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

/// Files are keyed by absolute, normalized path, so a file matches however it's spelled.
std::filesystem::path Key(const std::filesystem::path& path) {
    std::filesystem::path key = std::filesystem::absolute(path).lexically_normal();
    if (!key.has_filename() && key.has_relative_path())
        key = key.parent_path();
    return key;
}

/// Whether path is dir itself or inside it.
bool IsWithin(const std::filesystem::path& path, const std::filesystem::path& dir) {
    return std::mismatch(dir.begin(), dir.end(), path.begin(), path.end()).first == dir.end();
}

/// Program, stack, definition and label files. Definition and label names can contain dots,
/// so those are matched anywhere in the name rather than by extension.
bool IsI4File(const std::filesystem::path& path) {
    const std::string extension = path.extension().string(), name = path.filename().string();
    return extension == StackFile::CodeExtension || extension == StackFile::StackExtension
        || name.find(StackFile::DefExtension) != std::string::npos || name.find(StackFile::LabelExtension) != std::string::npos;
}

} // namespace

void DebugRunner::Preload(const std::filesystem::path& dir) {
    std::error_code ec;
    for (const auto& entry : std::filesystem::directory_iterator(dir, std::filesystem::directory_options::skip_permission_denied, ec))
        if (entry.is_regular_file(ec) && IsI4File(entry.path()))
            Load(Key(entry.path()));
}

bool DebugRunner::IsHidden(const std::filesystem::path& key) const {
    return std::any_of(Removed.begin(), Removed.end(),
                       [&](const std::filesystem::path& dir) { return IsWithin(key, dir); });
}

bool DebugRunner::IsOnDisk(const std::filesystem::path& key) const {
    std::error_code ec;
    return !IsHidden(key) && std::filesystem::is_regular_file(key, ec);
}

std::string* DebugRunner::Load(const std::filesystem::path& key) const {
    if (auto it = Files.find(key); it != Files.end())
        return &it->second;
    if (!IsOnDisk(key))
        return nullptr;
    std::ifstream in(key, std::ios::binary);
    return in ? &(Files[key] = std::string(std::istreambuf_iterator<char>(in), {})) : nullptr;
}

bool DebugRunner::exists(const std::filesystem::path& path) const {
    const std::filesystem::path key = Key(path);
    return Files.count(key) > 0 || IsOnDisk(key);
}

void DebugRunner::resize_file(const std::filesystem::path& path, std::uintmax_t size) const {
    std::string* s = Load(Key(path));
    if (!s || size > s->max_size())
        throw std::filesystem::filesystem_error("resize_file", path, std::make_error_code(
            s ? std::errc::file_too_large : std::errc::no_such_file_or_directory));
    s->resize(size, '\0');
}

FileSystemOpenStream DebugRunner::open(const std::filesystem::path& path, std::ios::openmode mode) const {
    const std::filesystem::path key = Key(path);
    Load(key);
    return FileSystemOpenStream(new DebugStream(Files, key, mode), deleteDebugStream);
}

std::uintmax_t DebugRunner::file_size(const std::filesystem::path& path) const {
    const std::filesystem::path key = Key(path);
    if (auto it = Files.find(key); it != Files.end())
        return it->second.size();
    std::error_code ec;
    const std::uintmax_t size = IsOnDisk(key) ? std::filesystem::file_size(key, ec) : 0;
    return ec ? 0 : size;
}

void DebugRunner::create_directories(const std::filesystem::path&) const {}  // Directories are implicit in memory.

void DebugRunner::remove_all(const std::filesystem::path& path) const {
    const std::filesystem::path key = Key(path);
    std::erase_if(Files, [&](const auto& file) { return IsWithin(file.first, key); });
    Removed.push_back(key);
}

std::vector<std::filesystem::path> DebugRunner::files_under(const std::filesystem::path& dir) const {
    const std::filesystem::path key = Key(dir);
    std::set<std::filesystem::path> found;

    std::error_code ec;
    if (std::filesystem::is_directory(key, ec))
        for (const auto& entry : std::filesystem::recursive_directory_iterator(key, std::filesystem::directory_options::skip_permission_denied))
            if (entry.is_regular_file() && !IsHidden(entry.path()))
                found.insert(entry.path());
    for (const auto& [path, content] : Files)
        if (path != key && IsWithin(path, key))
            found.insert(path);
    return { found.begin(), found.end() };
}
