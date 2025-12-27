/**
 * @file emit_method_call.cpp
 * @brief Method call emission for the Bishop code generator.
 */

#include "codegen.hpp"
#include <fmt/format.h>
#include <fmt/ranges.h>

using namespace std;

namespace codegen {

/**
 * Emits a method call: object.method(arg1, arg2, ...).
 */
string method_call(const string& object, const string& method, const vector<string>& args) {
    return fmt::format("{}.{}({})", object, method, fmt::join(args, ", "));
}

/**
 * Emits a method call AST node with special handling for self, channels, and lists.
 */
string emit_method_call(CodeGenState& state, const MethodCall& call) {
    vector<string> args;

    for (const auto& arg : call.args) {
        args.push_back(emit(state, *arg));
    }

    // Handle self.method() -> this->method() in methods
    if (auto* ref = dynamic_cast<const VariableRef*>(call.object.get())) {
        if (ref->name == "self") {
            string args_str;

            for (size_t i = 0; i < args.size(); i++) {
                if (i > 0) args_str += ", ";
                args_str += args[i];
            }

            return "this->" + call.method_name + "(" + args_str + ")";
        }
    }

    // Handle channel methods - direct calls on bishop::rt::Channel
    if (call.method_name == "send") {
        string val = args.empty() ? "" : args[0];
        return emit(state, *call.object) + ".send(" + val + ")";
    }

    if (call.method_name == "recv") {
        return emit(state, *call.object) + ".recv()";
    }

    string obj_str = emit(state, *call.object);

    // Handle List methods - map to std::vector equivalents
    if (call.object_type.rfind("List<", 0) == 0) {
        return emit_list_method_call(state, call, obj_str, args);
    }

    // Use -> for pointer types (auto-deref like Go)
    if (!call.object_type.empty() && call.object_type.back() == '*') {
        return fmt::format("{}->{}({})", obj_str, call.method_name, fmt::join(args, ", "));
    }

    return method_call(obj_str, call.method_name, args);
}

} // namespace codegen
