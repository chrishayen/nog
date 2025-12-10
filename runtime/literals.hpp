/**
 * @file literals.hpp
 * @brief C++ code generation for Nog literals.
 *
 * Helper functions that emit C++ code for string, number, float,
 * bool, and none literals.
 */

#pragma once
#include <string>
#include <fmt/format.h>

using namespace std;

namespace nog::runtime {

/** Emits a C++ string literal: "value" */
inline string string_literal(const string& value) {
    return fmt::format("\"{}\"", value);
}

/** Emits a C++ integer literal */
inline string number_literal(const string& value) {
    return value;
}

/** Emits a C++ float literal */
inline string float_literal(const string& value) {
    return value;
}

/** Emits a C++ bool literal: true or false */
inline string bool_literal(bool value) {
    return value ? "true" : "false";
}

/** Emits std::nullopt for the none literal */
inline string none_literal() {
    return "std::nullopt";
}

}
