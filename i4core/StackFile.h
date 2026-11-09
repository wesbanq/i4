#pragma once

#include "StackWord.h"
#include "IFileSystem.h"
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string_view>
#include <utility>

class StackFile {
public:
    /// The data stack never holds code, so a bare word that wouldn't read back as
    /// the same single word is quoted there. Code keeps bare words as written.
    enum class Kind { Code, Data };

private:
    const IFileSystem& Fs;
    std::filesystem::path Filename;
    Kind StackKind;

    FileSystemOpenStream GetFile() const;
public:
    static constexpr std::string_view CodeExtension = ".i4";
    static constexpr std::string_view StackExtension = ".i4s";
    static constexpr std::string_view DefExtension = ".i4d_";
    static constexpr std::string_view LabelExtension = ".i4l_";
    static constexpr std::size_t MaxFileNameLength = 255;

    StackFile(const IFileSystem& fs, std::filesystem::path filename, Kind kind = Kind::Code);

    StackWord PopWord();
    std::pair<StackWord, unsigned int> PeekWord() const;
    void PushWord(const StackWord& word);
    void PushEmpty();

    /// Empties the file, creating it if needed.
    void Clear();
    void Halt();
    std::uintmax_t Size() const;
    std::filesystem::path GetPath() const;

    /// The file of the definition or label called name, next to base. Bytes that aren't safe in
    /// a file name everywhere are written as %XX, so no name can point outside base's directory.
    static StackFile Find(const IFileSystem& fs, 
                          const std::filesystem::path& base, 
                          const std::string_view name,
                          const std::string_view ext);
    /// Whether the file name is short enough to exist. A definition or label whose name makes
    /// it too long can't be created, so it's treated the same on disk and in memory.
    bool HasUsableName() const;

    StackFile& operator<<(const StackWord& word);
    StackFile& operator<<(const StackFile& file);
    StackFile& operator>>(StackWord& word);
    // StackFile& operator>>(StackFile& file);
};
