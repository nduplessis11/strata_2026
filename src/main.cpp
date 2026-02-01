#include <format>
#include <print>
#include <string>

#include "error.hpp"
#include "log.hpp"

using namespace strata;

int main()
{
	int calls = 0;
	try
	{
		std::string_view log_level{log::to_string(log::Level::info)};
		std::println("{}", log_level);

		auto pred = [&] {
			++calls;
			return true;
		};
		auto msg = [&] {
			++calls;
			return std::string("msg");
		};

		err::debug_expect(pred, msg);
		// In debug: calls == 1 (predicate ran, msg didn’t)
		// In release: calls == 0
	}
	catch (const std::exception &e)
	{
		std::println(stderr, "Caught: {}", e.what());
	}

	std::println("Successful exit. Calls: {}", calls);
}
