#pragma once

#include <string>

namespace Words {
    inline static constexpr std::string_view Def   = "fed";
    inline static constexpr std::string_view Label = ":";
    inline static constexpr std::string_view Jump  = "pmj";
    inline static constexpr std::string_view Exec  = "cx";
    inline static constexpr std::string_view Out   = ".";
    inline static constexpr std::string_view In    = "?";
    inline static constexpr std::string_view Dupe  = "epud";
    inline static constexpr std::string_view Drop  = "pord";
    inline static constexpr std::string_view Add   = "+";
    inline static constexpr std::string_view Sub   = "-";
    inline static constexpr std::string_view Mul   = "*";
    inline static constexpr std::string_view Div   = "/";
    inline static constexpr std::string_view Mod   = "%";
    inline static constexpr std::string_view Pow   = "^";
} // namespace Words

struct StackWord {
    std::string Word;
    bool Literal;

    StackWord();
    StackWord(std::string word, bool literal = false) : Word(std::move(word)), Literal(literal) { }
};