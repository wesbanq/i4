#include "Runner.h"
#include "DebugRunner.h"
#include "Interpreter.h"
#include "Option.h"
#include <exception>
#include <filesystem>
#include <string>
#include <iostream>
#include <cstdlib>
#include <tuple>
#include <string_view>
#include <vector>
#include <array>
#include <utility>

#if __has_include("I4Version.h")
#include "I4Version.h"
#else
#define I4_VERSION_STRING "unknown"
#endif

static void PrintHelp() {
	std::cout <<
		"Usage: i4 [options] <file> [-- <program args>]\n"
		"\n"
		"Options:\n"
		"  -b, -box        Run a copy of the program in a fresh <name>.i4box directory\n"
		"  -m, -memory     Run in an in-memory filesystem; nothing is written to disk\n"
		"  -S, -step       Wait for Enter before executing each instruction\n"
		"  -F, -nofs       Disable open and write\n"
		"  -s, -safe       -nofs, and abort if the .i4 files under the program's directory\n"
		"                  grow past " << IRunner::SafeCodeFilesLimitMb << " MiB\n"
		"  -v, -verbose    Print every token as it runs\n"
		"  -d, -debug      -memory, -step and -safe combined\n"
		"  -h, -help       Show this help\n"
		"  -V, -version    Show version\n";
}

static void PrintVersion() {
	std::cout << "i4 interpreter version: " << I4_VERSION_STRING << "\n";
}

using FlagPair = std::pair<std::string_view, Option>;
static constexpr std::array<FlagPair, 14> StringToArgs = {
	FlagPair{ "-verbose", Option::VERBOSE },
	FlagPair{ "-v", Option::VERBOSE },
	FlagPair{ "-safe", Option::SAFE },
	FlagPair{ "-s", Option::SAFE },
	FlagPair{ "-box", Option::BOX },
	FlagPair{ "-b", Option::BOX },
	FlagPair{ "-debug", Option::DEBUG },
	FlagPair{ "-d", Option::DEBUG },
	FlagPair{ "-nofs", Option::NOFS },
	FlagPair{ "-F", Option::NOFS },
	FlagPair{ "-step", Option::STEP },
	FlagPair{ "-S", Option::STEP },
	FlagPair{ "-memory", Option::MEMORY },
	FlagPair{ "-m", Option::MEMORY },
};

static bool MapFlag(std::string_view arg, unsigned char& outBits) {
	for (const auto& [sv, flag] : StringToArgs) {
		if (sv.empty()) {
			continue;
		}
		if (sv == arg) {
			outBits = static_cast<unsigned char>(flag);
			return true;
		}
	}
	return false;
}

static std::tuple<std::vector<std::string>, unsigned char, std::vector<std::string>> FormatArgs(
	int argc, char* argv[]) {
	auto filenames = std::vector<std::string>();
	unsigned char options = 0;
	auto programArgs = std::vector<std::string>();
	bool afterDoubleDash = false;

	for (int i = 1; i < argc; ++i) {
		const char* a = argv[i];
		if (afterDoubleDash) {
			programArgs.emplace_back(a);
			continue;
		}
		if (std::string_view(a) == "--") {
			afterDoubleDash = true;
			continue;
		}
		if (a[0] == '-') {
			if (std::string_view(a) == "-h" || std::string_view(a) == "-help") {
				PrintHelp();
				std::exit(0);
			}
			if (std::string_view(a) == "-V" || std::string_view(a) == "-version") {
				PrintVersion();
				std::exit(0);
			}

			unsigned char bits = 0;
			if (!MapFlag(a, bits)) {
				std::cerr << "Unknown option: \"" << a << "\".\n";
				std::exit(1);
			}
			options |= bits;
		} else {
			if (std::filesystem::exists(a)) {
				filenames.push_back(a);
			} else {
				std::cerr << "Could not locate file: \"" << a << "\".\n";
				std::exit(1);
			}
		}
	}
	return {filenames, options, std::move(programArgs)};
}

int main(int argc, char* argv[]) {
	auto [filenames, options, programArgs] = FormatArgs(argc, argv);

	if (filenames.empty()) {
		std::cerr << "No files provided.\n";
		return 1;
	}

	ReturnCode result;
	try {
		if (Interpreter::HasOption(options, Option::MEMORY)) {
			DebugRunner debugRunner;
			debugRunner.Preload(std::filesystem::current_path());
			result = debugRunner.Start(filenames[0], options, std::cout, programArgs);
		} else {
			Runner runner;
			result = runner.Start(filenames[0], options, std::cout, programArgs);
		}
		std::cout << "\nProgram finished with code: " << result.first << std::endl;
	} catch (std::exception& e) {
		std::cerr << "Error: " << e.what() << std::endl;
		return 2;
	}
	
	return result.second;
}