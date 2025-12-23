/**
 * @file emit_error.cpp
 * @brief Error type emission for the Nog code generator.
 *
 * Handles emitting C++ code for error type definitions.
 * Error types inherit from nog::rt::Error and have message/cause built-in.
 */

#include "codegen.hpp"
#include <fmt/format.h>
#include <fmt/ranges.h>

using namespace std;

namespace codegen {

/**
 * Generates a C++ struct that inherits from nog::rt::Error.
 * All error types have built-in message (string) and cause (shared_ptr<Error>).
 */
string generate_error(CodeGenState& state, const ErrorDef& def) {
    string out = fmt::format("struct {} : public nog::rt::Error {{\n", def.name);

    // Generate custom fields
    for (const auto& f : def.fields) {
        string cpp_type = map_type(f.type);

        if (cpp_type == "void") {
            cpp_type = f.type;
        }

        out += fmt::format("\t{} {};\n", cpp_type, f.name);
    }

    // Generate constructor
    vector<string> params;
    vector<string> inits;

    params.push_back("const std::string& msg");

    for (const auto& f : def.fields) {
        string cpp_type = map_type(f.type);

        if (cpp_type == "void") {
            cpp_type = f.type;
        }

        params.push_back(fmt::format("{} {}_", cpp_type, f.name));
        inits.push_back(fmt::format("{}({}_)", f.name, f.name));
    }

    params.push_back("std::shared_ptr<nog::rt::Error> cause_ = nullptr");

    out += fmt::format("\n\t{}({}) : nog::rt::Error(msg, cause_)", def.name, fmt::join(params, ", "));

    if (!inits.empty()) {
        out += fmt::format(", {}", fmt::join(inits, ", "));
    }

    out += " {}\n};\n";
    return out;
}

/**
 * Generates error literal construction.
 * Returns shared_ptr creation expression.
 */
string error_literal(const string& name, const string& message, const vector<pair<string, string>>& field_values) {
    vector<string> args;
    args.push_back(message);

    for (const auto& [field_name, value] : field_values) {
        args.push_back(value);
    }

    return fmt::format("std::make_shared<{}>({})", name, fmt::join(args, ", "));
}

} // namespace codegen
