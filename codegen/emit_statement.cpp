/**
 * @file emit_statement.cpp
 * @brief Statement dispatch for the Nog code generator.
 *
 * Main entry point for generating C++ code from statement AST nodes.
 * Individual statement types are implemented in separate files.
 */

#include "codegen.hpp"
#include <fmt/format.h>
#include <fmt/ranges.h>

using namespace std;

namespace codegen {

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

    if (auto* stmt = dynamic_cast<const ForStmt*>(&node)) {
        vector<string> body;

        for (const auto& s : stmt->body) {
            body.push_back(generate_statement(state, *s));
        }

        if (stmt->kind == ForLoopKind::Range) {
            return for_range_stmt(
                stmt->loop_var,
                emit(state, *stmt->range_start),
                emit(state, *stmt->range_end),
                body
            );
        } else {
            return for_each_stmt(
                stmt->loop_var,
                emit(state, *stmt->iterable),
                body
            );
        }
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
