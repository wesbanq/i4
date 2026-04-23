#include "StackFile.h"
#include "IRunner.h"
#include <iostream>

StackFile::StackFile(const IRunner& fs, std::filesystem::path filename)
    : Fs(fs), Filename(std::move(filename)) { }

RunnerOpenStream StackFile::GetFile() const {
    return Fs.open(Filename, std::ios::in | std::ios::out | std::ios::binary | std::ios::ate | std::ios::app);
}

unsigned int StackFile::SkipSeparator(RunnerOpenStream& file) {
    unsigned int length = 0;
    while (file->tellg() > 0 && StackWord::IsSeparator(file->peek())) {
        file->seekg(-1, std::ios::cur);
        ++length;
    }
    return length;
}

std::pair<StackWord, unsigned int> StackFile::PeekWord() const {
    auto file = GetFile();
    if (!file || !file->good())
        return {{ "", false }, 0};

    const auto size = Size();
    if (size <= 0)
        return {{ "", false }, 0};
    unsigned int length = SkipSeparator(file);

    file->seekg(-1, std::ios::end);
    std::string word;
    bool escaping = false;
    bool quoted = false;
    while (
        (quoted ? file->peek() != StackWord::Quote : !StackWord::IsSeparator(file->peek())) 
        || escaping) {
        if (word.length() == 0 && !quoted && file->peek() == StackWord::Quote) {
            quoted = true;
            file->seekg(-1, std::ios::cur);
            continue;
        }

        if (file->peek() == StackWord::EscapeChar)
            escaping = !escaping;

        word.push_back(file->peek());
        if (file->tellg() == 0) break;
        file->seekg(-1, std::ios::cur);
    }

    length += word.length();
    StackWord result(word, false);

    if (quoted && file->peek() == StackWord::Quote) {
        result.Literal = true;
        length += 2;
        file->seekg(-1, std::ios::cur);
    }

    length += SkipSeparator(file);

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
        throw std::runtime_error("Failed to open stack file: " + Filename.string());

    if (Size() > 0)
        *file << ' ';
    *file << word.Format();
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

StackFile StackFile::Find(const IRunner& fs, 
                          const std::filesystem::path& base, 
                          const std::string_view name,
                          const std::string_view ext) {
    std::filesystem::path p(base);
    p.replace_extension(std::string(ext) + std::string(name));
    return StackFile(fs, p);
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
    *out << ' ' << in->rdbuf();
    return *this;
}

StackFile& StackFile::operator>>(StackWord& word) {
    word = PopWord();
    return *this;
}