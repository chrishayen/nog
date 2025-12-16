/**
 * @file check_refs.cpp
 * @brief Reference type inference for the Nog type checker.
 */

#include "typechecker.hpp"

using namespace std;

namespace typechecker {

/**
 * Infers the type of a variable reference.
 */
TypeInfo check_variable_ref(TypeCheckerState& state, const VariableRef& var) {
    if (state.locals.find(var.name) != state.locals.end()) {
        return state.locals[var.name];
    }

    error(state, "undefined variable '" + var.name + "'", var.line);
    return {"unknown", false, false};
}

/**
 * Infers the type of a function reference.
 */
TypeInfo check_function_ref(TypeCheckerState& state, const FunctionRef& fref) {
    size_t dot_pos = fref.name.find('.');

    if (dot_pos != string::npos) {
        string module_name = fref.name.substr(0, dot_pos);
        string func_name = fref.name.substr(dot_pos + 1);
        const FunctionDef* func = get_qualified_function(state, module_name, func_name);

        if (!func) {
            error(state, "undefined function '" + fref.name + "'", fref.line);
            return {"unknown", false, false};
        }

        return {"fn:" + fref.name, false, false};
    }

    if (state.functions.find(fref.name) == state.functions.end()) {
        error(state, "undefined function '" + fref.name + "'", fref.line);
        return {"unknown", false, false};
    }

    return {"fn:" + fref.name, false, false};
}

/**
 * Infers the type of a qualified reference.
 */
TypeInfo check_qualified_ref(TypeCheckerState& state, const QualifiedRef& qref) {
    const StructDef* s = get_qualified_struct(state, qref.module_name, qref.name);

    if (s) {
        return {qref.module_name + "." + qref.name, false, false};
    }

    error(state, "undefined reference '" + qref.module_name + "." + qref.name + "'", qref.line);
    return {"unknown", false, false};
}

} // namespace typechecker
