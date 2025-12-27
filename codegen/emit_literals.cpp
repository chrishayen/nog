/**
 * @file emit_literals.cpp
 * @brief Literal emission for the Bishop code generator.
 */

#include "codegen.hpp"
#include <fmt/format.h>

using namespace std;

namespace codegen {

/**
 * Emits a C++ string literal wrapped in std::string constructor.
 */
string string_literal(const string& value) {
    return fmt::format("std::string(\"{}\")", value);
}

/**
 * Emits a C++ integer literal.
 */
string number_literal(const string& value) {
    return value;
}

/**
 * Emits a C++ float literal.
 */
string float_literal(const string& value) {
    return value;
}

/**
 * Emits a C++ bool literal: true or false.
 */
string bool_literal(bool value) {
    return value ? "true" : "false";
}

/**
 * Emits std::nullopt for the none literal.
 */
string none_literal() {
    return "std::nullopt";
}

/**
 * Emits a C++ char literal.
 */
string char_literal(char value) {
    return fmt::format("'{}'", value);
}

} // namespace codegen
