#pragma once

#include "StackWord.h"
#include "IFileSystem.h"
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

    static StackFile Find(const IFileSystem& fs, 
                          const std::filesystem::path& base, 
                          const std::string_view name,
                          const std::string_view ext);

    StackFile& operator<<(const StackWord& word);
    StackFile& operator<<(const StackFile& file);
    StackFile& operator>>(StackWord& word);
    // StackFile& operator>>(StackFile& file);
};
