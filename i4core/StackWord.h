#pragma once

#include <string>

namespace Words {
    static constexpr std::string_view Def   = "fed";
    static constexpr std::string_view Label = ":";
    static constexpr std::string_view Jump  = "pmj";
    static constexpr std::string_view Exec  = "cx";
    static constexpr std::string_view Out   = ".";
    static constexpr std::string_view In    = "?";
    static constexpr std::string_view Dupe  = "epud";
    static constexpr std::string_view Drop  = "pord";
    static constexpr std::string_view Add   = "+";
    static constexpr std::string_view Sub   = "-";
    static constexpr std::string_view Mul   = "*";
    static constexpr std::string_view Div   = "/";
    static constexpr std::string_view Mod   = "%";
    static constexpr std::string_view Pow   = "^";
}

struct StackWord {
    std::string Word;
    bool Literal;
    
    void Reverse() {
        std::reverse(Word.begin(), Word.end());
    }

    StackWord();
    StackWord(std::string word, bool literal = false) : Word(std::move(word)), Literal(literal) { }
};