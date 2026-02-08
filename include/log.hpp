#pragma once

#include <cstdint>
#include <string_view>
#include <utility>
#include <string>
#include <chrono>
#include <source_location>
#include <thread>

namespace strata::log
{

enum class Level : std::uint8_t
{
	trace = 0,
	debug,
	info,
	warn,
	error,
	fatal,
	off
};

[[nodiscard]] constexpr std::string_view to_string(Level level) noexcept
{
	if (std::to_underlying(level) > std::to_underlying(Level::off))
	{
		return "UNKNOWN";
	}

	switch (level)
	{
		case Level::trace:
			return "TRACE";
		case Level::debug:
			return "DEBUG";
		case Level::info:
			return "INFO";
		case Level::warn:
			return "WARN";
		case Level::error:
			return "ERROR";
		case Level::fatal:
			return "FATAL";
		case Level::off:
			return "OFF";
	}

	std::unreachable();
}

struct LogRecord
{
    Level level{};
    std::string_view category{};
    std::string message{};
    std::source_location location{};
    std::chrono::system_clock::time_point timestamp{};
    std::thread::id thread_id{};
};

}        // namespace strata::log
