/**
 * @file emit_fail.cpp
 * @brief Fail statement emission for the Bishop code generator.
 *
 * Generates C++ code for fail statements that return errors from functions.
 */

#include "codegen.hpp"
#include <fmt/format.h>
#include <fmt/ranges.h>

using namespace std;

namespace codegen {

/**
 * Generates C++ return statement for a fail expression.
 * For string: return std::make_shared<bishop::rt::Error>("message");
 * For error struct: return std::make_shared<ErrorType>("msg", field1, field2);
 */
string emit_fail(CodeGenState& state, const FailStmt& stmt) {
    if (!stmt.value) {
        return "return std::make_shared<bishop::rt::Error>(\"error\")";
    }

    // Check if it's a string literal
    if (auto* str_lit = dynamic_cast<const StringLiteral*>(stmt.value.get())) {
        return fmt::format("return std::make_shared<bishop::rt::Error>({})",
                           string_literal(str_lit->value));
    }

    // Check if it's an error struct literal
    if (auto* struct_lit = dynamic_cast<const StructLiteral*>(stmt.value.get())) {
        vector<string> args;

        // Find the message field
        string message_val = "\"\"";
        for (const auto& [name, value] : struct_lit->field_values) {
            if (name == "message") {
                message_val = emit(state, *value);
                break;
            }
        }

        args.push_back(message_val);

        // Add other fields in order
        for (const auto& [name, value] : struct_lit->field_values) {
            if (name != "message" && name != "cause") {
                args.push_back(emit(state, *value));
            }
        }

        // Check for cause field
        for (const auto& [name, value] : struct_lit->field_values) {
            if (name == "cause") {
                args.push_back(emit(state, *value));
                break;
            }
        }

        // Cast to base Error type for Result<T> compatibility
        return fmt::format(
            "return std::static_pointer_cast<bishop::rt::Error>(std::make_shared<{}>({}))",
            struct_lit->struct_name,
            fmt::join(args, ", "));
    }

    // For other expressions (variable ref, etc.)
    return fmt::format("return {}", emit(state, *stmt.value));
}

} // namespace codegen
