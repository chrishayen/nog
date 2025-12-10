/**
 * @file expressions.hpp
 * @brief C++ code generation for Nog expressions.
 *
 * Helper functions for emitting variable references, binary operations,
 * function calls, method calls, and "is none" checks.
 */

#pragma once
#include <string>
#include <vector>
#include <fmt/format.h>
#include <fmt/ranges.h>

using namespace std;

namespace nog::runtime {

/** Emits a variable reference (just the name) */
inline string variable_ref(const string& name) {
    return name;
}

/** Emits a binary expression: left op right */
inline string binary_expr(const string& left, const string& op, const string& right) {
    return fmt::format("{} {} {}", left, op, right);
}

/** Emits a function call: name(arg1, arg2, ...) */
inline string function_call(const string& name, const vector<string>& args) {
    return fmt::format("{}({})", name, fmt::join(args, ", "));
}

/** Emits an "is none" check using std::optional::has_value() */
inline string is_none(const string& value) {
    return fmt::format("!{}.has_value()", value);
}

/** Emits a method call: object.method(arg1, arg2, ...) */
inline string method_call(const string& object, const string& method, const vector<string>& args) {
    return fmt::format("{}.{}({})", object, method, fmt::join(args, ", "));
}

}
