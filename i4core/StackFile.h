#pragma once

#include "StackWord.h"
#include "IRunner.h"
#include <string_view>

class StackFile {
private:
    const IRunner& Fs;
    std::filesystem::path Filename;

    std::pair<StackWord, unsigned int> PopWordNonDestructive() const;
    std::fstream GetFile() const;
public:
    static constexpr std::string_view CodeExtension = ".i4";
    static constexpr std::string_view StackExtension = ".i4s";
    static constexpr std::string_view DefExtension = ".i4d_";
    static constexpr std::string_view LabelExtension = ".i4l_";

    StackFile(const IRunner& fs, std::filesystem::path filename);
    static constexpr auto Separator = ' ';

    StackWord PopWord();
    void PushWord(const StackWord& word);

    void Halt();
    std::uintmax_t Size() const;

    static StackFile Find(const IRunner& fs, 
                          const std::filesystem::path& base, 
                          const std::string_view name,
                          const std::string_view ext);

    StackFile& operator<<(const StackWord& word);
    StackFile& operator<<(const StackFile& file);
    StackFile& operator>>(StackWord& word);
    // StackFile& operator>>(StackFile& file);
};
