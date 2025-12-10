/**
 * @file statements.hpp
 * @brief C++ code generation for Nog statements.
 *
 * Helper functions for emitting variable declarations, return statements,
 * assignments, if statements, and while loops.
 */

#pragma once
#include <string>
#include <vector>
#include <fmt/format.h>
#include "types.hpp"

using namespace std;

namespace nog::runtime {

/**
 * Emits a variable declaration: type name = value;
 * Uses 'auto' for inferred types, std::optional<T> for optional types.
 */
inline string variable_decl(const string& type, const string& name, const string& value, bool is_optional = false) {
    string t = type.empty() ? "auto" : map_type(type);
    if (is_optional) {
        return fmt::format("std::optional<{}> {} = {};", t, name, value);
    }
    return fmt::format("{} {} = {};", t, name, value);
}

/** Emits a return statement: return value; */
inline string return_stmt(const string& value) {
    return fmt::format("return {};", value);
}

/** Emits an assignment: name = value; */
inline string assignment(const string& name, const string& value) {
    return fmt::format("{} = {};", name, value);
}

/** Emits an if statement with optional else block */
inline string if_stmt(const string& condition, const vector<string>& then_body, const vector<string>& else_body) {
    string out = fmt::format("if ({}) {{\n", condition);
    for (const auto& stmt : then_body) {
        out += "\t" + stmt + "\n";
    }
    out += "}";
    if (!else_body.empty()) {
        out += " else {\n";
        for (const auto& stmt : else_body) {
            out += "\t" + stmt + "\n";
        }
        out += "}";
    }
    return out;
}

/** Emits a while loop */
inline string while_stmt(const string& condition, const vector<string>& body) {
    string out = fmt::format("while ({}) {{\n", condition);
    for (const auto& stmt : body) {
        out += "\t" + stmt + "\n";
    }
    out += "}";
    return out;
}

}
