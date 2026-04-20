#include "HttpClient.h"

#include <array>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <random>
#include <string>

namespace {

#ifdef _WIN32
constexpr const char* kCurl = "curl.exe";
FILE* pipeOpenRead(const std::string& cmd) {
	return _popen(cmd.c_str(), "rb");
}
int pipeClose(FILE* f) {
	return _pclose(f);
}
#else
constexpr const char* kCurl = "curl";
FILE* pipeOpenRead(const std::string& cmd) {
	return popen(cmd.c_str(), "r");
}
int pipeClose(FILE* f) {
	return pclose(f);
}
#endif

std::filesystem::path makeTempBodyPath() {
	auto dir = std::filesystem::temp_directory_path();
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<unsigned> dist;
	for (int i = 0; i < 16; ++i) {
		auto p = dir / ("i4-http-body-" + std::to_string(dist(gen)) + ".bin");
		if (!std::filesystem::exists(p))
			return p;
	}
	return dir / ("i4-http-body-" + std::to_string(dist(gen)) + ".bin");
}

bool writeBodyFile(const std::filesystem::path& path, const std::string& payload) {
	std::ofstream out(path, std::ios::binary | std::ios::trunc);
	if (!out.good())
		return false;
	out.write(payload.data(), static_cast<std::streamsize>(payload.size()));
	return out.good();
}

std::string quoteForCmd(const std::filesystem::path& path) {
	// Double quotes for Windows cmd; path from filesystem should not contain "
	std::string s = path.string();
	std::string out;
	out.reserve(s.size() + 2);
	out.push_back('"');
	for (char c : s) {
		if (c == '"')
			return {};
		out.push_back(c);
	}
	out.push_back('"');
	return out;
}

std::string quoteUrlForCmd(const std::string& url) {
	std::string out;
	out.reserve(url.size() + 2);
	out.push_back('"');
	for (char c : url) {
		if (c == '"' || c == '\r' || c == '\n')
			return {};
		out.push_back(c);
	}
	out.push_back('"');
	return out;
}

bool methodUsesRequestBody(std::string_view method) {
	return method == "POST" || method == "PUT";
}

} // namespace

std::optional<std::string> HttpRequest(std::string_view method, const std::string& url,
                                       const std::string& payload) {
	if (url.empty())
		return std::nullopt;

	const auto quotedUrl = quoteUrlForCmd(url);
	if (quotedUrl.empty())
		return std::nullopt;

	std::string cmd;
	cmd.reserve(128 + url.size());
	cmd += kCurl;
	cmd += " -sS -g -X ";
	cmd += method;
	cmd += ' ';
	cmd += quotedUrl;

	std::filesystem::path bodyFile;
	if (methodUsesRequestBody(method)) {
		bodyFile = makeTempBodyPath();
		if (!writeBodyFile(bodyFile, payload))
			return std::nullopt;
		const auto qpath = quoteForCmd(bodyFile);
		if (qpath.empty()) {
			std::error_code ec;
			std::filesystem::remove(bodyFile, ec);
			return std::nullopt;
		}
		cmd += " --data-binary @";
		cmd += qpath;
	} else if (method == "DELETE" || method == "OPTIONS") {
		// Optional body: curl sends DELETE/OPTIONS body only if we pass --data-binary
		if (!payload.empty()) {
			bodyFile = makeTempBodyPath();
			if (!writeBodyFile(bodyFile, payload))
				return std::nullopt;
			const auto qpath = quoteForCmd(bodyFile);
			if (qpath.empty()) {
				std::error_code ec;
				std::filesystem::remove(bodyFile, ec);
				return std::nullopt;
			}
			cmd += " --data-binary @";
			cmd += qpath;
		}
	}
	// GET: no body flags; payload ignored

	FILE* pipe = pipeOpenRead(cmd);
	if (!pipe) {
		std::error_code ec;
		if (!bodyFile.empty())
			std::filesystem::remove(bodyFile, ec);
		return std::nullopt;
	}

	std::string out;
	std::array<char, 4096> buf{};
	while (true) {
		const size_t n = std::fread(buf.data(), 1, buf.size(), pipe);
		if (n == 0)
			break;
		out.append(buf.data(), n);
	}
	const int st = pipeClose(pipe);

	std::error_code ec;
	if (!bodyFile.empty())
		std::filesystem::remove(bodyFile, ec);

	if (st != 0)
		return std::nullopt;

	return out;
}
