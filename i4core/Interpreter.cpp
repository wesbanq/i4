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
	while (!Finished())
		Step();

	auto word = StackFile.PopWord();
	if (word.Word.empty())
		return "OK";
	
	return word.Word;
}

void Interpreter::Step() {
	auto lastWord = CodeFile.PopWord();
	if (lastWord.Word.empty())
		return;
	
	if (lastWord.Word == Words::Def) {
		auto name = CodeFile.PopWord();
		if (name.Word.empty())
			return;
		
		CodeFile << StackFile::FindDef(CodeFilePath, name.Word);
	}
}

bool Interpreter::Finished() const {
	return CodeFile.Size() == 0;
}

bool Interpreter::HasOption(Args opt) const {
	return (Options & opt) != 0;
}
