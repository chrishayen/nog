/**
 * @file emit_function_call.cpp
 * @brief Function call emission for the Nog code generator.
 */

#include "codegen.hpp"
#include <fmt/format.h>
#include <fmt/ranges.h>

using namespace std;

namespace codegen {

/**
 * Emits a function call: name(arg1, arg2, ...).
 */
string function_call(const string& name, const vector<string>& args) {
    return fmt::format("{}({})", name, fmt::join(args, ", "));
}

/**
 * Emits a function call AST node.
 */
string emit_function_call(CodeGenState& state, const FunctionCall& call) {
    vector<string> args;

    for (const auto& arg : call.args) {
        args.push_back(emit(state, *arg));
    }

    // Handle qualified function call: module.func -> module::func
    string func_name = call.name;
    size_t dot_pos = func_name.find('.');

    if (dot_pos != string::npos) {
        func_name = func_name.substr(0, dot_pos) + "::" + func_name.substr(dot_pos + 1);
    }

    return function_call(func_name, args);
}

} // namespace codegen
