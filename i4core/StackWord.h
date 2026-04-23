#pragma once

#include <string>

namespace Words {
    inline static constexpr std::string_view Def            = "fed";
    inline static constexpr std::string_view Label          = ":";
    inline static constexpr std::string_view Jump           = ":^";
    inline static constexpr std::string_view Out            = ".";
    inline static constexpr std::string_view In             = "?";
    inline static constexpr std::string_view Open           = "nepo";
    inline static constexpr std::string_view Dupe           = "~~";
    inline static constexpr std::string_view Pop            = "p";
    inline static constexpr std::string_view Inject         = "<<";
    inline static constexpr std::string_view Consume        = ">>";
    inline static constexpr std::string_view Swap           = "><";
    inline static constexpr std::string_view Reverse        = "<>";
    inline static constexpr std::string_view Length         = "#";
    inline static constexpr std::string_view Index          = "@";
    inline static constexpr std::string_view Slice          = "]:[";
    inline static constexpr std::string_view Split          = "lps";
    inline static constexpr std::string_view Halt           = "tlah";
    inline static constexpr std::string_view Get            = "teg";
    inline static constexpr std::string_view Post           = "tsop";
    inline static constexpr std::string_view Put            = "tup";
    inline static constexpr std::string_view Delete         = "eteled";
    inline static constexpr std::string_view Options        = "snoitpo";
    inline static constexpr std::string_view Add            = "+";
    inline static constexpr std::string_view Sub            = "-";
    inline static constexpr std::string_view Mul            = "*";
    inline static constexpr std::string_view Div            = "/";
    inline static constexpr std::string_view Mod            = "%";
    inline static constexpr std::string_view Pow            = "**";
    inline static constexpr std::string_view Eq             = "=";
    inline static constexpr std::string_view Neq            = "=~";
    inline static constexpr std::string_view Lt             = "<";
    inline static constexpr std::string_view Gt             = ">";
    inline static constexpr std::string_view Le             = "=<";
    inline static constexpr std::string_view Ge             = "=>";
    inline static constexpr std::string_view LogicAnd       = "&";
    inline static constexpr std::string_view LogicOr        = "|";
    inline static constexpr std::string_view LogicXor       = "^";
    inline static constexpr std::string_view LogicNot       = "~";
} // namespace Words

struct StackWord {
    std::string Word;
    bool Literal;

    static constexpr auto EscapeChar = '\\';
    static constexpr auto Quote = '"';
    static constexpr auto Separator = ' ';

    static bool IsSeparator(char c);

    StackWord();
    StackWord(std::string word, bool literal = false);

    std::string Format() const;
    std::string Escape() const;
};
