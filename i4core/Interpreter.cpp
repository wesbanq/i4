#include "Interpreter.h"
#include "IFileSystem.h"
#include <algorithm>
#include <cctype>
#include <cerrno>
#include <charconv>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <iterator>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

namespace {

enum class NumKind { Int, Float, NaN };

/// Converts like a cast, but saturates where a cast would be undefined behavior.
std::int64_t ClampToInt(long double d) {
	using Limits = std::numeric_limits<std::int64_t>;
	return d >= Limits::max() ? Limits::max() : d <= Limits::min() ? Limits::min() : static_cast<std::int64_t>(d);
}

/// Parses all of s as a long double, accepting exactly what std::from_chars would (no leading
/// whitespace, '+' or hex). libc++ (macOS) has no from_chars for long double, so this uses
/// strtold, which reads '.' as the decimal point since i4 never changes the C locale.
bool ParseLongDouble(const std::string& s, long double& out) {
	if (s.empty() || std::isspace(static_cast<unsigned char>(s[0])) || s[0] == '+'
		|| s.find_first_of("xX") != std::string::npos)
		return false;
	errno = 0;
	char* end;
	out = std::strtold(s.c_str(), &end);
	// Out of range means too large, or too small to be anything but zero; tiny subnormal values are fine.
	return end == s.c_str() + s.size() && !(errno == ERANGE && (out == 0 || std::isinf(out)));
}

struct ParsedNum {
	NumKind kind;
	std::int64_t i;
	long double d;

	ParsedNum(std::int64_t _i) : kind(NumKind::Int), i(_i), d(_i) {}
	ParsedNum(long double _d) : kind(NumKind::Float), i(ClampToInt(_d)), d(_d) {}
	explicit ParsedNum(std::string s) {
		const char* first = s.data();
		const char* last = first + s.size();
		if (std::find(s.begin(), s.end(), '.') == s.end()) {
			kind = NumKind::Int;
			auto [ptr, ec] = std::from_chars(first, last, i);
			if (ec != std::errc() || ptr != last) {
				kind = NumKind::NaN;
				return;
			}
			d = static_cast<long double>(i);
		} else {
			kind = NumKind::Float;
			if (!ParseLongDouble(s, d)) {
				kind = NumKind::NaN;
				return;
			}
			i = ClampToInt(d);
		}
	}
};

template<typename T>
StackWord ToStackWord(T s, bool literal = false) {
	auto text = std::to_string(s);
	return StackWord(std::string(text.rbegin(), text.rend()), literal);
}

/// Position in a non-empty string of the given length, wrapping around in both directions.
std::size_t Wrap(std::int64_t index, std::size_t length) {
	const auto n = static_cast<std::int64_t>(length);
	return static_cast<std::size_t>((index % n + n) % n);
}

/// Per the README: numbers are true when non-zero, anything else when non-empty.
bool IsTrue(const StackWord& word) {
	const auto num = ParsedNum(word.Word);
	return num.kind == NumKind::NaN ? !word.Word.empty() : num.d != 0;
}

} // namespace

Interpreter::Interpreter(const IFileSystem& fs, 
						 std::filesystem::path mainFile, 
						 std::ostream& outputStream,
                         unsigned char options)
	: OutputStream(outputStream),
	  Options(options),
	  Fs(fs),
	  WorkDir(mainFile.parent_path()),
	  CodeFilePath(mainFile),
	  CodeFile(fs, CodeFilePath),
	  DataFile(fs, std::filesystem::path(CodeFilePath).replace_extension(StackFile::StackExtension), StackFile::Kind::Data) {
		if (!Fs.exists(CodeFilePath)) {
			throw std::runtime_error("File does not exist: " + CodeFilePath.string());
		}
	}

void Interpreter::PushArgs(const std::vector<std::string>& args) {
	for (const auto& arg : args)
		DataFile << StackWord(arg, true);
}

ReturnCode Interpreter::PopResult() {
	const auto lastWord = DataFile.PopWord().Escape();
	const auto lastNum = ParsedNum(lastWord);
	
	if (lastNum.kind == NumKind::NaN)
		return { lastWord.empty() ? "OK" : lastWord, lastWord.empty() || lastWord == "OK" ? 0u : 1u };
	else
		return { lastWord, static_cast<unsigned char>(lastNum.i) };
}

ReturnCode Interpreter::Run(std::vector<std::string> args) {
	PushArgs(args);
	while (!Finished())
		Step();
	return PopResult();
}

void Interpreter::Step() {
	auto word = CodeFile.PopWord();

	if (HasOption(Option::VERBOSE))
		std::cout << word.Format() << '\n';
	
	if (word.Literal) {
		DataFile << word;
		return;
	}

	if (word.Word == Words::Def) {
		auto name = DataFile.PopWord();
		auto meaning = DataFile.PopWord();
		
		if (name.Word.empty())
			throw std::runtime_error("Definition name is empty");
		
		auto defFile = StackFile::Find(Fs, CodeFilePath, name.Word, StackFile::DefExtension);
		meaning.Word = meaning.Escape();
		meaning.Literal = false;
		
		defFile.Clear();
		defFile << meaning;
		return;
	}
	if (word.Word == Words::Label) {
		auto name = DataFile.PopWord();
		if (name.Word.empty())
			throw std::runtime_error("Label name is empty");

		auto labelFile = StackFile::Find(Fs, CodeFilePath, name.Word, StackFile::LabelExtension);
		labelFile.Clear();
		labelFile << CodeFile;
		return;
	}
	if (word.Word == Words::Jump) {
		auto name = DataFile.PopWord();
		if (name.Word.empty())
			throw std::runtime_error("Jump target is empty");

		auto labelFile = StackFile::Find(Fs, CodeFilePath, name.Word, StackFile::LabelExtension);
		if (!Fs.exists(labelFile.GetPath()))
			return;
		CodeFile.Clear();
		CodeFile << labelFile;
		return;
	}
	if (word.Word == Words::Inject) {
		auto name = DataFile.PopWord();
		name.Word = name.Escape();
		name.Literal = false;
		CodeFile << name;
		return;
	}
	if (word.Word == Words::Consume) {
		auto word = CodeFile.PopWord();
		DataFile << word;
		return;
	}
	if (word.Word == Words::Swap) {
		auto codeTop = CodeFile.PopWord();
		auto dataTop = DataFile.PopWord();

		CodeFile << dataTop;
		DataFile << codeTop;
		return;
	}
	if (word.Word == Words::Out) {
		auto top = DataFile.PopWord();
		OutputStream << top.Escape();
		return;
	}
	if (word.Word == Words::In) {
		std::cout << "? ";
		std::string line;
		std::getline(std::cin, line);

		DataFile << StackWord(std::move(line), true);
		return;
	}
	
	if (word.Word == Words::Dupe) {
		auto top = DataFile.PopWord();
		DataFile << top;
		DataFile << top;
		return;
	}
	if (word.Word == Words::Pop) {
		DataFile.PopWord();
		return;
	}
	if (word.Word == Words::Reverse) {
		auto top = DataFile.PopWord();
		DataFile << top;
		return;
	}
	if (word.Word == Words::Length) {
		auto top = DataFile.PopWord();
		DataFile << ToStackWord(top.Word.length(), true);
		return;
	}
	if (word.Word == Words::Index) {
		const auto idx = ParsedNum(DataFile.PopWord().Word);
		const auto str = DataFile.PopWord().Word;
		if (idx.kind == NumKind::NaN || str.empty())
			DataFile.PushEmpty();
		else
			DataFile << StackWord(std::string(1, str[Wrap(idx.i, str.length())]), true);
		return;
	}
	if (word.Word == Words::Slice) {
		const auto start = ParsedNum(DataFile.PopWord().Word);
		const auto end = ParsedNum(DataFile.PopWord().Word);
		const auto str = DataFile.PopWord().Word;
		const auto length = str.length();
		if (length == 0)
			return DataFile.PushEmpty();

		// Both ends wrap like @, except that an end equal to the length means "to the end",
		// and a start past the end wraps around the end of the string.
		const auto from = start.kind == NumKind::NaN ? 0 : Wrap(start.i, length);
		const auto to = end.kind == NumKind::NaN || end.i == static_cast<std::int64_t>(length) ? length : Wrap(end.i, length);
		DataFile << StackWord(from <= to ? str.substr(from, to - from) : str.substr(from) + str.substr(0, to), true);
		return;
	}
	if (word.Word == Words::Split) {
		auto sep = DataFile.PopWord();
		auto str = DataFile.PopWord();

		if (sep.Word.empty()) {
			for (auto it = str.Word.rbegin(); it != str.Word.rend(); ++it)
				DataFile << StackWord(std::string(1, *it), true);
			return;
		}

		std::vector<std::string> parts;
		std::size_t from = 0;
		for (std::size_t found; (found = str.Word.find(sep.Word, from)) != std::string::npos; from = found + sep.Word.size())
			parts.push_back(str.Word.substr(from, found - from));
		parts.push_back(str.Word.substr(from));

		for (auto it = parts.rbegin(); it != parts.rend(); ++it)
			DataFile << StackWord(std::move(*it), true);

		return;
	}
	if (word.Word == Words::Halt) {
		CodeFile.Halt();
		return;
	}

	auto doBinaryArithmetic = [this](auto op) {
		auto rhsW = DataFile.PopWord();
		auto lhsW = DataFile.PopWord();

		if (rhsW.Word.empty())
			rhsW = StackWord("0", true);
		if (lhsW.Word.empty())
			lhsW = StackWord("0", true);

		auto rhs = ParsedNum(rhsW.Word);
		auto lhs = ParsedNum(lhsW.Word);

		if (lhs.kind == NumKind::NaN || rhs.kind == NumKind::NaN)
			DataFile.PushEmpty();
		else
			op(rhs, lhs);
	};

	if (word.Word == Words::Add) {
		auto rhs = DataFile.PopWord();
		auto lhs = DataFile.PopWord();
		auto rhsNum = ParsedNum(rhs.Word);
		auto lhsNum = ParsedNum(lhs.Word);

		if (rhsNum.kind == NumKind::NaN || lhsNum.kind == NumKind::NaN) {
			DataFile << StackWord(rhs.Word + lhs.Word, lhs.Literal || rhs.Literal);
			return;
		}
		else if (rhsNum.kind == NumKind::Int && lhsNum.kind == NumKind::Int)
			DataFile << ToStackWord(rhsNum.i + lhsNum.i);
		else
			DataFile << ToStackWord(rhsNum.d + lhsNum.d);
		return;
	}
	if (word.Word == Words::Sub) {
		doBinaryArithmetic([this](const ParsedNum& lhs, const ParsedNum& rhs) {
			if (lhs.kind == NumKind::Int && rhs.kind == NumKind::Int)
				DataFile << ToStackWord(lhs.i - rhs.i);
			else
				DataFile << ToStackWord(lhs.d - rhs.d);
		});
		return;
	}
	if (word.Word == Words::Mul) {
		doBinaryArithmetic([this](const ParsedNum& lhs, const ParsedNum& rhs) {
			if (lhs.kind == NumKind::Int && rhs.kind == NumKind::Int)
				DataFile << ToStackWord(lhs.i * rhs.i);
			else
				DataFile << ToStackWord(lhs.d * rhs.d);
		});
		return;
	}
	if (word.Word == Words::Div) {
		doBinaryArithmetic([this](const ParsedNum& lhs, const ParsedNum& rhs) {
			if (rhs.d == 0)
				DataFile.PushEmpty();
			else
				DataFile << ToStackWord(lhs.d / rhs.d);
		});
		return;
	}
	if (word.Word == Words::Mod) {
		doBinaryArithmetic([this](const ParsedNum& lhs, const ParsedNum& rhs) {
			if (rhs.d == 0)
				DataFile.PushEmpty();
			else if (lhs.kind == NumKind::Int && rhs.kind == NumKind::Int)
				DataFile << ToStackWord(lhs.i % rhs.i);
			else
				DataFile << ToStackWord(std::fmod(lhs.d, rhs.d));
		});
		return;
	}
	if (word.Word == Words::Pow) {
		doBinaryArithmetic([this](const ParsedNum& lhs, const ParsedNum& rhs) {
			if (lhs.d == 0.0L && rhs.d < 0.0L)
				DataFile.PushEmpty();
			else
				DataFile << ToStackWord(std::pow(lhs.d, rhs.d));
		});
		return;
	}

	// Numbers compare as numbers; if either side isn't one, both compare as text. Empty counts as 0.
	auto doCompare = [this](auto cmp) {
		auto lhsW = DataFile.PopWord().Word;
		auto rhsW = DataFile.PopWord().Word;
		if (lhsW.empty()) lhsW = "0";
		if (rhsW.empty()) rhsW = "0";
		const ParsedNum lhs(lhsW), rhs(rhsW);
		const bool result = lhs.kind == NumKind::NaN || rhs.kind == NumKind::NaN ? cmp(lhsW, rhsW)
			: lhs.kind == NumKind::Float || rhs.kind == NumKind::Float ? cmp(lhs.d, rhs.d)
			: cmp(lhs.i, rhs.i);
		DataFile << ToStackWord(result ? 1 : 0);
	};
	if (word.Word == Words::Eq) return doCompare(std::equal_to<>());
	if (word.Word == Words::Neq) return doCompare(std::not_equal_to<>());
	if (word.Word == Words::Lt) return doCompare(std::less<>());
	if (word.Word == Words::Gt) return doCompare(std::greater<>());
	if (word.Word == Words::Le) return doCompare(std::less_equal<>());
	if (word.Word == Words::Ge) return doCompare(std::greater_equal<>());

	auto doLogic = [this](auto op) {
		const bool rhs = IsTrue(DataFile.PopWord()), lhs = IsTrue(DataFile.PopWord());
		DataFile << ToStackWord(op(lhs, rhs) ? 1 : 0);
	};
	if (word.Word == Words::LogicAnd) return doLogic(std::logical_and<>());
	if (word.Word == Words::LogicOr) return doLogic(std::logical_or<>());
	if (word.Word == Words::LogicXor) return doLogic(std::not_equal_to<>());
	if (word.Word == Words::LogicNot) return DataFile.PushWord(ToStackWord(IsTrue(DataFile.PopWord()) ? 0 : 1));
	
	if (!HasOption(Option::NOFS)) {
		if (word.Word == Words::Open) {
			auto pathWord = DataFile.PopWord();
			std::filesystem::path path(pathWord.Escape());
			if (path.is_relative()) path = WorkDir / path;

			if (!Fs.exists(path)) {
				DataFile.PushEmpty();
				return;
			}

			auto in = Fs.open(path, std::ios::in | std::ios::binary);
			if (!in || !in->good())
				throw std::runtime_error("Failed to open file for reading: " + path.string());
			
			std::string content((std::istreambuf_iterator<char>(*in)), std::istreambuf_iterator<char>());
			DataFile << StackWord(std::move(content), true);
			return;
		}
		if (word.Word == Words::Write) {
			auto pathWord = DataFile.PopWord();
			auto contentWord = DataFile.PopWord();
			
			std::filesystem::path path(pathWord.Escape());
			if (path.is_relative()) path = WorkDir / path;
			
			auto out = Fs.open(path, std::ios::out | std::ios::binary | std::ios::trunc);
			if (!out || !out->good())
				throw std::runtime_error("Failed to open file for writing: " + path.string());
			
			*out << contentWord.Escape();
			return;
		}
	}

	auto def = StackFile::Find(Fs, CodeFilePath, word.Word, StackFile::DefExtension);
	if (Fs.exists(def.GetPath())) {
		CodeFile << def;
		return;
	}

	DataFile << word;
}

bool Interpreter::Finished() const {
	return CodeFile.Size() == 0;
}

bool Interpreter::HasOption(Option opt) const {
	return HasOption(Options, opt);
}

bool Interpreter::HasOption(unsigned char options, Option opt) {
	return (options & static_cast<unsigned char>(opt)) == static_cast<unsigned char>(opt);
}