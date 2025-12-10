/**
 * @file structs.hpp
 * @brief C++ code generation for Nog structs.
 *
 * Helper functions for emitting struct definitions, struct literals,
 * field access, field assignment, and methods.
 */

#pragma once
#include <string>
#include <vector>
#include <utility>
#include <fmt/core.h>
#include <fmt/ranges.h>
#include "types.hpp"

using namespace std;

namespace nog::runtime {

/** Emits a C++ struct definition with fields */
inline string struct_def(const string& name, const vector<pair<string, string>>& fields) {
    string body;
    for (const auto& [field_name, field_type] : fields) {
        string cpp_type = map_type(field_type);
        if (cpp_type == "void") {
            cpp_type = field_type;
        }
        body += fmt::format("\t{} {};\n", cpp_type, field_name);
    }
    return fmt::format("struct {} {{\n{}}};", name, body);
}

/** Emits a struct literal: TypeName { .field = value, ... } */
inline string struct_literal(const string& name, const vector<pair<string, string>>& field_values) {
    vector<string> inits;
    for (const auto& [field_name, value] : field_values) {
        inits.push_back(fmt::format(".{} = {}", field_name, value));
    }
    return fmt::format("{} {{ {} }}", name, fmt::join(inits, ", "));
}

/** Emits field access: object.field */
inline string field_access(const string& object, const string& field) {
    return fmt::format("{}.{}", object, field);
}

/** Emits field assignment: object.field = value */
inline string field_assignment(const string& object, const string& field, const string& value) {
    return fmt::format("{}.{} = {}", object, field, value);
}

/** Emits a struct definition with both fields and method bodies */
inline string struct_def_with_methods(const string& name,
                                      const vector<pair<string, string>>& fields,
                                      const vector<string>& method_bodies) {
    string body;

    // Generate fields
    for (const auto& [field_name, field_type] : fields) {
        string cpp_type = map_type(field_type);

        if (cpp_type == "void") {
            cpp_type = field_type;
        }

        body += fmt::format("\t{} {};\n", cpp_type, field_name);
    }

    // Add methods
    for (const auto& method_body : method_bodies) {
        body += method_body;
    }

    return fmt::format("struct {} {{\n{}}};", name, body);
}

/** Emits a method definition as a C++ member function */
inline string method_def(const string& name,
                         const vector<pair<string, string>>& params,
                         const string& return_type,
                         const vector<string>& body_stmts) {
    string rt = return_type.empty() ? "void" : map_type(return_type);

    vector<string> param_strs;
    for (const auto& [ptype, pname] : params) {
        param_strs.push_back(fmt::format("{} {}", map_type(ptype), pname));
    }

    string out = fmt::format("\t{} {}({}) {{\n", rt, name, fmt::join(param_strs, ", "));

    for (const auto& stmt : body_stmts) {
        out += fmt::format("\t\t{}\n", stmt);
    }

    out += "\t}\n";
    return out;
}

}
