#pragma once

#include <cstdint>
#include <string_view>
#include <utility>

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

}        // namespace strata::log
