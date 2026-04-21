#pragma once

#include <string>
#include <string_view>

namespace Words {
    inline static constexpr std::string_view Def            = "fed";
    inline static constexpr std::string_view Label          = ":";
    inline static constexpr std::string_view Jump           = ":^";
    inline static constexpr std::string_view Out            = ".";
    inline static constexpr std::string_view In             = "?";
    inline static constexpr std::string_view Open           = "nepo";
    inline static constexpr std::string_view Dupe           = "~~";
    inline static constexpr std::string_view Pop            = "!p";
    inline static constexpr std::string_view Inject         = "<<";
    inline static constexpr std::string_view Consume        = ">>";
    inline static constexpr std::string_view Reverse        = "<>";
    inline static constexpr std::string_view Length         = "#";
    inline static constexpr std::string_view Index          = "@";
    inline static constexpr std::string_view Halt           = "!tlh";
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
    inline static constexpr std::string_view BitAnd         = "&";
    inline static constexpr std::string_view BitOr          = "|";
    inline static constexpr std::string_view BitXor         = "^";
    inline static constexpr std::string_view BitNot         = "!";
    inline static constexpr std::string_view LogicAnd       = "&&";
    inline static constexpr std::string_view LogicOr        = "||";
    inline static constexpr std::string_view LogicNot       = "~";
    inline static constexpr std::string_view Shl            = "<<";
    inline static constexpr std::string_view Shr            = ">>";
} // namespace Words

struct StackWord {
    std::string Word;
    bool Literal;

    static constexpr auto Separator = ' ';
    static constexpr auto EscapeChar = '\\';
    static constexpr auto Quote = '"';

    StackWord();
    StackWord(std::string word, bool literal = false) : Word(std::move(word)), Literal(literal) { }

    std::string Format() const {
        return Literal ? "\"" + Word + "\"" : Word;
    }

    std::string Escape() const {
        //if (!Literal)
        //    return Word;
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
};