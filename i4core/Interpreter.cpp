#include "Interpreter.h"
#include "HttpClient.h"
#include "IRunner.h"
#include <charconv>
#include <iostream>
#include <iterator>
#include <optional>

namespace {

enum class NumKind { Int, Float };

struct ParsedNum {
	NumKind kind;
	std::int64_t i;
	long double d;

	ParsedNum(std::int64_t _i) : kind(NumKind::Int), i(_i), d(_i) {}
	ParsedNum(long double _d) : kind(NumKind::Float), i(_d), d(_d) {}
	explicit ParsedNum(std::string s) {
		const char* first = s.data();
		const char* last = first + s.size();
		if (std::find(s.begin(), s.end(), '.') == s.end()) {
			kind = NumKind::Int;
			auto [ptr, ec] = std::from_chars(first, last, i);
			if (ec != std::errc() || ptr != last)
				throw std::runtime_error("Invalid number: " + s);
			d = static_cast<long double>(i);
		} else {
			kind = NumKind::Float;
			auto [ptr, ec] = std::from_chars(first, last, d);
			if (ec != std::errc() || ptr != last)
				throw std::runtime_error("Invalid number: " + s);
			i = static_cast<std::int64_t>(d);
		}
	}
};

} // namespace

Interpreter::Interpreter(const IRunner& fs, 
						 std::filesystem::path mainFile, 
						 std::ostream& outputStream,
                         unsigned char options)
	: OutputStream(outputStream),
	  Options(options),
	  Fs(fs),
	  WorkDir(mainFile.parent_path()),
	  CodeFilePath(mainFile),
	  CodeFile(fs, CodeFilePath),
	  StackFile(fs, std::filesystem::path(CodeFilePath).replace_extension(StackFile::StackExtension)) {
		if (!Fs.exists(CodeFilePath)) {
			throw std::runtime_error("File does not exist: " + CodeFilePath.string());
		}
	}

void Interpreter::PushProgramArgs(const std::vector<std::string>& args) {
	for (const auto& arg : args)
		StackFile << arg;
}

std::string Interpreter::PopFinalResult() {
	auto lastWord = StackFile.PopWord().Word;
	
	if (lastWord == "")
		lastWord = "OK";
	
	return lastWord;
}

std::string Interpreter::Run(std::vector<std::string> args) {
	PushProgramArgs(args);
	while (!Finished())
		Step();
	return PopFinalResult();
}

void Interpreter::Step() {
	auto word = CodeFile.PopWord();

	if (HasOption(Option::VERBOSE))
		std::cout << word.Format() << '\n';

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
			if (!in || !in->good())
				return;
			std::string content((std::istreambuf_iterator<char>(*in)), std::istreambuf_iterator<char>());
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

		if (rhsW.Word.empty())
			rhsW = StackWord("0", true);
		if (lhsW.Word.empty())
			lhsW = StackWord("0", true);

		auto rhs = ParsedNum(rhsW.Word);
		auto lhs = ParsedNum(lhsW.Word);
		op(lhs, rhs);
	};

	if (word.Word == Words::Add) {
		doBinaryArithmetic([this](const ParsedNum& lhs, const ParsedNum& rhs) {
			if (lhs.kind == NumKind::Int && rhs.kind == NumKind::Int)
				StackFile << StackWord(std::to_string(lhs.i + rhs.i), true);
			else
				StackFile << StackWord(std::to_string(lhs.d + rhs.d), true);
		});
		return;
	}
	if (word.Word == Words::Sub) {
		doBinaryArithmetic([this](const ParsedNum& lhs, const ParsedNum& rhs) {
			if (lhs.kind == NumKind::Int && rhs.kind == NumKind::Int)
				StackFile << StackWord(std::to_string(lhs.i - rhs.i), true);
			else
				StackFile << StackWord(std::to_string(lhs.d - rhs.d), true);
		});
		return;
	}
	if (word.Word == Words::Mul) {
		doBinaryArithmetic([this](const ParsedNum& lhs, const ParsedNum& rhs) {
			if (lhs.kind == NumKind::Int && rhs.kind == NumKind::Int)
				StackFile << StackWord(std::to_string(lhs.i * rhs.i), true);
			else
				StackFile << StackWord(std::to_string(lhs.d * rhs.d), true);
		});
		return;
	}
	if (word.Word == Words::Div) {
		doBinaryArithmetic([this](const ParsedNum& lhs, const ParsedNum& rhs) {
			if (rhs.i == 0)
				return;
				
			StackFile << StackWord(std::to_string(lhs.d / rhs.d), true);
		});
		return;
	}
	if (word.Word == Words::Mod) {
		doBinaryArithmetic([this](const ParsedNum& lhs, const ParsedNum& rhs) {
			StackFile << StackWord(std::to_string(lhs.i % rhs.i), true);
		});
		return;
	}
	if (word.Word == Words::Pow) {
		doBinaryArithmetic([this](const ParsedNum& lhs, const ParsedNum& rhs) {
			if (lhs.d == 0.0L && rhs.d < 0.0L)
				return;
			StackFile << StackWord(std::to_string(std::pow(lhs.d, rhs.d)), true);
		});
		return;
	}

	if (!HasOption(Option::NOWEB)) {
		auto doHttp = [this](std::string_view method) {
			auto payload = StackFile.PopWord();
			auto address = StackFile.PopWord();
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

	auto def = StackFile::Find(Fs, CodeFilePath, word.Word, StackFile::DefExtension);
	if (Fs.exists(def.GetPath())) {
		CodeFile << def;
		return;
	}

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