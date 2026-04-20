#include "Runner.h"
#include "DebugRunner.h"
#include "Interpreter.h"
#include "Option.h"
#include <filesystem>
#include <string>
#include <iostream>
#include <cstdlib>
#include <tuple>
#include <string_view>
#include <vector>
#include <array>

#if __has_include("I4Version.h")
#include "I4Version.h"
#else
#define I4_VERSION_STRING "unknown"
#endif

static void PrintHelp() {
	std::cout <<
		"Usage: i4 [options] [file ...]\n"
		"\n"
		"Options:\n"
		"  -v, -verbose    Verbose output\n"
		"  -s, -safe       Safe mode\n"
		"  -b, -box        Box mode\n"
		"  -F, -nofs       Disable filesystem features\n"
		"  -W, -noweb      Disable web features\n"
		"  -x, -no-ext     Same as -nofs and -noweb\n"
		"  -h, -help       Show this help\n"
		"  -V, -version    Show version\n"
		"  -d, -debug      Debug mode\n";
}

static void PrintVersion() {
	std::cout << "i4 interpreter version: " << I4_VERSION_STRING << "\n";
}

using FlagPair = std::pair<std::string_view, Option>;
static constexpr std::array<FlagPair, 18> StringToArgs = {
	FlagPair{ "-verbose", Option::VERBOSE },
	FlagPair{ "-v", Option::VERBOSE },
	FlagPair{ "-safe", Option::SAFE },
	FlagPair{ "-s", Option::SAFE },
	FlagPair{ "-box", Option::BOX },
	FlagPair{ "-b", Option::BOX },
	FlagPair{ "-debug", Option::DEBUG },
	FlagPair{ "-d", Option::DEBUG },
	FlagPair{ "-no-ext", Option::NOEXT },
	FlagPair{ "-x", Option::NOEXT },
	FlagPair{ "-nofs", Option::NOFS },
	FlagPair{ "-F", Option::NOFS },
	FlagPair{ "-noweb", Option::NOWEB },
	FlagPair{ "-W", Option::NOWEB },
	FlagPair{ "-help", Option::HELP },
	FlagPair{ "-h", Option::HELP },
	FlagPair{ "-version", Option::VERSION },
	FlagPair{ "-V", Option::VERSION },
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

static std::tuple<std::vector<std::string>, unsigned char> FormatArgs(int argc, char* argv[],
                                                                      const IRunner& fs) {
	auto filenames = std::vector<std::string>();
	unsigned char options = 0;

	for (int i = 1; i < argc; ++i) {
		const char* a = argv[i];
		if (a[0] == '-') {
			unsigned char bits = 0;
			if (!MapFlag(a, bits)) {
				std::cerr << "Unknown option: \"" << a << "\".\n";
				std::exit(1);
			}
			options |= bits;
			if ((bits & static_cast<unsigned char>(Option::HELP)) != 0) {
				PrintHelp();
				std::exit(0);
			}
			if ((bits & static_cast<unsigned char>(Option::VERSION)) != 0) {
				PrintVersion();
				std::exit(0);
			}
		}
		else {
			if (fs.exists(a)) {
				filenames.push_back(a);
			}
			else {
				std::cerr << "Could not locate file: \"" << a << "\".\n";
				std::exit(1);
			}
		}
	}
	return {filenames, options};
}

int main(int argc, char* argv[]) {
	Runner runner;
	auto t_ = FormatArgs(argc, argv, runner);
	auto& filenames = std::get<0>(t_);
	auto options = std::get<1>(t_);

	if (filenames.empty()) {
		std::cerr << "No files provided.\n";
		return 1;
	}

	std::vector<std::string> argvec(argv, argv + argc);
	std::string result;
	if (Interpreter::HasOption(options, Option::DEBUG)) {
		DebugRunner debugRunner = DebugRunner::fromFilesystemAround(filenames[0]);
		std::error_code ec;
		const std::filesystem::path cwd = std::filesystem::current_path();
		const std::filesystem::path mainAbs = std::filesystem::weakly_canonical(filenames[0]);
		std::filesystem::path mainPath = std::filesystem::relative(mainAbs, cwd, ec);
		if (ec)
			mainPath = std::filesystem::path(filenames[0]);
		result = debugRunner.Start(mainPath, options, std::cout, argvec);
	} else {
		result = runner.Start(filenames[0], options, std::cout, argvec);
	}
	std::cout << "\nProgram finished with code: " << result << std::endl;
	
	return result == "OK" ? 0 : 1;
}