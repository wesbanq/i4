#include "Interpreter.h"

Interpreter::Interpreter(std::filesystem::path mainFile, std::ostream& outputStream, unsigned char options) 
	: OutputStream(outputStream), 
	  Options(options), 
	  WorkDir(mainFile.parent_path()), 
	  CodeFilePath(mainFile), 
	  CodeFile(CodeFilePath), 
	  StackFile(CodeFilePath.replace_extension(StackFile::StackExtension)) {
		if (!std::filesystem::exists(CodeFilePath)) {
			throw std::runtime_error("File does not exist: " + CodeFilePath.string());
		}
	}

std::string Interpreter::Run(std::vector<std::string> args) {
	for (const auto& arg : args)
		StackFile << arg;
	
	while (!Finished())
		Step();
	

	return StackFile.PopWord().Word;
}

void Interpreter::Step() {
	auto word = CodeFile.PopWord();
	if (word.Word.empty())
		return;
	
	if (word.Literal)
		StackFile << word;

	if (word.Word == Words::Def) {
		auto name = StackFile.PopWord();
		auto meaning = StackFile.PopWord();
		if (name.Word.empty() || meaning.Word.empty())
			return;
		
		auto defFile = StackFile::FindDef(CodeFilePath, name.Word);
		meaning.Literal = false;
		defFile << meaning;

		return;
	}
	if (word.Word == Words::Label) {
		auto name = StackFile.PopWord();
		if (name.Word.empty())
			return;
		
		auto labelFile = StackFile::FindLabel(CodeFilePath, name.Word);
		labelFile << word;

		return;
	}
}

bool Interpreter::Finished() const {
	return CodeFile.Size() == 0;
}

bool Interpreter::HasOption(Args opt) const {
	return (Options & opt) != 0;
}
