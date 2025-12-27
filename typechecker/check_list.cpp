/**
 * @file check_list.cpp
 * @brief List type inference for the Bishop type checker.
 */

#include "typechecker.hpp"
#include "lists.hpp"

using namespace std;

namespace typechecker {

/**
 * Infers the type of a list creation expression.
 */
TypeInfo check_list_create(TypeCheckerState& state, const ListCreate& list) {
    (void)state;
    return {"List<" + list.element_type + ">", false, false};
}

/**
 * Infers the type of a list literal.
 */
TypeInfo check_list_literal(TypeCheckerState& state, const ListLiteral& list) {
    if (list.elements.empty()) {
        error(state, "cannot infer type of empty list literal, use List<T>() instead", list.line);
        return {"unknown", false, false};
    }

    TypeInfo first_type = infer_type(state, *list.elements[0]);

    for (size_t i = 1; i < list.elements.size(); i++) {
        TypeInfo elem_type = infer_type(state, *list.elements[i]);

        if (elem_type.base_type != first_type.base_type) {
            error(state, "list literal has mixed types: '" + format_type(first_type) +
                  "' and '" + format_type(elem_type) + "'", list.line);
        }
    }

    return {"List<" + first_type.base_type + ">", false, false};
}

/**
 * Type checks a method call on a list.
 */
TypeInfo check_list_method(TypeCheckerState& state, const MethodCall& mcall, const string& element_type) {
    auto method_info = nog::get_list_method_info(mcall.method_name);

    if (!method_info) {
        error(state, "List has no method '" + mcall.method_name + "'", mcall.line);
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
        string expected = param_types[i] == "T" ? element_type : param_types[i];
        TypeInfo expected_type = {expected, false, false};

        if (!types_compatible(expected_type, arg_type)) {
            error(state, "argument " + to_string(i + 1) + " of method '" +
                  mcall.method_name + "' expects '" + expected +
                  "', got '" + format_type(arg_type) + "'", mcall.line);
        }
    }

    string ret = return_type == "T" ? element_type : return_type;

    if (ret == "void") {
        return {"void", false, true};
    }

    return {ret, false, false};
}

} // namespace typechecker
