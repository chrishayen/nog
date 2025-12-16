/**
 * @file emit_statement.cpp
 * @brief Statement emission for the Nog code generator.
 *
 * Handles emitting C++ code for all statement types: variable declarations,
 * assignments, if/while statements, function calls, etc.
 */

#include "codegen.hpp"
#include <fmt/format.h>
#include <fmt/ranges.h>

using namespace std;

namespace codegen {

/**
 * Emits a variable declaration: type name = value;
 * Uses 'auto' for inferred types, std::optional<T> for optional types.
 */
string variable_decl(const string& type, const string& name, const string& value, bool is_optional) {
    string t = type.empty() ? "auto" : map_type(type);

    if (is_optional) {
        return fmt::format("std::optional<{}> {} = {};", t, name, value);
    }

    return fmt::format("{} {} = {};", t, name, value);
}

/**
 * Emits a return statement: return value;
 */
string return_stmt(const string& value) {
    return fmt::format("return {};", value);
}

/**
 * Emits an assignment: name = value;
 */
string assignment(const string& name, const string& value) {
    return fmt::format("{} = {};", name, value);
}

/**
 * Emits an if statement with optional else block.
 */
string if_stmt(const string& condition, const vector<string>& then_body, const vector<string>& else_body) {
    string out = fmt::format("if ({}) {{\n", condition);

    for (const auto& stmt : then_body) {
        out += "\t" + stmt + "\n";
    }

    out += "}";

    if (!else_body.empty()) {
        out += " else {\n";

        for (const auto& stmt : else_body) {
            out += "\t" + stmt + "\n";
        }

        out += "}";
    }

    return out;
}

/**
 * Emits a while loop.
 */
string while_stmt(const string& condition, const vector<string>& body) {
    string out = fmt::format("while ({}) {{\n", condition);

    for (const auto& stmt : body) {
        out += "\t" + stmt + "\n";
    }

    out += "}";
    return out;
}

/**
 * Emits std::cout for multiple values.
 */
string print_multi(const vector<string>& args) {
    return fmt::format("std::cout << {} << std::endl", fmt::join(args, " << "));
}

/**
 * Emits a call to the _assert_eq test helper with line number for errors.
 */
string assert_eq(const string& a, const string& b, int line) {
    return fmt::format("_assert_eq({}, {}, {})", a, b, line);
}

/**
 * Generates C++ for a select statement using ASIO awaitable operators.
 */
string generate_select(CodeGenState& state, const SelectStmt& stmt) {
    string out;

    vector<string> ops;

    for (const auto& c : stmt.cases) {
        string channel_code = emit(state, *c->channel);

        if (c->operation == "recv") {
            ops.push_back(channel_code + ".async_receive(asio::as_tuple(asio::use_awaitable))");
        } else if (c->operation == "send") {
            string send_val = c->send_value ? emit(state, *c->send_value) : "";
            ops.push_back(channel_code + ".async_send(asio::error_code{}, " + send_val + ", asio::use_awaitable)");
        }
    }

    out += "auto _sel_result = co_await (";

    for (size_t i = 0; i < ops.size(); i++) {
        out += ops[i];

        if (i < ops.size() - 1) {
            out += " || ";
        }
    }

    out += ");\n";

    size_t case_idx = 0;

    for (const auto& c : stmt.cases) {
        out += "if (_sel_result.index() == " + to_string(case_idx) + ") {\n";

        if (c->operation == "recv" && !c->binding_name.empty()) {
            out += "\tauto " + c->binding_name + " = std::get<1>(std::get<" + to_string(case_idx) + ">(_sel_result));\n";
        }

        for (const auto& s : c->body) {
            out += "\t" + generate_statement(state, *s) + "\n";
        }

        out += "}\n";
        case_idx++;
    }

    return out;
}

/**
 * Generates C++ code for a statement. Handles print(), assert_eq() (in test mode),
 * if/while statements, method calls, field assignments, and other statements.
 */
string generate_statement(CodeGenState& state, const ASTNode& node) {
    if (auto* call = dynamic_cast<const FunctionCall*>(&node)) {
        if (call->name == "print") {
            vector<string> args;

            for (const auto& arg : call->args) {
                args.push_back(emit(state, *arg));
            }

            return print_multi(args) + ";";
        }

        if (call->name == "assert_eq" && state.test_mode && call->args.size() >= 2) {
            return assert_eq(emit(state, *call->args[0]), emit(state, *call->args[1]), call->line) + ";";
        }

        // Check if this is an extern function call
        auto ext_it = state.extern_functions.find(call->name);

        if (ext_it != state.extern_functions.end()) {
            // Extern function - convert string args to const char* with .c_str()
            const ExternFunctionDef* ext = ext_it->second;
            vector<string> args;

            for (size_t i = 0; i < call->args.size(); i++) {
                string arg_code = emit(state, *call->args[i]);

                // If param type is cstr, convert std::string to const char*
                if (i < ext->params.size() && ext->params[i].type == "cstr") {
                    // Wrap the argument with .c_str() call
                    // Need to handle string literals specially
                    if (auto* str_lit = dynamic_cast<const StringLiteral*>(call->args[i].get())) {
                        // String literal - just emit the raw string for C function
                        (void)str_lit;
                        args.push_back(arg_code + ".c_str()");
                    } else {
                        // Variable or expression - needs .c_str()
                        args.push_back("(" + arg_code + ").c_str()");
                    }
                } else {
                    args.push_back(arg_code);
                }
            }

            return function_call(call->name, args) + ";";
        }

        vector<string> args;

        for (const auto& arg : call->args) {
            args.push_back(emit(state, *arg));
        }

        // Handle qualified function call: module.func -> module::func
        string func_name = call->name;
        size_t dot_pos = func_name.find('.');

        if (dot_pos != string::npos) {
            func_name = func_name.substr(0, dot_pos) + "::" + func_name.substr(dot_pos + 1);
        }

        return function_call(func_name, args) + ";";
    }

    if (auto* stmt = dynamic_cast<const IfStmt*>(&node)) {
        vector<string> then_body;

        for (const auto& s : stmt->then_body) {
            then_body.push_back(generate_statement(state, *s));
        }

        vector<string> else_body;

        for (const auto& s : stmt->else_body) {
            else_body.push_back(generate_statement(state, *s));
        }

        return if_stmt(emit(state, *stmt->condition), then_body, else_body);
    }

    if (auto* stmt = dynamic_cast<const WhileStmt*>(&node)) {
        vector<string> body;

        for (const auto& s : stmt->body) {
            body.push_back(generate_statement(state, *s));
        }

        return while_stmt(emit(state, *stmt->condition), body);
    }

    if (auto* select_stmt = dynamic_cast<const SelectStmt*>(&node)) {
        return generate_select(state, *select_stmt);
    }

    if (auto* await_expr = dynamic_cast<const AwaitExpr*>(&node)) {
        return emit(state, node) + ";";
    }

    if (auto* call = dynamic_cast<const MethodCall*>(&node)) {
        return emit(state, node) + ";";
    }

    if (auto* fa = dynamic_cast<const FieldAssignment*>(&node)) {
        return emit(state, node) + ";";
    }

    return emit(state, node);
}

} // namespace codegen
