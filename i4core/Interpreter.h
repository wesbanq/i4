#pragma once

#include "ReturnCode.h"
#include "StackFile.h"
#include "IFileSystem.h"
#include "Option.h"
#include <iostream>
#include <string>
#include <vector>

class Interpreter {
private:
	std::ostream& OutputStream;
	unsigned char Options;
	const IFileSystem& Fs;
	std::filesystem::path WorkDir;
	std::filesystem::path CodeFilePath;
	StackFile CodeFile;
	StackFile DataFile;
public:
	Interpreter(const IFileSystem& fs, std::filesystem::path mainFile, std::ostream& outputStream,
	            unsigned char options);

	ReturnCode Run(std::vector<std::string> args);
	void PushArgs(const std::vector<std::string>& args);
	ReturnCode PopResult();
	void Step();
	bool Finished() const;

	bool HasOption(Option opt) const;
	static bool HasOption(unsigned char options, Option opt);
};
