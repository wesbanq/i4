#include "StackFile.h"
#include "IRunner.h"
#include <iostream>

StackFile::StackFile(const IRunner& fs, std::filesystem::path filename)
    : Fs(fs), Filename(std::move(filename)) { }

RunnerOpenStream StackFile::GetFile() const {
    return Fs.open(Filename, std::ios::in | std::ios::out | std::ios::binary | std::ios::ate);
}

std::pair<StackWord, unsigned int> StackFile::PeekWord() const {
    auto file = GetFile();
    if (!file || !file->good()) {
        return {{ "", false }, 0};
    }

    const auto size = Size();
    if (size <= 0) {
        return {{ "", false }, 0};
    }

    file->seekg(-1, std::ios::end);
    std::string word;
    while (file->peek() != Separator) {
        word.push_back(file->peek());
        if (file->tellg() == 0)
            break;
        file->seekg(-1, std::ios::cur);
    }
    //if (file->tellg() == 0 && file->peek() != Separator)
    //    word.push_back(file->peek());

    //while (file->peek() == Separator && file->tellg() > 0) {
    //    file->seekg(-1, std::ios::cur);
    //}

    StackWord result(word, false);
    if (!word.empty() && word.back() == '"' && word.front() == '"') {
        result.Literal = true;
        result.Word.pop_back();
        result.Word.erase(result.Word.begin());
    }

    unsigned int length = word.length();
    while (file->tellg() > 0 && file->peek() == Separator) {
        file->seekg(-1, std::ios::cur);
        ++length;
    }

    return { result, length };
}

StackWord StackFile::PopWord() {
    auto [word, length] = PeekWord();
    Fs.resize_file(Filename, Size() - length);
    return word;
}

void StackFile::PushWord(const StackWord& word) {
    auto file = GetFile();
    if (!file || !file->good())
        return;

    if (word.Literal)
        *file << Separator << '"' << word.Word << '"';
    else
        *file << Separator << word.Word;
}

void StackFile::Halt() {
    Fs.resize_file(Filename, 0);
}

std::uintmax_t StackFile::Size() const {
    return Fs.file_size(Filename);
}

std::filesystem::path StackFile::GetPath() const {
    return Filename;
}

StackFile StackFile::Find(const IRunner& fs, const std::filesystem::path& base, const std::string_view name,
                          const std::string_view ext) {
    std::filesystem::path p(base);
    p.replace_extension(ext);
    return StackFile(fs, p / name);
}

StackFile& StackFile::operator<<(const StackWord& word) {
    PushWord(word);
    return *this;
}

StackFile& StackFile::operator<<(const StackFile& src) {
    auto in = src.Fs.open(src.Filename, std::ios::in | std::ios::binary);
    if (!in || !in->good())
        return *this;
    auto out = Fs.open(Filename, std::ios::out | std::ios::binary | std::ios::app);
    if (!out || !out->good())
        return *this;
    *out << in->rdbuf();
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