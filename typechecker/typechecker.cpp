/**
 * @file typechecker.cpp
 * @brief Main type checker entry point and utility functions.
 *
 * Contains the main check() function, collection functions for building
 * symbol tables, and type utility functions.
 */

#include "typechecker.hpp"

using namespace std;

namespace typechecker {

// -----------------------------------------------------------------------------
// Local scope helpers
// -----------------------------------------------------------------------------

void push_scope(TypeCheckerState& state) {
    state.local_scopes.emplace_back();
}

void pop_scope(TypeCheckerState& state) {
    if (!state.local_scopes.empty()) {
        state.local_scopes.pop_back();
    }
}

bool is_declared_in_current_scope(const TypeCheckerState& state, const string& name) {
    if (state.local_scopes.empty()) {
        return false;
    }

    const auto& current = state.local_scopes.back();
    return current.find(name) != current.end();
}

void declare_local(TypeCheckerState& state, const string& name, const TypeInfo& type, int line) {
    if (state.local_scopes.empty()) {
        // All checking should start with an initial scope; this fallback keeps the
        // checker resilient if a new entry point forgets to push one.
        push_scope(state);
    }

    if (is_declared_in_current_scope(state, name)) {
        error(state, "variable '" + name + "' is already defined in this scope", line);
        return;
    }

    state.local_scopes.back()[name] = type;
}

TypeInfo* lookup_local(TypeCheckerState& state, const string& name) {
    for (auto it = state.local_scopes.rbegin(); it != state.local_scopes.rend(); ++it) {
        auto found = it->find(name);

        if (found != it->end()) {
            return &found->second;
        }
    }

    return nullptr;
}

const TypeInfo* lookup_local(const TypeCheckerState& state, const string& name) {
    for (auto it = state.local_scopes.rbegin(); it != state.local_scopes.rend(); ++it) {
        auto found = it->find(name);

        if (found != it->end()) {
            return &found->second;
        }
    }

    return nullptr;
}

/**
 * Main entry point for type checking. Collects all declarations into symbol
 * tables, then validates each function and method body.
 */
bool check(TypeCheckerState& state, const Program& program, const string& fname) {
    state.filename = fname;
    state.errors.clear();

    collect_structs(state, program);
    collect_methods(state, program);
    collect_functions(state, program);
    collect_extern_functions(state, program);

    for (const auto& method : program.methods) {
        check_method(state, *method);
    }

    for (const auto& func : program.functions) {
        check_function(state, *func);
    }

    return state.errors.empty();
}

/**
 * Registers an imported module for cross-module type checking.
 */
void register_module(TypeCheckerState& state, const string& alias, const Module& module) {
    state.imported_modules[alias] = &module;
}

/**
 * Collects all struct definitions into the structs symbol table.
 */
void collect_structs(TypeCheckerState& state, const Program& program) {
    for (const auto& s : program.structs) {
        state.structs[s->name] = s.get();
    }
}

/**
 * Collects all method definitions. Validates that the struct exists
 * and checks for duplicate method definitions.
 */
void collect_methods(TypeCheckerState& state, const Program& program) {
    for (const auto& m : program.methods) {
        if (state.structs.find(m->struct_name) == state.structs.end()) {
            error(state, "method '" + m->name + "' defined on unknown struct '" + m->struct_name + "'", m->line);
            continue;
        }

        auto& struct_methods = state.methods[m->struct_name];

        for (const auto* existing : struct_methods) {
            if (existing->name == m->name) {
                error(state, "duplicate method '" + m->name + "' on struct '" + m->struct_name + "'", m->line);
                break;
            }
        }

        struct_methods.push_back(m.get());
    }
}

/**
 * Collects all function definitions into the functions symbol table.
 */
void collect_functions(TypeCheckerState& state, const Program& program) {
    for (const auto& f : program.functions) {
        state.functions[f->name] = f.get();
    }
}

/**
 * Collects all extern function declarations into the extern_functions symbol table.
 */
void collect_extern_functions(TypeCheckerState& state, const Program& program) {
    for (const auto& e : program.externs) {
        state.extern_functions[e->name] = e.get();
    }
}

/**
 * Checks if a type is a built-in primitive (int, str, bool, etc).
 */
bool is_primitive_type(const string& type) {
    return type == "int" || type == "str" || type == "bool" ||
           type == "char" || type == "f32" || type == "f64" ||
           type == "u32" || type == "u64" ||
           type == "cint" || type == "cstr" || type == "void";
}

/**
 * Checks if a type is valid (either a primitive, a known struct, channel,
 * function type, or a qualified module type).
 */
bool is_valid_type(const TypeCheckerState& state, const string& type) {
    if (is_primitive_type(type)) {
        return true;
    }

    if (state.structs.find(type) != state.structs.end()) {
        return true;
    }

    if (type.rfind("fn:", 0) == 0 || type.rfind("fn(", 0) == 0) {
        return true;
    }

    if (type.rfind("Channel<", 0) == 0 && type.back() == '>') {
        size_t start = 8;
        size_t end = type.find('>', start);
        string element_type = type.substr(start, end - start);
        return is_valid_type(state, element_type);
    }

    if (type.rfind("List<", 0) == 0 && type.back() == '>') {
        size_t start = 5;
        size_t end = type.find('>', start);
        string element_type = type.substr(start, end - start);
        return is_valid_type(state, element_type);
    }

    size_t dot_pos = type.find('.');

    if (dot_pos != string::npos) {
        string module_name = type.substr(0, dot_pos);
        string type_name = type.substr(dot_pos + 1);
        return get_qualified_struct(state, module_name, type_name) != nullptr;
    }

    return false;
}

/**
 * Checks if actual type can be assigned to expected type.
 */
bool types_compatible(const TypeInfo& expected, const TypeInfo& actual) {
    if (actual.base_type == "none" && expected.is_optional && !expected.is_awaitable) {
        return true;
    }

    if (actual.base_type == "unknown" || expected.base_type == "unknown") {
        return true;
    }

    if (expected.is_awaitable != actual.is_awaitable) {
        return false;
    }

    if (actual.base_type.rfind("fn:", 0) == 0 && expected.base_type.rfind("fn(", 0) == 0) {
        return true;
    }

    if (expected.base_type == "cstr" && actual.base_type == "str") {
        return true;
    }

    if (expected.base_type == "cint" && actual.base_type == "int") {
        return true;
    }

    // Numeric type conversions
    if (expected.base_type == "u32" && actual.base_type == "int") {
        return true;
    }

    if (expected.base_type == "u64" && actual.base_type == "int") {
        return true;
    }

    if (expected.base_type == "f32" && actual.base_type == "f64") {
        return true;
    }

    return expected.base_type == actual.base_type;
}

/**
 * Formats a type for error messages, including awaitable/optional modifiers.
 */
string format_type(const TypeInfo& type) {
    string inner = type.base_type;

    if (type.is_optional && inner != "none") {
        inner += "?";
    }

    if (type.is_awaitable) {
        return "awaitable<" + inner + ">";
    }

    return inner;
}

/**
 * Wraps a type in an awaitable marker (used for async calls and channel ops).
 */
TypeInfo make_awaitable(const TypeInfo& inner) {
    TypeInfo out = inner;
    out.is_awaitable = true;
    return out;
}

/**
 * Looks up a struct definition by name. Handles both local structs
 * and qualified module types (module.Type).
 */
const StructDef* get_struct(const TypeCheckerState& state, const string& name) {
    auto it = state.structs.find(name);

    if (it != state.structs.end()) {
        return it->second;
    }

    size_t dot_pos = name.find('.');

    if (dot_pos != string::npos) {
        string module_name = name.substr(0, dot_pos);
        string type_name = name.substr(dot_pos + 1);
        return get_qualified_struct(state, module_name, type_name);
    }

    return nullptr;
}

/**
 * Looks up a method on a struct. Returns nullptr if not found.
 */
const MethodDef* get_method(const TypeCheckerState& state, const string& struct_name, const string& method_name) {
    auto it = state.methods.find(struct_name);

    if (it == state.methods.end()) {
        return nullptr;
    }

    for (const auto* m : it->second) {
        if (m->name == method_name) {
            return m;
        }
    }

    return nullptr;
}

/**
 * Gets the type of a field on a struct. Returns empty string if not found.
 */
string get_field_type(const TypeCheckerState& state, const string& struct_name, const string& field_name) {
    const StructDef* sdef = get_struct(state, struct_name);

    if (!sdef) {
        return "";
    }

    for (const auto& field : sdef->fields) {
        if (field.name == field_name) {
            return field.type;
        }
    }

    return "";
}

/**
 * Records a type error with the given message and line number.
 */
void error(TypeCheckerState& state, const string& msg, int line) {
    state.errors.push_back({msg, line, state.filename});
}

/**
 * Looks up a function in an imported module by module alias and function name.
 */
const FunctionDef* get_qualified_function(const TypeCheckerState& state, const string& module, const string& name) {
    auto it = state.imported_modules.find(module);

    if (it == state.imported_modules.end()) {
        return nullptr;
    }

    for (const auto* func : it->second->get_public_functions()) {
        if (func->name == name) {
            return func;
        }
    }

    return nullptr;
}

/**
 * Looks up a struct in an imported module by module alias and struct name.
 */
const StructDef* get_qualified_struct(const TypeCheckerState& state, const string& module, const string& name) {
    auto it = state.imported_modules.find(module);

    if (it == state.imported_modules.end()) {
        return nullptr;
    }

    for (const auto* s : it->second->get_public_structs()) {
        if (s->name == name) {
            return s;
        }
    }

    return nullptr;
}

/**
 * Looks up a method on a struct in an imported module.
 */
const MethodDef* get_qualified_method(const TypeCheckerState& state, const string& module, const string& struct_name, const string& method_name) {
    auto it = state.imported_modules.find(module);

    if (it == state.imported_modules.end()) {
        return nullptr;
    }

    for (const auto* m : it->second->get_public_methods(struct_name)) {
        if (m->name == method_name) {
            return m;
        }
    }

    return nullptr;
}

} // namespace typechecker

// Legacy class API for backwards compatibility
bool TypeChecker::check(const Program& program, const string& filename) {
    return typechecker::check(state, program, filename);
}

void TypeChecker::register_module(const string& alias, const Module& module) {
    typechecker::register_module(state, alias, module);
}
