#pragma once

#include <concepts>
#include <cstdio>
#include <exception>
#include <format>
#include <iostream>
#include <print>
#include <source_location>
#include <stacktrace>
#include <stdexcept>
#include <string_view>
#include <type_traits>
#include <utility>

/**
 * @defgroup err_assert Assertions & Contracts
 *
 * @brief Runtime assertion, contract, and diagnostic helpers.
 *
 * This module provides a small set of assertion-style utilities with
 * clearly separated failure semantics:
 *
 * - **expect(...)**
 *   - Indicates a violated *invariant*
 *   - Prints diagnostics + stack trace
 *   - Terminates the process
 *
 * - **ensure(...)**
 *   - Indicates a *recoverable contract failure*
 *   - Throws `std::runtime_error`
 *
 * - **debug_expect(...)**
 *   - Debug-only invariant checking
 *   - Fully compiled out in release builds
 *
 * All formatted variants provide compile-time format-string checking
 * and capture the call-site source location automatically.
 */

namespace strata::err::detail
{

/**
 * @brief Concept for types convertible to std::string_view, excluding raw
 * arrays.
 *
 * This concept matches types like `const char*`, `std::string_view`, and
 * `std::string`, but rejects array types (e.g., `char[N]`) so that string
 * literals bind to the format-string overloads instead.
 *
 * @tparam T The type to check.
 */
template <class T>
concept string_viewable = std::constructible_from<std::string_view, T> &&
                          (!std::is_array_v<std::remove_reference_t<T>>);

/**
 * @brief Wrapper that couples a compile-time checked format string with a
 * call-site source location.
 *
 * This type enables call patterns like:
 * `expect(false, "x = {}", x);`
 *
 * by:
 * - validating the format string against `Args...` at compile time via
 *   `std::basic_format_string<char, Args...>`, and
 * - capturing the caller's location through a defaulted `std::source_location`.
 *
 * The constructor is `consteval`, so the format string must be a compile-time
 * string (typically a string literal).
 *
 * @tparam Args Format argument types for the format string.
 */
template <class... Args>
struct fmt_with_loc_impl
{
	/// Compile-time checked format string.
	std::basic_format_string<char, std::type_identity_t<Args>...> fmt;

	/// Captured call-site location (file, line, column, function).
	std::source_location loc;

	/**
	 * @brief Construct from a string literal and capture the call-site location
	 * by default.
	 *
	 * @param s String literal used as the format string.
	 * @param l Call-site location (defaults to
	 * `std::source_location::current()`).
	 */
	template <size_t N>
	consteval fmt_with_loc_impl(
	    char const (&s)[N],
	    std::source_location l = std::source_location::current()) :
	    fmt(s), loc(l)
	{}
};

/**
 * @brief Public alias for the format+location wrapper.
 *
 * This alias normalizes the `Args...` list through `std::type_identity_t` to
 * avoid surprising deductions caused by references/cv-qualifiers.
 *
 * @tparam Args Format argument types.
 */
template <class... Args>
using fmt_with_loc = fmt_with_loc_impl<std::type_identity_t<Args>...>;
}        // namespace strata::err::detail

// clang-format off
/**
 * @ingroup err_assert
 *
 * @brief Assertion and contract utilities.
 *
 * ## Failure Semantics
 *
 * This namespace distinguishes between *invariants* and *recoverable errors*:
 *
 * | Function        | On failure                                 | Intended use                       |
 * |-----------------|--------------------------------------------|------------------------------------|
 * | `expect`        | Print diagnostics + terminate              | Invariants / programmer errors     |
 * | `ensure`        | Throw `std::runtime_error`                 | Recoverable contract violations    |
 * | `debug_expect`  | Same as `expect`, debug-only               | Expensive or debug-only invariants |
 *
 * ## Design Notes
 *
 * - `expect(...)` **never returns on failure** and is suitable for conditions
 *   that indicate undefined or invalid program state.
 *
 * - `ensure(...)` reports failure by throwing, allowing callers to recover
 *   or propagate the error.
 *
 * - `debug_expect(...)` is completely compiled out when `NDEBUG` is defined;
 *   neither predicates nor messages are evaluated in release builds.
 *
 * - Formatted overloads provide **compile-time format checking** and capture
 *   the **call-site source location** automatically using `std::source_location`.
 */
// clang-format on
namespace strata::err
{

/**
 * @ingroup err_assert
 *
 * @brief Prints a stack trace (best-effort) and terminates the program.
 *
 * Writes the current stack trace to `std::cerr` and then unconditionally
 * terminates the process via `std::terminate()`.
 *
 * The stack trace is captured with `std::stacktrace::current(skip)`, which
 * skips the top @p skip frames to keep the output focused on the caller path.
 *
 * Any exception thrown while streaming the stack trace is swallowed.
 *
 * @param skip Number of stack frames to skip (default: 2, skips fail_expect and
 * its caller).
 *
 * @note This function never returns.
 */
[[noreturn]] inline void fail_expect(int skip = 2) noexcept
{
	try
	{
		std::cerr << std::stacktrace::current(skip) << '\n' << std::flush;
	}
	catch (...)
	{}
	std::terminate();
}

/**
 * @ingroup err_assert
 *
 * @brief Asserts that a predicate is true (plain-text message overload).
 *
 * If @p predicate is false, prints the call-site location (file:line:column
 * function) followed by @p msg (as plain text) to `stderr`, then terminates via
 * fail_expect().
 *
 * This overload accepts any message type constructible as `std::string_view`
 * (for example `const char*`, `std::string_view`, or `std::string`) while
 * rejecting array types (so string literals do not bind as arrays).
 *
 * @tparam Msg      Message type satisfying `detail::string_viewable`.
 * @param predicate Condition that must be true.
 * @param msg       Plain-text message printed if the predicate is false.
 * @param loc       Call-site location (defaults to
 * `std::source_location::current()`).
 *
 * @note This function terminates the program on failure.
 */
template <detail::string_viewable Msg>
inline void
    expect(bool predicate, Msg &&msg,
           std::source_location loc = std::source_location::current()) noexcept
{
	if (predicate) [[likely]]
		return;

	try
	{
		std::println(stderr, "{}:{}:{} {} -- {}", loc.file_name(), loc.line(),
		             loc.column(), loc.function_name(),
		             std::string_view{std::forward<Msg>(msg)});
		std::fflush(stderr);
	}
	catch (...)
	{}

	fail_expect();
}

/**
 * @ingroup err_assert
 *
 * @brief Asserts that a predicate is true (formatted message overload).
 *
 * If @p predicate is false, prints the call-site location captured inside @p f,
 * then prints a formatted message to `stderr` using:
 * `std::println(stderr, f.fmt, args...)`,
 * and terminates via fail_expect().
 *
 * The format string is compile-time checked by `detail::fmt_with_loc<Args...>`,
 * and the call-site location is captured automatically via the wrapper's
 * defaulted `std::source_location`.
 *
 * @tparam Args     Types of the format arguments.
 * @param predicate Condition that must be true.
 * @param f         Wrapper containing a compile-time checked format string and
 * call-site location.
 * @param args      Arguments forwarded to `std::println`.
 *
 * @note This function terminates the program on failure.
 */
template <class... Args>
inline void expect(bool predicate, detail::fmt_with_loc<Args...> f,
                   Args &&...args) noexcept
{
	if (predicate) [[likely]]
		return;

	try
	{
		std::print(stderr, "{}:{}:{} {} -- ", f.loc.file_name(), f.loc.line(),
		           f.loc.column(), f.loc.function_name());
		std::println(stderr, f.fmt, std::forward<Args>(args)...);
		std::fflush(stderr);
	}
	catch (...)
	{}

	fail_expect();
}

/**
 * @ingroup err_assert
 *
 * @brief Ensures that a predicate is true, otherwise throws (plain-text message
 * overload).
 *
 * If @p predicate is false, throws `std::runtime_error` with a message that
 * includes the call-site location (file:line:column function) followed by @p
 * msg.
 *
 * This overload accepts any message type constructible as `std::string_view`
 * (for example `const char*`, `std::string_view`, or `std::string`) while
 * rejecting array types.
 *
 * Unlike expect(), ensure() reports failure by throwing rather than
 * terminating.
 *
 * @tparam Msg      Message type satisfying `detail::string_viewable`.
 * @param predicate Condition that must be true.
 * @param msg       Plain-text message used to construct the exception.
 * @param loc       Call-site location (defaults to
 * `std::source_location::current()`).
 *
 * @throws std::runtime_error if @p predicate is false.
 */
template <detail::string_viewable Msg>
inline void ensure(bool predicate, Msg &&msg,
                   std::source_location loc = std::source_location::current())
{
	if (predicate) [[likely]]
		return;

	throw std::runtime_error(std::format(
	    "{}:{}:{} {} -- {}", loc.file_name(), loc.line(), loc.column(),
	    loc.function_name(), std::string_view{std::forward<Msg>(msg)}));
}

/**
 * @ingroup err_assert
 *
 * @brief Ensures that a predicate is true, otherwise throws (formatted message
 * overload).
 *
 * If @p predicate is false, formats a message with `std::format(f.fmt,
 * args...)` and throws `std::runtime_error` containing the call-site location
 * followed by the formatted message.
 *
 * The format string is compile-time checked by `detail::fmt_with_loc<Args...>`,
 * and the call-site location is captured automatically via the wrapper's
 * defaulted `std::source_location`.
 *
 * @tparam Args     Types of the format arguments.
 * @param predicate Condition that must be true.
 * @param f         Wrapper containing a compile-time checked format string and
 * call-site location.
 * @param args      Arguments forwarded to `std::format`.
 *
 * @throws std::runtime_error if @p predicate is false.
 */
template <class... Args>
inline void ensure(bool predicate, detail::fmt_with_loc<Args...> f,
                   Args &&...args)
{
	if (predicate) [[likely]]
		return;

	throw std::runtime_error(
	    std::format("{}:{}:{} {} -- {}", f.loc.file_name(), f.loc.line(),
	                f.loc.column(), f.loc.function_name(),
	                std::format(f.fmt, std::forward<Args>(args)...)));
}

#ifndef NDEBUG

/**
 * @ingroup err_assert
 *
 * @brief Debug-only assertion with lazy predicate and lazy message.
 *
 * In debug builds (when `NDEBUG` is not defined), evaluates @p predicate by
 * invoking it. If it returns `false`, evaluates @p msg (only on failure) to
 * produce a message that is convertible to `std::string_view` (see
 * `detail::string_viewable`), then reports the failure via the plain-text
 * `expect(false, msg(), loc)` overload (terminating).
 *
 * This allows @p msg to return `const char*`, `std::string_view`,
 * `std::string`, etc., without requiring an owning `std::string`.
 *
 * In release builds (when `NDEBUG` is defined), this overload is a no-op and
 * neither callable is invoked.
 *
 * @tparam Predicate Callable type returning `bool`.
 * @tparam Message   Callable type returning a `detail::string_viewable`
 * message.
 * @param predicate  Callable evaluated to check the condition.
 * @param msg        Callable evaluated only on failure to produce a message.
 * @param loc        Call-site location (defaults to
 * `std::source_location::current()`).
 *
 * @note Terminates the program on failure (debug builds only).
 */
template <std::invocable Predicate, std::invocable Message>
    requires std::same_as<std::invoke_result_t<Predicate>, bool> &&
             detail::string_viewable<std::invoke_result_t<Message>>
inline void
    debug_expect(Predicate &&predicate, Message &&msg,
                 std::source_location loc = std::source_location::current())
{
	if (std::forward<Predicate>(predicate)()) [[likely]]
		return;
	expect(false, std::forward<Message>(msg)(), loc);
}

/**
 * @ingroup err_assert
 *
 * @brief Debug-only assertion with lazy predicate and eager plain-text message.
 *
 * In debug builds (when `NDEBUG` is not defined), evaluates @p predicate by
 * invoking it. If it returns `false`, reports failure via the plain-text
 * `expect(false, msg, loc)` overload (terminating).
 *
 * In release builds (when `NDEBUG` is defined), this overload is a no-op and
 * the predicate/message are not evaluated.
 *
 * @tparam Predicate Callable type returning `bool`.
 * @tparam Msg       Message type satisfying `detail::string_viewable`.
 * @param predicate  Callable evaluated to check the condition.
 * @param msg        Plain-text message printed if the predicate fails.
 * @param loc        Call-site location (defaults to
 * `std::source_location::current()`).
 *
 * @note Terminates the program on failure (debug builds only).
 */
template <std::invocable Predicate, detail::string_viewable Msg>
    requires std::same_as<std::invoke_result_t<Predicate>, bool>
inline void
    debug_expect(Predicate &&predicate, Msg &&msg,
                 std::source_location loc = std::source_location::current())
{
	if (std::forward<Predicate>(predicate)()) [[likely]]
		return;
	expect(false, std::forward<Msg>(msg), loc);
}

/**
 * @ingroup err_assert
 *
 * @brief Debug-only assertion with lazy predicate and an eager formatted
 * message (no args).
 *
 * In debug builds (when `NDEBUG` is not defined), evaluates @p predicate by
 * invoking it. If it returns `false`, reports failure via the formatted
 * `expect(false, fmt)` overload and terminates.
 *
 * This overload exists to support calls like:
 * `debug_expect([&]{ return ok(); }, "failed");`
 *
 * where the format string has **no format arguments**. The call-site source
 * location is captured automatically by `detail::fmt_with_loc<>`.
 *
 * In release builds, this overload is replaced with a no-op implementation and
 * neither the predicate nor the format wrapper are evaluated.
 *
 * @tparam Predicate Callable type returning `bool`.
 * @param predicate  Callable evaluated to check the condition.
 * @param fmt        Wrapper containing a compile-time checked format string (no
 * arguments) and the call-site source location.
 *
 * @note Terminates the program on failure (debug builds only).
 */
template <std::invocable Predicate>
    requires std::same_as<std::invoke_result_t<Predicate>, bool>
inline void debug_expect(Predicate            &&predicate,
                         detail::fmt_with_loc<> fmt) noexcept
{
	if (std::forward<Predicate>(predicate)()) [[likely]]
		return;
	expect(false, fmt);
}

#else

/**
 * @ingroup err_assert
 *
 * @brief Debug-only assertion with lazy predicate and lazy message (release
 * no-op).
 *
 * This is the release-build implementation (when `NDEBUG` is defined). It
 * performs no checks and does not evaluate either callable.
 *
 * The `Message` callable is permitted to return any type satisfying
 * `detail::string_viewable`, but it is not invoked in release builds.
 *
 * @tparam Predicate Callable type returning `bool`.
 * @tparam Message   Callable type returning a `detail::string_viewable`
 * message.
 * @param predicate  Unused.
 * @param msg        Unused.
 */
template <std::invocable Predicate, std::invocable Message>
    requires std::same_as<std::invoke_result_t<Predicate>, bool> &&
             detail::string_viewable<std::invoke_result_t<Message>>
inline void debug_expect([[maybe_unused]] Predicate &&predicate,
                         [[maybe_unused]] Message   &&msg)
{
	// Do nothing
}

/**
 * @ingroup err_assert
 *
 * @brief Debug-only assertion with lazy predicate and eager plain-text message
 * (release no-op).
 *
 * This is the release-build implementation (when `NDEBUG` is defined). It does
 * nothing, and the provided predicate/message are not evaluated.
 *
 * @tparam Predicate Callable type returning `bool`.
 * @tparam Msg       Message type satisfying `detail::string_viewable`.
 * @param predicate  Unused.
 * @param msg        Unused.
 */
template <std::invocable Predicate, detail::string_viewable Msg>
    requires std::same_as<std::invoke_result_t<Predicate>, bool>
inline void debug_expect([[maybe_unused]] Predicate &&predicate,
                         [[maybe_unused]] Msg       &&msg)
{
	// Do nothing
}

/**
 * @ingroup err_assert
 *
 * @brief Debug-only assertion with lazy predicate and an eager formatted
 * message (release no-op).
 *
 * This is the release-build implementation (when `NDEBUG` is defined). It
 * performs no checks and does not evaluate the predicate or the format wrapper.
 *
 * @tparam Predicate Callable type returning `bool`.
 * @param predicate  Unused.
 * @param fmt        Unused.
 */
template <std::invocable Predicate>
    requires std::same_as<std::invoke_result_t<Predicate>, bool>
inline void debug_expect([[maybe_unused]] Predicate            &&predicate,
                         [[maybe_unused]] detail::fmt_with_loc<> fmt) noexcept
{
	// Do nothing
}

#endif
}        // namespace strata::err
