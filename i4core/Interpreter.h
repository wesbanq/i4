#pragma once

#include "StackFile.h"
#include "IRunner.h"
#include "Option.h"
#include <iostream>
#include <string>
#include <vector>

class Interpreter {
private:
	std::ostream& OutputStream;
	unsigned char Options;
	const IRunner& Fs;
	std::filesystem::path WorkDir;
	std::filesystem::path CodeFilePath;
	StackFile CodeFile;
	StackFile StackFile;
public:
	Interpreter(const IRunner& fs, std::filesystem::path mainFile, std::ostream& outputStream,
	            unsigned char options);

	std::string Run(std::vector<std::string> args);
	void PushProgramArgs(const std::vector<std::string>& args);
	std::string PopFinalResult();
	void Step();
	bool Finished() const;

	bool HasOption(Option opt) const;
	static bool HasOption(unsigned char options, Option opt);
};
