/**
 * @file emit_struct.cpp
 * @brief Struct emission for the Bishop code generator.
 *
 * Handles emitting C++ code for struct definitions, struct literals,
 * and field access/assignment.
 */

#include "codegen.hpp"
#include <fmt/format.h>
#include <fmt/ranges.h>

using namespace std;

namespace codegen {

/**
 * Emits a C++ struct definition with fields.
 */
string struct_def(const string& name, const vector<pair<string, string>>& fields) {
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

/**
 * Emits a struct definition with both fields and method bodies.
 */
string struct_def_with_methods(const string& name,
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

/**
 * Emits a struct literal: TypeName { .field = value, ... }.
 */
string struct_literal(const string& name, const vector<pair<string, string>>& field_values) {
    vector<string> inits;

    for (const auto& [field_name, value] : field_values) {
        inits.push_back(fmt::format(".{} = {}", field_name, value));
    }

    return fmt::format("{} {{ {} }}", name, fmt::join(inits, ", "));
}

/**
 * Emits field access: object.field.
 */
string field_access(const string& object, const string& field) {
    return fmt::format("{}.{}", object, field);
}

/**
 * Emits field assignment: object.field = value.
 */
string field_assignment(const string& object, const string& field, const string& value) {
    return fmt::format("{}.{} = {}", object, field, value);
}

/**
 * Generates a C++ struct with optional methods.
 * Methods become member functions with 'self' mapped to 'this'.
 */
string generate_struct(CodeGenState& state, const StructDef& def) {
    vector<pair<string, string>> fields;

    for (const auto& f : def.fields) {
        fields.push_back({f.name, f.type});
    }

    // Find methods for this struct
    vector<string> method_bodies;

    if (state.current_program) {
        for (const auto& method : state.current_program->methods) {
            if (method->struct_name == def.name) {
                method_bodies.push_back(generate_method(state, *method));
            }
        }
    }

    if (method_bodies.empty()) {
        return struct_def(def.name, fields);
    }

    return struct_def_with_methods(def.name, fields, method_bodies);
}

} // namespace codegen
