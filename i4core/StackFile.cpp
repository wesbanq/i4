#include "StackFile.h"
#include "IFileSystem.h"
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>

namespace {

/// Whether s[pos] closes a \X\ escape unit when s is read right to left, as the parser reads.
bool EndsEscapeUnit(const std::string& s, std::size_t pos) {
    return pos >= 2 && s[pos] == StackWord::EscapeChar && s[pos - 2] == StackWord::EscapeChar;
}

/// Literal content that reads back as one literal. The parser ends a literal at the first quote
/// outside a \X\ unit, so content with such a quote (raw input, or a slice that cut an escape in
/// half) gets its stray quotes and backslashes encoded, keeping its existing units. Other content
/// is returned unchanged.
std::string SafeLiteralContent(const std::string& s) {
    // Built right to left, then reversed; \X\ units read the same both ways.
    std::string out;
    bool hasStrayQuote = false;
    for (std::size_t i = s.size(); i-- > 0;) {
        if (EndsEscapeUnit(s, i))
            out.append(s, i -= 2, 3);
        else if (s[i] == StackWord::Quote || s[i] == StackWord::EscapeChar) {
            hasStrayQuote |= s[i] == StackWord::Quote;
            out += { StackWord::EscapeChar, s[i], StackWord::EscapeChar };
        } else
            out += s[i];
    }
    std::reverse(out.begin(), out.end());
    return hasStrayQuote ? out : s;
}

/// Whether s reads back as the same single bare word: no separator outside a \X\ unit, and no
/// trailing quote (which would start a literal). An empty word is written as nothing, as before.
bool IsSafeBareWord(const std::string& s) {
    if (!s.empty() && s.back() == StackWord::Quote)
        return false;
    for (std::size_t i = s.size(); i-- > 0;)
        if (EndsEscapeUnit(s, i))
            i -= 2;
        else if (StackWord::IsSeparator(s[i]))
            return false;
    return true;
}

} // namespace

StackFile::StackFile(const IFileSystem& fs, std::filesystem::path filename, Kind kind)
    : Fs(fs), Filename(std::move(filename)), StackKind(kind) { }

FileSystemOpenStream StackFile::GetFile() const {
    return Fs.open(Filename, std::ios::in | std::ios::out | std::ios::binary | std::ios::ate | std::ios::app);
}

std::pair<StackWord, unsigned int> StackFile::PeekWord() const {
    auto file = GetFile();
    const std::intmax_t size = file && file->good() ? Size() : 0;
    if (size <= 0)
        return {};

    // The end of the file is loaded in chunks that double as the scan moves left,
    // so reading a word costs about its own length.
    std::string tail;
    std::intmax_t tailStart = size;
    auto at = [&](std::intmax_t pos) {
        if (pos < tailStart) {
            const std::intmax_t start = std::max<std::intmax_t>(0, std::min(pos, size - std::max<std::intmax_t>(256, 2 * (size - tailStart))));
            std::string chunk(tailStart - start, '\0');
            file->clear();
            file->seekg(start);
            file->read(chunk.data(), chunk.size());
            tail.insert(0, chunk);
            tailStart = start;
        }
        return tail[pos - tailStart];
    };
    auto endsEscapeUnit = [&](std::intmax_t pos) {
        return pos >= 2 && at(pos) == StackWord::EscapeChar && at(pos - 2) == StackWord::EscapeChar;
    };

    // Separators after the word are removed with it.
    std::intmax_t last = size - 1;
    while (last >= 0 && StackWord::IsSeparator(at(last)))
        --last;
    if (last < 0)
        return {{ "", false }, static_cast<unsigned int>(size)};

    // A word ending in a quote is a literal that reaches back to the nearest opening quote.
    // \X\ units are skipped whole, so an escaped quote can't end it. With no opening quote
    // before the start of the file, the quote is an ordinary character.
    std::intmax_t first = -1;
    if (at(last) == StackWord::Quote)
        for (std::intmax_t pos = last - 1; pos >= 0 && first < 0; pos -= endsEscapeUnit(pos) ? 3 : 1)
            if (at(pos) == StackWord::Quote)
                first = pos;
    const bool literal = first >= 0;
    if (!literal) {
        first = last;
        while (first >= 0 && (endsEscapeUnit(first) || !StackWord::IsSeparator(at(first))))
            first -= endsEscapeUnit(first) ? 3 : 1;
        ++first;
    }

    // In memory a word is reversed, the order it was read in; a literal leaves out its quotes.
    std::string word;
    for (std::intmax_t pos = last - literal; pos >= first + literal; --pos)
        word.push_back(at(pos));

    // Separators before the word go with it too, except one at the very start of the file.
    std::intmax_t cut = first;
    while (cut > 1 && StackWord::IsSeparator(at(cut - 1)))
        --cut;

    return { StackWord(std::move(word), literal), static_cast<unsigned int>(size - cut) };
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
    if (word.Literal || (StackKind == Kind::Data && !IsSafeBareWord(word.Word)))
        *file << StackWord::Quote << SafeLiteralContent(word.Word) << StackWord::Quote;
    else
        *file << word.Word;
}

void StackFile::PushEmpty() {
    PushWord({ "", true });
}

void StackFile::Clear() {
    auto file = Fs.open(Filename, std::ios::out | std::ios::binary | std::ios::trunc);
    if (!file || !file->good())
        throw std::runtime_error("Failed to open stack file: " + Filename.string());
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

StackFile StackFile::Find(const IFileSystem& fs, 
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

    if (Size() > 0)
        *out << ' ';
    *out << in->rdbuf();
    return *this;
}

StackFile& StackFile::operator>>(StackWord& word) {
    word = PopWord();
    return *this;
}