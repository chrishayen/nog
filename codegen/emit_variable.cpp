/**
 * @file emit_variable.cpp
 * @brief Variable declaration and assignment emission for the Nog code generator.
 */

#include "codegen.hpp"
#include <fmt/format.h>

using namespace std;

namespace codegen {

/**
 * Emits a variable declaration: type name = value;
 * Uses 'auto' for inferred types, std::optional<T> for optional types.
 */
string variable_decl(const string& type, const string& name, const string& value, bool is_optional) {
    string t = type.empty() ? "auto" : map_type(type);

    if (is_optional) {
        return fmt::format("std::optional<{}> {} = {};", t, name, value);
    }

    return fmt::format("{} {} = {};", t, name, value);
}

/**
 * Emits a return statement: return value;
 */
string return_stmt(const string& value) {
    return fmt::format("return {};", value);
}

/**
 * Emits an assignment: name = value;
 */
string assignment(const string& name, const string& value) {
    return fmt::format("{} = {};", name, value);
}

} // namespace codegen
