#pragma once

#include <string>

namespace Words {
    inline static constexpr std::string_view Def   = "fed";
    inline static constexpr std::string_view Label = ":";
    inline static constexpr std::string_view Jump  = "pmj";
    inline static constexpr std::string_view Exec  = "cx";
    inline static constexpr std::string_view Out   = ".";
    inline static constexpr std::string_view In    = "?";
    inline static constexpr std::string_view Open   = "nepo";
    inline static constexpr std::string_view Dupe  = "epud";
    inline static constexpr std::string_view Drop  = "pord";
    inline static constexpr std::string_view Reverse  = "<>";
    inline static constexpr std::string_view Get = "teg";
    inline static constexpr std::string_view Post = "tsop";
    inline static constexpr std::string_view Put = "tup";
    inline static constexpr std::string_view Delete = "eteled";
    inline static constexpr std::string_view Options = "snoitpo";
    inline static constexpr std::string_view Add   = "+";
    inline static constexpr std::string_view Sub   = "-";
    inline static constexpr std::string_view Mul   = "*";
    inline static constexpr std::string_view Div   = "/";
    inline static constexpr std::string_view Mod   = "%";
    inline static constexpr std::string_view Pow   = "**";
    inline static constexpr std::string_view Eq    = "=";
    inline static constexpr std::string_view Neq   = "=~";
    inline static constexpr std::string_view Lt    = "<";
    inline static constexpr std::string_view Gt    = ">";
    inline static constexpr std::string_view Le    = "=<";
    inline static constexpr std::string_view Ge    = "=>";
    inline static constexpr std::string_view BitAnd = "&";
    inline static constexpr std::string_view BitOr = "|";
    inline static constexpr std::string_view BitXor = "^";
    inline static constexpr std::string_view BitNot = "!";
    inline static constexpr std::string_view LogicAnd = "&&";
    inline static constexpr std::string_view LogicOr = "||";
    inline static constexpr std::string_view LogicNot = "~";
    inline static constexpr std::string_view Shl = "<<";
    inline static constexpr std::string_view Shr = ">>";
} // namespace Words

struct StackWord {
    std::string Word;
    bool Literal;

    StackWord();
    StackWord(std::string word, bool literal = false) : Word(std::move(word)), Literal(literal) { }

    std::string Format() const {
        return Literal ? "\"" + Word + "\"" : Word;
    }
};