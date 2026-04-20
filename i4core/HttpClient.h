#pragma once

#include <optional>
#include <string>
#include <string_view>

std::optional<std::string> HttpRequest(std::string_view method, const std::string& url,
                                       const std::string& payload);
