#include "Interpreter.h"
#include "HttpClient.h"
#include "IRunner.h"
#include <charconv>
#include <iostream>
#include <iterator>
#include <optional>
#include <vector>

namespace {

enum class NumKind { Int, Float, NaN };

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
			if (ec != std::errc() || ptr != last) {
				kind = NumKind::NaN;
				return;
			}
			d = static_cast<long double>(i);
		} else {
			kind = NumKind::Float;
			auto [ptr, ec] = std::from_chars(first, last, d);
			if (ec != std::errc() || ptr != last) {
				kind = NumKind::NaN;
				return;
			}
			i = static_cast<std::int64_t>(d);
		}
	}
};

template<typename T>
StackWord ToStackWord(T s) {
	auto text = std::to_string(s);
	return StackWord(std::string(text.rbegin(), text.rend()), false);
}

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
	  DataFile(fs, std::filesystem::path(CodeFilePath).replace_extension(StackFile::StackExtension)) {
		if (!Fs.exists(CodeFilePath)) {
			throw std::runtime_error("File does not exist: " + CodeFilePath.string());
		}
	}

void Interpreter::PushProgramArgs(const std::vector<std::string>& args) {
	for (const auto& arg : args)
		DataFile << arg;
}

std::string Interpreter::PopFinalResult() {
	auto lastWord = DataFile.PopWord().Escape();
	
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
		DataFile << word;
		return;
	}

	if (word.Word == Words::Def) {
		auto name = DataFile.PopWord();
		auto meaning = DataFile.PopWord();
		if (name.Word.empty())
			return;
		
		auto defFile = StackFile::Find(Fs, CodeFilePath, name.Word, StackFile::DefExtension);
		meaning.Word = meaning.Escape();
		meaning.Literal = false;
		defFile << meaning;

		return;
	}
	if (word.Word == Words::Label) {
		auto name = DataFile.PopWord();
		auto labelFile = StackFile::Find(Fs, CodeFilePath, name.Word, StackFile::LabelExtension);
		labelFile << CodeFile;

		return;
	}
	if (word.Word == Words::Jump) {
		auto name = DataFile.PopWord();
		if (name.Word.empty())
			return;
		CodeFile << StackFile::Find(Fs, CodeFilePath, name.Word, StackFile::LabelExtension);
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
		std::cin >> line;
		DataFile << StackWord(std::move(line), true);
		return;
	}
	
	if (word.Word == Words::Dupe) {
		auto top = DataFile.PopWord();
		if (top.Word.empty())
			return;
		DataFile << top;
		DataFile << top;
		return;
	}
	if (word.Word == Words::Pop) {
		auto top = DataFile.PopWord();
		(void)top;
		return;
	}
	if (word.Word == Words::Reverse) {
		auto top = DataFile.PopWord();
		if (top.Word.empty())
			return;
		DataFile << top;
		return;
	}
	if (word.Word == Words::Length) {
		auto top = DataFile.PopWord();
		DataFile << StackWord(std::to_string(top.Word.length()), false);
		return;
	}
	if (word.Word == Words::Index) {
		auto idx = DataFile.PopWord();
		auto str = DataFile.PopWord();
		auto idxNum = ParsedNum(idx.Word);
		if (idxNum.kind == NumKind::NaN)
			return;

		auto newStr = str.Word[(idxNum.i > 0 ? idxNum.i % str.Word.length() : str.Word.length() % (str.Word.length() - -idxNum.i))];

		DataFile << StackWord(std::to_string(newStr), true);
		return;
	}
	if (word.Word == Words::Slice) {
		auto start = DataFile.PopWord();
		auto end = DataFile.PopWord();
		auto str = DataFile.PopWord();

		auto startNum = ParsedNum(start.Word);
		auto endNum = ParsedNum(end.Word);

		if (str.Word.empty())
			return;

		if (startNum.kind == NumKind::NaN)
			startNum = ParsedNum("0");
		if (endNum.kind == NumKind::NaN)
			endNum = ParsedNum(std::to_string(str.Word.length()));

		auto newStr = str.Word.substr(startNum.i, endNum.i - startNum.i);

		DataFile << StackWord(std::move(newStr), true);

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

		const char delim = sep.Word[0];
		std::vector<std::string> parts;
		std::string collect;
		collect.reserve(str.Word.size());

		for (char c : str.Word) {
			if (c == delim) {
				parts.push_back(std::move(collect));
				collect.clear();
			} else {
				collect.push_back(c);
			}
		}
		parts.push_back(std::move(collect));

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
			return;

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
		else
			DataFile << ToStackWord(rhsNum.i + lhsNum.i);
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
			if (rhs.i == 0)
				return;
				
			DataFile << ToStackWord(lhs.d / rhs.d);
		});
		return;
	}
	if (word.Word == Words::Mod) {
		doBinaryArithmetic([this](const ParsedNum& lhs, const ParsedNum& rhs) {
			DataFile << ToStackWord(lhs.i % rhs.i);
		});
		return;
	}
	if (word.Word == Words::Pow) {
		doBinaryArithmetic([this](const ParsedNum& lhs, const ParsedNum& rhs) {
			if (lhs.d == 0.0L && rhs.d < 0.0L)
				return;
			DataFile << ToStackWord(std::pow(lhs.d, rhs.d));
		});
		return;
	}
	if (word.Word == Words::Eq) {
		doBinaryArithmetic([this](const ParsedNum& lhs, const ParsedNum& rhs) {
			if (lhs.kind == NumKind::Float || rhs.kind == NumKind::Float)
				DataFile << StackWord(std::to_string(lhs.d == rhs.d), false);
			else
				DataFile << StackWord(std::to_string(lhs.i == rhs.i), false);
		});
		return;
	}
	if (word.Word == Words::Neq) {
		doBinaryArithmetic([this](const ParsedNum& lhs, const ParsedNum& rhs) {
			if (lhs.kind == NumKind::Float || rhs.kind == NumKind::Float)
				DataFile << ToStackWord(lhs.d != rhs.d);
			else
				DataFile << ToStackWord(lhs.i != rhs.i);
		});
		return;
	}
	if (word.Word == Words::Lt) {
		doBinaryArithmetic([this](const ParsedNum& lhs, const ParsedNum& rhs) {
			if (lhs.kind == NumKind::Float || rhs.kind == NumKind::Float)
				DataFile << ToStackWord(lhs.d < rhs.d);
			else
				DataFile << ToStackWord(lhs.i < rhs.i);
		});
		return;
	}
	if (word.Word == Words::Gt) {
		doBinaryArithmetic([this](const ParsedNum& lhs, const ParsedNum& rhs) {
			if (lhs.kind == NumKind::Float || rhs.kind == NumKind::Float)
				DataFile << ToStackWord(lhs.d > rhs.d);
			else
				DataFile << ToStackWord(lhs.i > rhs.i);
		});
		return;
	}
	if (word.Word == Words::Le) {
		doBinaryArithmetic([this](const ParsedNum& lhs, const ParsedNum& rhs) {
			if (lhs.kind == NumKind::Float || rhs.kind == NumKind::Float)
				DataFile << StackWord(std::to_string(lhs.d <= rhs.d), false);
			else
				DataFile << StackWord(std::to_string(lhs.i <= rhs.i), false);
		});
		return;
	}
	if (word.Word == Words::Ge) {
		doBinaryArithmetic([this](const ParsedNum& lhs, const ParsedNum& rhs) {
			if (lhs.kind == NumKind::Float || rhs.kind == NumKind::Float)
				DataFile << ToStackWord(lhs.d >= rhs.d);
			else
				DataFile << ToStackWord(lhs.i >= rhs.i);
		});
		return;
	}

	auto doLogic = [this](auto op) {
		auto rhs = DataFile.PopWord();
		auto lhs = DataFile.PopWord();
		if (lhs.Word.empty() || rhs.Word.empty())
			lhs = StackWord("0", true);
		if (rhs.Word.empty())
			rhs = StackWord("0", true);
		auto lhsNum = ParsedNum(lhs.Word);
		auto rhsNum = ParsedNum(rhs.Word);
		if (lhsNum.kind == NumKind::NaN)
			lhsNum = ParsedNum("0");
		if (rhsNum.kind == NumKind::NaN)
			rhsNum = ParsedNum("0");
		op(lhsNum, rhsNum);
	};

	if (word.Word == Words::LogicAnd) {
		doLogic([this](const ParsedNum& lhs, const ParsedNum& rhs) {
			DataFile << ToStackWord(bool(lhs.i) && bool(rhs.i) ? 1 : 0);
		});
		return;
	}
	if (word.Word == Words::LogicOr) {
		doLogic([this](const ParsedNum& lhs, const ParsedNum& rhs) {
			DataFile << ToStackWord(bool(lhs.i) || bool(rhs.i) ? 1 : 0);
		});
		return;
	}
	if (word.Word == Words::LogicXor) {
		doLogic([this](const ParsedNum& lhs, const ParsedNum& rhs) {
			DataFile << ToStackWord(bool(lhs.i) ^ bool(rhs.i) ? 1 : 0);
		});
		return;
	}
	if (word.Word == Words::LogicNot) {
		auto top = DataFile.PopWord();
		auto topNum = ParsedNum(top.Word);
		if (topNum.kind == NumKind::NaN)
			topNum = ParsedNum("0");
		DataFile << ToStackWord(!bool(topNum.i) ? 1 : 0);
		return;
	}
	
	if (!HasOption(Option::NOFS)) {
		if (word.Word == Words::Open) {
			auto pathWord = DataFile.PopWord();
			std::filesystem::path path(pathWord.Escape());
			if (path.is_relative())
				path = WorkDir / path;
			if (!Fs.exists(path))
				return;
			auto in = Fs.open(path, std::ios::in | std::ios::binary);
			if (!in || !in->good())
				return;
			std::string content((std::istreambuf_iterator<char>(*in)), std::istreambuf_iterator<char>());
			DataFile << StackWord(std::move(content), true);
			return;
		}
	}

	if (!HasOption(Option::NOWEB)) {
		auto doHttp = [this](std::string_view method) {
			auto payload = DataFile.PopWord();
			auto address = DataFile.PopWord();
			if (address.Word.empty())
				return;
			auto response = HttpRequest(method, address.Word, payload.Word);
			if (!response.has_value())
				return;
			DataFile << StackWord(std::move(*response), true);
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