/**
 * @file emit_list.cpp
 * @brief List emission for the Nog code generator.
 */

#include "codegen.hpp"
#include <fmt/format.h>

using namespace std;

namespace codegen {

/**
 * Emits a list creation: List<T>() -> std::vector<T>{}.
 */
string emit_list_create(const ListCreate& list) {
    string cpp_type = map_type(list.element_type);
    return "std::vector<" + cpp_type + ">{}";
}

/**
 * Emits a list literal: [1, 2, 3] -> std::vector{1, 2, 3}.
 */
string emit_list_literal(CodeGenState& state, const ListLiteral& list) {
    string elements;

    for (size_t i = 0; i < list.elements.size(); i++) {
        if (i > 0) {
            elements += ", ";
        }

        elements += emit(state, *list.elements[i]);
    }

    return "std::vector{" + elements + "}";
}

/**
 * Emits a list method call, mapping Nog methods to std::vector equivalents.
 */
string emit_list_method_call(CodeGenState& state, const MethodCall& call, const string& obj_str, const vector<string>& args) {
    if (call.method_name == "length") {
        return obj_str + ".size()";
    }

    if (call.method_name == "is_empty") {
        return obj_str + ".empty()";
    }

    if (call.method_name == "append") {
        return obj_str + ".push_back(" + args[0] + ")";
    }

    if (call.method_name == "pop") {
        return obj_str + ".pop_back()";
    }

    if (call.method_name == "get") {
        return obj_str + ".at(" + args[0] + ")";
    }

    if (call.method_name == "set") {
        return obj_str + "[" + args[0] + "] = " + args[1];
    }

    if (call.method_name == "clear") {
        return obj_str + ".clear()";
    }

    if (call.method_name == "first") {
        return obj_str + ".front()";
    }

    if (call.method_name == "last") {
        return obj_str + ".back()";
    }

    if (call.method_name == "insert") {
        return obj_str + ".insert(" + obj_str + ".begin() + " + args[0] + ", " + args[1] + ")";
    }

    if (call.method_name == "remove") {
        return obj_str + ".erase(" + obj_str + ".begin() + " + args[0] + ")";
    }

    if (call.method_name == "contains") {
        return "(std::find(" + obj_str + ".begin(), " + obj_str + ".end(), " + args[0] + ") != " + obj_str + ".end())";
    }

    // Unknown list method - fall back to generic method call
    return method_call(obj_str, call.method_name, args);
}

} // namespace codegen
