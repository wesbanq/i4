#pragma once

#include "ReturnCode.h"
#include "StackFile.h"
#include "IFileSystem.h"
#include "Option.h"
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <map>
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
	/// With -safe: the size of each of the program's definition and label files, by file name,
	/// and their total. Only def and : change these files, so they're never rescanned.
	std::map<std::string, std::uintmax_t> NamedFileSizes;
	std::uintmax_t NamedFilesSize = 0;

	void TrackNamedFile(const StackFile& file);
public:
	Interpreter(const IFileSystem& fs, std::filesystem::path mainFile, std::ostream& outputStream,
	            unsigned char options);

	ReturnCode Run(std::vector<std::string> args);
	void PushArgs(const std::vector<std::string>& args);
	ReturnCode PopResult();
	void Step();
	bool Finished() const;
	/// With -safe: the total size of the program's own files (code, data stack, definitions and labels).
	std::uintmax_t FilesSize() const;

	bool HasOption(Option opt) const;
	static bool HasOption(unsigned char options, Option opt);
};
