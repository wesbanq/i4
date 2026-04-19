#include <string>
#include <iostream>
#include <filesystem>
#include <array>
#include <cstdlib>
#include <tuple>
#include <string_view>
#include <vector>
#include "Interpreter.h"

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
		"  -V, -version    Show version\n";
}

static void PrintVersion() {
	std::cout << "i4 interpreter version: " << I4_VERSION_STRING << "\n";
}

using FlagPair = std::pair<std::string_view, Interpreter::Args>;
static constexpr std::array<FlagPair, 14> StringToArgs = {{
	FlagPair{ "-verbose", Interpreter::Args::VERBOSE },
	FlagPair{ "-v", Interpreter::Args::VERBOSE },
	FlagPair{ "-safe", Interpreter::Args::SAFE },
	FlagPair{ "-s", Interpreter::Args::SAFE },
	FlagPair{ "-box", Interpreter::Args::BOX },
	FlagPair{ "-b", Interpreter::Args::BOX },
	FlagPair{ "-nofs", Interpreter::Args::NOFS },
	FlagPair{ "-F", Interpreter::Args::NOFS },
	FlagPair{ "-noweb", Interpreter::Args::NOWEB },
	FlagPair{ "-W", Interpreter::Args::NOWEB },
	FlagPair{ "-help", Interpreter::Args::HELP },
	FlagPair{ "-h", Interpreter::Args::HELP },
	FlagPair{ "-version", Interpreter::Args::VERSION },
	FlagPair{ "-V", Interpreter::Args::VERSION },
}};

static bool MapFlag(std::string_view arg, unsigned char& outBits) {
	if (arg == "-no-ext" || arg == "-x") {
		outBits = static_cast<unsigned char>(Interpreter::Args::NOFS | Interpreter::Args::NOWEB);
		return true;
	}
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

static std::tuple<std::vector<std::string>, unsigned char> FormatArgs(int argc, char* argv[]) {
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
			if ((bits & static_cast<unsigned char>(Interpreter::Args::HELP)) != 0) {
				PrintHelp();
				std::exit(0);
			}
			if ((bits & static_cast<unsigned char>(Interpreter::Args::VERSION)) != 0) {
				PrintVersion();
				std::exit(0);
			}
		}
		else {
			if (std::filesystem::exists(a)) {
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
	auto t_ = FormatArgs(argc, argv);
	auto& filenames = std::get<0>(t_);
	auto options = std::get<1>(t_);

	if (filenames.empty()) {
		std::cerr << "No files provided.\n";
		return 1;
	}

	std::vector<std::string> argvec(argv, argv + argc);
	auto interpreter = Interpreter(
		filenames[0], 
		std::cout, 
		options
	);
	auto result = interpreter.Run(argvec);
	std::cout << "Program finished with code: " << result << std::endl;
	
	return result == "OK" ? 0 : 1;
}