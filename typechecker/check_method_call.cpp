/**
 * @file check_method_call.cpp
 * @brief Method call type inference for the Nog type checker.
 */

#include "typechecker.hpp"
#include "strings.hpp"

using namespace std;

namespace typechecker {

/**
 * Type checks a method call on a string.
 */
TypeInfo check_str_method(TypeCheckerState& state, const MethodCall& mcall) {
    auto method_info = nog::get_str_method_info(mcall.method_name);

    if (!method_info) {
        error(state, "str has no method '" + mcall.method_name + "'", mcall.line);
        return {"unknown", false, false};
    }

    const auto& [param_types, return_type] = *method_info;

    if (mcall.args.size() != param_types.size()) {
        error(state, "method '" + mcall.method_name + "' expects " +
              to_string(param_types.size()) + " arguments, got " +
              to_string(mcall.args.size()), mcall.line);
    }

    for (size_t i = 0; i < mcall.args.size() && i < param_types.size(); i++) {
        TypeInfo arg_type = infer_type(state, *mcall.args[i]);
        TypeInfo expected = {param_types[i], false, false};

        if (!types_compatible(expected, arg_type)) {
            error(state, "argument " + to_string(i + 1) + " of method '" +
                  mcall.method_name + "' expects '" + param_types[i] +
                  "', got '" + format_type(arg_type) + "'", mcall.line);
        }
    }

    return {return_type, false, false};
}

/**
 * Type checks a method call on a struct.
 */
TypeInfo check_struct_method(TypeCheckerState& state, const MethodCall& mcall, const TypeInfo& obj_type) {
    const StructDef* sdef = get_struct(state, obj_type.base_type);

    if (!sdef) {
        error(state, "cannot call method on non-struct type '" + format_type(obj_type) + "'", mcall.line);
        return {"unknown", false, false};
    }

    const MethodDef* method = nullptr;
    size_t dot_pos = obj_type.base_type.find('.');

    if (dot_pos != string::npos) {
        string module_name = obj_type.base_type.substr(0, dot_pos);
        string struct_name = obj_type.base_type.substr(dot_pos + 1);
        method = get_qualified_method(state, module_name, struct_name, mcall.method_name);
    } else {
        method = get_method(state, obj_type.base_type, mcall.method_name);
    }

    if (!method) {
        error(state, "method '" + mcall.method_name + "' not found on struct '" + obj_type.base_type + "'", mcall.line);
        return {"unknown", false, false};
    }

    size_t expected_args = method->params.size() - 1;

    if (mcall.args.size() != expected_args) {
        error(state, "method '" + mcall.method_name + "' expects " + to_string(expected_args) + " arguments, got " + to_string(mcall.args.size()), mcall.line);
    }

    for (size_t i = 0; i < mcall.args.size() && i + 1 < method->params.size(); i++) {
        TypeInfo arg_type = infer_type(state, *mcall.args[i]);
        TypeInfo param_type = {method->params[i + 1].type, false, false};

        if (!types_compatible(param_type, arg_type)) {
            error(state, "argument " + to_string(i + 1) + " of method '" + mcall.method_name +
                  "' expects '" + format_type(param_type) + "', got '" + format_type(arg_type) + "'", mcall.line);
        }
    }

    TypeInfo ret_type = method->return_type.empty() ? TypeInfo{"void", false, true}
                                                    : TypeInfo{method->return_type, false, false};

    if (method->is_async) {
        if (!state.in_async_context) {
            error(state, "async method '" + mcall.method_name + "' can only be called inside async functions", mcall.line);
        }

        return make_awaitable(ret_type);
    }

    return ret_type;
}

/**
 * Infers the type of a method call expression.
 */
TypeInfo check_method_call(TypeCheckerState& state, const MethodCall& mcall) {
    TypeInfo obj_type = infer_type(state, *mcall.object);

    if (obj_type.is_awaitable) {
        error(state, "cannot call method on '" + format_type(obj_type) + "' (did you forget 'await'?)", mcall.line);
        return {"unknown", false, false};
    }

    mcall.object_type = obj_type.base_type;  // Store for codegen

    if (obj_type.base_type.rfind("Channel<", 0) == 0) {
        size_t start = 8;
        size_t end = obj_type.base_type.find('>', start);
        string element_type = obj_type.base_type.substr(start, end - start);
        return check_channel_method(state, mcall, element_type);
    }

    if (obj_type.base_type.rfind("List<", 0) == 0) {
        size_t start = 5;
        size_t end = obj_type.base_type.find('>', start);
        string element_type = obj_type.base_type.substr(start, end - start);
        return check_list_method(state, mcall, element_type);
    }

    if (obj_type.base_type == "str") {
        return check_str_method(state, mcall);
    }

    return check_struct_method(state, mcall, obj_type);
}

} // namespace typechecker
