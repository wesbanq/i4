#pragma once

#include "StackWord.h"
#include <filesystem>

class StackFile {
private:
    std::filesystem::path Filename;

    std::pair<StackWord, unsigned int> PopWordNonDestructive() const;
    std::fstream GetFile() const;
public:
    static constexpr std::string_view CodeExtension = ".i4";
    static constexpr std::string_view StackExtension = ".i4s";
    static constexpr std::string_view DefExtension = ".i4d_";
    static constexpr std::string_view LabelExtension = ".i4l_";

    explicit StackFile(std::filesystem::path filename);
    static constexpr auto TokenSeparator = ' ';

    StackWord PopWord();
    void PushWord(const StackWord& word);

    void Halt();
    std::uintmax_t Size() const;

    static StackFile FindDef(std::filesystem::path code, std::string name);
    static StackFile FindLabel(std::filesystem::path code, std::string name);

    StackFile& operator<<(const StackWord& word);
    StackFile& operator<<(const StackFile& file);
    StackFile& operator>>(StackWord& word);
    // StackFile& operator>>(StackFile& file);
};
