#include "StackWord.h"

bool StackWord::IsSeparator(char c) {
    switch (c) {
        case Separator:
            return true;
        case '\n':
            return true;
        case '\r':
            return true;
        case '\t':
            return true;
        default:
            return false;
    }
}

StackWord::StackWord() : Word(), Literal(false) { }

StackWord::StackWord(std::string word, bool literal) : Word(std::move(word)), Literal(literal) { }

std::string StackWord::Format() const {
    return Literal ? "\"" + Word + "\"" : Word;
}

std::string StackWord::Escape() const {
    std::string out;
    out.reserve(Word.size());
    for (std::size_t i = 0; i < Word.size(); ++i) {
        if (Word[i] != EscapeChar || Word[i+2] != EscapeChar || i + 1 >= Word.size()) {
            out.push_back(Word[i]);
            continue;
        }
        switch (Word[++i]) {
        case 'n':
            out.push_back('\n');
            break;
        case 'r':
            out.push_back('\r');
            break;
        case 't':
            out.push_back('\t');
            break;
        case '0':
            out.push_back('\0');
            break;
        case '\\':
            out.push_back('\\');
            break;
        case '"':
            out.push_back('"');
            break;
        default:
            out.push_back(EscapeChar);
            out.push_back(Word[i]);
            out.push_back(Word[++i]);
            continue;
        }
        ++i;
    }
    return out;
}
