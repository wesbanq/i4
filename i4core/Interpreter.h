#pragma once

#include "StackWord.h"
#include "StackFile.h"
#include <iostream>
#include <string>
#include <vector>
#include <filesystem>

class Interpreter {
private:
	std::ostream& OutputStream;
	unsigned char Options;
	std::filesystem::path WorkDir;
	std::filesystem::path CodeFilePath;
	StackFile CodeFile;
	StackFile StackFile;
public:
	enum Args : unsigned char { 
		NONE	= 0, 
		VERBOSE	= 1 << 0,
		SAFE	= 1 << 1,
		BOX		= 1 << 2,
		NOFS	= 1 << 3,
		NOWEB	= 1 << 4,
		HELP	= 1 << 5,
		VERSION	= 1 << 6,
	};

	Interpreter(std::filesystem::path mainFile, std::ostream& outputStream, unsigned char options);

	std::string Run(std::vector<std::string> args);
	void Step();
	bool Finished() const;

	bool HasOption(Args opt) const;
};
