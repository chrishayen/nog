/**
 * @file io.hpp
 * @brief C++ code generation for Nog I/O and test assertions.
 *
 * Helper functions for emitting print() statements and assert_eq() test calls.
 */

#pragma once
#include <string>
#include <vector>
#include <fmt/format.h>
#include <fmt/ranges.h>

using namespace std;

namespace nog::runtime {

/** Emits std::cout for a single value */
inline string print(const string& arg) {
    return fmt::format("std::cout << {} << std::endl", arg);
}

/** Emits std::cout for multiple values */
inline string print_multi(const vector<string>& args) {
    return fmt::format("std::cout << {} << std::endl", fmt::join(args, " << "));
}

/** Emits a call to the _assert_eq test helper with line number for errors */
inline string assert_eq(const string& a, const string& b, int line) {
    return fmt::format("_assert_eq({}, {}, {})", a, b, line);
}

}
