#include "StackFile.h"

#include <fstream>
#include <filesystem>

StackFile::StackFile(std::filesystem::path filename) : Filename(std::move(filename)) { }

std::fstream StackFile::GetFile() const {
    if (!std::filesystem::exists(Filename)) {
        std::filesystem::create_directories(Filename.parent_path());
    }

    return std::fstream(Filename, std::ios::in | std::ios::out | std::ios::binary | std::ios::ate);
}

std::pair<StackWord, unsigned int> StackFile::PopWordNonDestructive() const {
    auto file = GetFile();
    if (!file.good()) {
        return {{ "", false }, 0};
    }

    const auto size = Size();
    if (size <= 0) {
        return {{ "", false }, 0};
    }

    file.seekg(-1, std::ios::end);
    std::string word;
    while (file.peek() != TokenSeparator) {
        word.push_back(file.peek());
        if (file.tellg() == 0)
            break;
        file.seekg(-1, std::ios::cur);
    }
    if (file.tellg() == 0 && file.peek() != TokenSeparator)
        word.push_back(file.peek());

    while (file.peek() == TokenSeparator && file.tellg() > 0) {
        file.seekg(-1, std::ios::cur);
    }

    StackWord result(word, false);
    if (!word.empty() && word.back() == '"' && word.front() == '"') {
        result.Literal = true;
        result.Word.pop_back();
        result.Word.erase(result.Word.begin());
    }

    unsigned int length = word.length();
    do {
        file.seekg(-1, std::ios::cur);
        ++length;
    } while (file.tellg() > 0 && file.peek() == TokenSeparator);

    return { result, length };
}

StackWord StackFile::PopWord() {
    auto [word, length] = PopWordNonDestructive();
    std::filesystem::resize_file(Filename, Size() - length);
    return word;
}

void StackFile::PushWord(const StackWord& word) {
    auto file = GetFile();
    if (!file.good())
        return;

    if (word.Literal)
        file << TokenSeparator << '"' << word.Word << '"';
    else
        file << TokenSeparator << word.Word;
}

void StackFile::Halt() {
    std::filesystem::resize_file(Filename, 0);
}

std::uintmax_t StackFile::Size() const {
    return std::filesystem::file_size(Filename);
}

StackFile StackFile::FindDef(std::filesystem::path code, std::string name) {
    return StackFile(
        code.replace_extension(DefExtension)
        / name
    );
}

StackFile StackFile::FindLabel(std::filesystem::path code, std::string name) {
    return StackFile(
        code.replace_extension(LabelExtension)
        / name
    );
}

StackFile& StackFile::operator<<(const StackWord& word) {
    PushWord(word);
    return *this;
}

StackFile& StackFile::operator<<(const StackFile& src) {
    std::ifstream in(src.Filename, std::ios::binary);
    if (!in.good())
        return *this;
    std::ofstream out(Filename, std::ios::binary | std::ios::app);
    if (!out.good())
        return *this;
    out << in.rdbuf();
    return *this;
}

StackFile& StackFile::operator>>(StackWord& word) {
    word = PopWord();
    return *this;
}

// StackFile& StackFile::operator>>(StackFile& dest) {
//     std::ifstream in(Filename, std::ios::binary);
//     if (!in.good())
//         return *this;
//     std::ofstream out(dest.Filename, std::ios::binary | std::ios::app);
//     if (!out.good())
//         return *this;
//     out << in.rdbuf();
//     return *this;
// }