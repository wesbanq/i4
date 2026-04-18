#include <fstream>
#include <string>
#include <iostream>
#include <filesystem>
#include <array>
#include <optional>
#include <tuple>
#include <vector>
#include "Interpreter.h"

using FlagPair = std::pair<std::string_view, Interpreter::Args>;
static constexpr std::array<FlagPair, 7> StringToArgs = {{
	FlagPair{ "-verbose", Interpreter::Args::VERBOSE },
	FlagPair{ "-safe", Interpreter::Args::SAFE },
	FlagPair{ "-box", Interpreter::Args::BOX },
	FlagPair{ "-nofs", Interpreter::Args::NOFS },
	FlagPair{ "-noweb", Interpreter::Args::NOWEB },
	FlagPair{ "-help", Interpreter::Args::HELP },
	FlagPair{ "-version", Interpreter::Args::VERSION },
}};

static bool MapFlag(std::string_view arg, unsigned char& outBits) {
	if (arg == "-no-ext") {
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
				exit(1);
			}
			options |= bits;
		}
		else {
			if (std::filesystem::exists(a)) {
				filenames.push_back(a);
			}
			else {
				std::cerr << "Could not locate file: \"" << a << "\".\n";
				exit(1);
			}
		}
	}
	return {filenames, options};
}

int main(int argc, char* argv[]) {
	auto t_ = FormatArgs(argc, argv);
	auto& filenames = std::get<0>(t_);
	auto options = std::get<1>(t_);

	std::istream* in = &std::cin;
	std::ostream* out = &std::cout;

	std::optional<std::ifstream> file;
	if (!filenames.empty()) {
		file.emplace(filenames[0]);
		if (!file->is_open()) {
			std::cerr << "Could not open file: \"" << filenames[0] << "\".\n";
			return 1;
		}
		in = &*file;
	}

	auto interpreter = Interpreter(*in, *out, options);
	return interpreter.Run(0, {});
}
