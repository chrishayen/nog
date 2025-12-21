/**
 * @file check_call.cpp
 * @brief Function call type inference for the Nog type checker.
 */

#include "typechecker.hpp"

using namespace std;

namespace typechecker {

/**
 * Infers the type of a function call expression.
 */
TypeInfo check_function_call(TypeCheckerState& state, const FunctionCall& call) {
    if (call.name == "assert_eq" || call.name == "print") {
        // Still infer types for arguments (to populate object_type on MethodCalls)
        for (const auto& arg : call.args) {
            infer_type(state, *arg);
        }

        return {"void", false, true};
    }

    if (call.name == "sleep") {
        if (call.args.size() != 1) {
            error(state, "sleep expects 1 argument (milliseconds), got " + to_string(call.args.size()), call.line);
        } else {
            TypeInfo arg_type = infer_type(state, *call.args[0]);

            if (arg_type.base_type != "int") {
                error(state, "sleep expects int argument, got '" + format_type(arg_type) + "'", call.line);
            }
        }

        return {"void", false, true};
    }

    size_t dot_pos = call.name.find('.');

    if (dot_pos != string::npos) {
        string module_name = call.name.substr(0, dot_pos);
        string func_name = call.name.substr(dot_pos + 1);

        const FunctionDef* func = get_qualified_function(state, module_name, func_name);

        if (!func) {
            error(state, "undefined function '" + call.name + "'", call.line);
            return {"unknown", false, false};
        }

        if (call.args.size() != func->params.size()) {
            error(state, "function '" + call.name + "' expects " + to_string(func->params.size()) + " arguments, got " + to_string(call.args.size()), call.line);
        }

        for (size_t i = 0; i < call.args.size() && i < func->params.size(); i++) {
            TypeInfo arg_type = infer_type(state, *call.args[i]);
            TypeInfo param_type = {func->params[i].type, false, false};

            if (!types_compatible(param_type, arg_type)) {
                error(state, "argument " + to_string(i + 1) + " of function '" + call.name +
                      "' expects '" + format_type(param_type) + "', got '" + format_type(arg_type) + "'", call.line);
            }
        }

        return func->return_type.empty() ? TypeInfo{"void", false, true}
                                         : TypeInfo{func->return_type, false, false};
    }

    if (const TypeInfo* local = lookup_local(state, call.name)) {
        TypeInfo local_type = *local;

        if (local_type.base_type.rfind("fn(", 0) == 0) {
            size_t arrow_pos = local_type.base_type.find(" -> ");

            if (arrow_pos != string::npos) {
                string ret_type = local_type.base_type.substr(arrow_pos + 4);
                return {ret_type, false, false};
            }

            return {"void", false, true};
        }
    }

    if (state.functions.find(call.name) != state.functions.end()) {
        const FunctionDef* func = state.functions[call.name];

        if (call.args.size() != func->params.size()) {
            error(state, "function '" + call.name + "' expects " + to_string(func->params.size()) + " arguments, got " + to_string(call.args.size()), call.line);
        }

        for (size_t i = 0; i < call.args.size() && i < func->params.size(); i++) {
            TypeInfo arg_type = infer_type(state, *call.args[i]);
            TypeInfo param_type = {func->params[i].type, false, false};

            if (!types_compatible(param_type, arg_type)) {
                error(state, "argument " + to_string(i + 1) + " of function '" + call.name +
                      "' expects '" + format_type(param_type) + "', got '" + format_type(arg_type) + "'", call.line);
            }
        }

        return func->return_type.empty() ? TypeInfo{"void", false, true}
                                         : TypeInfo{func->return_type, false, false};
    }

    if (state.extern_functions.find(call.name) != state.extern_functions.end()) {
        const ExternFunctionDef* ext = state.extern_functions[call.name];

        if (call.args.size() != ext->params.size()) {
            error(state, "function '" + call.name + "' expects " + to_string(ext->params.size()) + " arguments, got " + to_string(call.args.size()), call.line);
        }

        for (size_t i = 0; i < call.args.size() && i < ext->params.size(); i++) {
            TypeInfo arg_type = infer_type(state, *call.args[i]);
            TypeInfo param_type = {ext->params[i].type, false, false};

            if (!types_compatible(param_type, arg_type)) {
                error(state, "argument " + to_string(i + 1) + " of function '" + call.name +
                      "' expects '" + format_type(param_type) + "', got '" + format_type(arg_type) + "'", call.line);
            }
        }

        if (ext->return_type.empty() || ext->return_type == "void") {
            return {"void", false, true};
        }

        return {ext->return_type, false, false};
    }

    error(state, "undefined function '" + call.name + "'", call.line);
    return {"unknown", false, false};
}

} // namespace typechecker
