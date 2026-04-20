#include "Interpreter.h"
#include "HttpClient.h"
#include "IRunner.h"
#include <charconv>
#include <cerrno>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <iterator>
#include <optional>
#include <sstream>

namespace {

enum class NumKind { Int, Float };

struct ParsedNum {
	NumKind kind;
	std::int64_t i;
	long double d;
};

long double toLongDouble(const ParsedNum& n) {
	return n.kind == NumKind::Int ? static_cast<long double>(n.i) : n.d;
}

std::optional<ParsedNum> tryParseStackNumeric(const StackWord& w) {
	const std::string& s = w.Word;
	if (s.empty())
		return std::nullopt;

	std::int64_t i{};
	const auto* begin = s.data();
	const auto* end = begin + s.size();
	auto [ptr, ec] = std::from_chars(begin, end, i, 10);
	if (ec == std::errc{} && ptr == end)
		return ParsedNum{ NumKind::Int, i, 0 };

	char* parseEnd = nullptr;
	errno = 0;
	long double d = std::strtold(s.c_str(), &parseEnd);
	if (parseEnd == s.c_str() + s.size() && errno == 0 && parseEnd != s.c_str())
		return ParsedNum{ NumKind::Float, 0, d };

	return std::nullopt;
}

std::string formatNumericResult(std::int64_t v) {
	return std::to_string(v);
}

std::string formatNumericResult(long double v) {
	std::ostringstream os;
	os.precision(21);
	os << v;
	return os.str();
}

} // namespace

Interpreter::Interpreter(const IRunner& fs, std::filesystem::path mainFile, std::ostream& outputStream,
                         unsigned char options)
	: OutputStream(outputStream),
	  Options(options),
	  Fs(fs),
	  WorkDir(mainFile.parent_path()),
	  CodeFilePath(mainFile),
	  CodeFile(fs, CodeFilePath),
	  StackFile(fs, CodeFilePath.replace_extension(StackFile::StackExtension)) {
		if (!Fs.exists(CodeFilePath)) {
			throw std::runtime_error("File does not exist: " + CodeFilePath.string());
		}
	}

void Interpreter::PushProgramArgs(const std::vector<std::string>& args) {
	for (const auto& arg : args)
		StackFile << arg;
}

std::string Interpreter::PopFinalResult() {
	return StackFile.PopWord().Word;
}

std::string Interpreter::Run(std::vector<std::string> args) {
	PushProgramArgs(args);
	while (!Finished())
		Step();
	return PopFinalResult();
}

void Interpreter::Step() {
	auto word = CodeFile.PopWord();
	if (word.Word.empty())
		return;
	
	if (word.Literal) {
		StackFile << word;
		return;
	}

	if (word.Word == Words::Def) {
		auto name = StackFile.PopWord();
		auto meaning = StackFile.PopWord();
		if (name.Word.empty() || meaning.Word.empty())
			return;
		
		auto defFile = StackFile::Find(Fs, CodeFilePath, name.Word, StackFile::DefExtension);
		meaning.Literal = false;
		defFile << meaning;

		return;
	}
	if (word.Word == Words::Label) {
		auto name = StackFile.PopWord();
		if (name.Word.empty())
			return;
		
		auto labelFile = StackFile::Find(Fs, CodeFilePath, name.Word, StackFile::LabelExtension);
		labelFile << word;

		return;
	}
	if (word.Word == Words::Jump) {
		auto name = StackFile.PopWord();
		if (name.Word.empty())
			return;
		CodeFile << StackFile::Find(Fs, CodeFilePath, name.Word, StackFile::LabelExtension);
		return;
	}
	if (word.Word == Words::Exec) {
		auto name = StackFile.PopWord();
		if (name.Word.empty())
			return;
		CodeFile << StackFile::Find(Fs, CodeFilePath, name.Word, StackFile::DefExtension);
		return;
	}
	if (word.Word == Words::Out) {
		auto top = StackFile.PopWord();
		if (top.Word.empty())
			return;
		OutputStream << top.Word;
		return;
	}
	if (word.Word == Words::In) {
		std::string line;
		if (!std::getline(std::cin, line))
			return;
		StackFile << StackWord(std::move(line), true);
		return;
	}

	if (!HasOption(Option::NOFS)) {
		if (word.Word == Words::Open) {
			auto pathWord = StackFile.PopWord();
			if (pathWord.Word.empty())
				return;
			std::filesystem::path path(pathWord.Word);
			if (path.is_relative())
				path = WorkDir / path;
			if (!Fs.exists(path))
				return;
			auto in = Fs.open(path, std::ios::in | std::ios::binary);
			if (!in.good())
				return;
			std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
			StackFile << StackWord(std::move(content), true);
			return;
		}
	}
	
	if (word.Word == Words::Dupe) {
		auto top = StackFile.PopWord();
		if (top.Word.empty())
			return;
		StackFile << top;
		StackFile << top;
		return;
	}
	if (word.Word == Words::Drop) {
		auto top = StackFile.PopWord();
		(void)top;
		return;
	}

	auto doBinaryArithmetic = [this](auto op) {
		auto rhsW = StackFile.PopWord();
		auto lhsW = StackFile.PopWord();
		if (rhsW.Word.empty() || lhsW.Word.empty())
			return;
		auto rhs = tryParseStackNumeric(rhsW);
		auto lhs = tryParseStackNumeric(lhsW);
		if (!rhs || !lhs)
			return;
		op(*lhs, *rhs);
	};

	if (word.Word == Words::Add) {
		doBinaryArithmetic([this](const ParsedNum& lhs, const ParsedNum& rhs) {
			if (lhs.kind == NumKind::Int && rhs.kind == NumKind::Int)
				StackFile << StackWord(formatNumericResult(lhs.i + rhs.i), true);
			else
				StackFile << StackWord(formatNumericResult(toLongDouble(lhs) + toLongDouble(rhs)), true);
		});
		return;
	}
	if (word.Word == Words::Sub) {
		doBinaryArithmetic([this](const ParsedNum& lhs, const ParsedNum& rhs) {
			if (lhs.kind == NumKind::Int && rhs.kind == NumKind::Int)
				StackFile << StackWord(formatNumericResult(lhs.i - rhs.i), true);
			else
				StackFile << StackWord(formatNumericResult(toLongDouble(lhs) - toLongDouble(rhs)), true);
		});
		return;
	}
	if (word.Word == Words::Mul) {
		doBinaryArithmetic([this](const ParsedNum& lhs, const ParsedNum& rhs) {
			if (lhs.kind == NumKind::Int && rhs.kind == NumKind::Int)
				StackFile << StackWord(formatNumericResult(lhs.i * rhs.i), true);
			else
				StackFile << StackWord(formatNumericResult(toLongDouble(lhs) * toLongDouble(rhs)), true);
		});
		return;
	}
	if (word.Word == Words::Div) {
		doBinaryArithmetic([this](const ParsedNum& lhs, const ParsedNum& rhs) {
			if (lhs.kind == NumKind::Int && rhs.kind == NumKind::Int) {
				if (rhs.i == 0)
					return;
				StackFile << StackWord(formatNumericResult(lhs.i / rhs.i), true);
			} else {
				const long double b = toLongDouble(rhs);
				if (b == 0.0L)
					return;
				StackFile << StackWord(formatNumericResult(toLongDouble(lhs) / b), true);
			}
		});
		return;
	}
	if (word.Word == Words::Mod) {
		doBinaryArithmetic([this](const ParsedNum& lhs, const ParsedNum& rhs) {
			if (lhs.kind != NumKind::Int || rhs.kind != NumKind::Int || rhs.i == 0)
				return;
			StackFile << StackWord(formatNumericResult(lhs.i % rhs.i), true);
		});
		return;
	}
	if (word.Word == Words::Pow) {
		doBinaryArithmetic([this](const ParsedNum& lhs, const ParsedNum& rhs) {
			const long double a = toLongDouble(lhs);
			const long double b = toLongDouble(rhs);
			if (a == 0.0L && b < 0.0L)
				return;
			StackFile << StackWord(formatNumericResult(std::pow(a, b)), true);
		});
		return;
	}

	if (!HasOption(Option::NOWEB)) {
		auto doHttp = [this](std::string_view method) {
			auto address = StackFile.PopWord();
			auto payload = StackFile.PopWord();
			if (address.Word.empty())
				return;
			auto response = HttpRequest(method, address.Word, payload.Word);
			if (!response.has_value())
				return;
			StackFile << StackWord(std::move(*response), true);
		};
	
		if (word.Word == Words::Get) {
			doHttp("GET");
			return;
		}
		if (word.Word == Words::Post) {
			doHttp("POST");
			return;
		}
		if (word.Word == Words::Put) {
			doHttp("PUT");
			return;
		}
		if (word.Word == Words::Delete) {
			doHttp("DELETE");
			return;
		}
		if (word.Word == Words::Options) {
			doHttp("OPTIONS");
			return;
		}
	}

	// if all else fails
	StackFile << word;
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