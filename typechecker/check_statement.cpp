/**
 * @file check_statement.cpp
 * @brief Statement type checking for the Nog type checker.
 *
 * Handles type checking for all statement types: variable declarations,
 * assignments, return statements, if/while, function calls, etc.
 */

#include "typechecker.hpp"

using namespace std;

namespace typechecker {

/**
 * Validates a single statement. Handles variable declarations, assignments,
 * field assignments, return statements, if/while statements, and function/method calls.
 */
void check_statement(TypeCheckerState& state, const ASTNode& stmt) {
    if (auto* decl = dynamic_cast<const VariableDecl*>(&stmt)) {
        if (!decl->type.empty() && !is_valid_type(state, decl->type)) {
            error(state, "unknown type '" + decl->type + "'", decl->line);
        }

        if (decl->value) {
            TypeInfo init_type = infer_type(state, *decl->value);

            if (!decl->type.empty()) {
                TypeInfo expected = {decl->type, decl->is_optional, false};

                if (!types_compatible(expected, init_type)) {
                    error(state, "cannot assign '" + init_type.base_type + "' to variable of type '" + decl->type + "'", decl->line);
                }
            }

            if (!decl->type.empty()) {
                state.locals[decl->name] = {decl->type, decl->is_optional, false};
            } else {
                state.locals[decl->name] = init_type;
            }
        }
    } else if (auto* assign = dynamic_cast<const Assignment*>(&stmt)) {
        if (state.locals.find(assign->name) == state.locals.end()) {
            error(state, "assignment to undefined variable '" + assign->name + "'", assign->line);
            return;
        }

        TypeInfo var_type = state.locals[assign->name];
        TypeInfo val_type = infer_type(state, *assign->value);

        if (!types_compatible(var_type, val_type)) {
            error(state, "cannot assign '" + val_type.base_type + "' to variable of type '" + var_type.base_type + "'", assign->line);
        }
    } else if (auto* fa = dynamic_cast<const FieldAssignment*>(&stmt)) {
        TypeInfo obj_type = infer_type(state, *fa->object);

        if (state.structs.find(obj_type.base_type) == state.structs.end()) {
            error(state, "cannot access field on non-struct type '" + obj_type.base_type + "'", fa->line);
            return;
        }

        string field_type = get_field_type(state, obj_type.base_type, fa->field_name);

        if (field_type.empty()) {
            error(state, "struct '" + obj_type.base_type + "' has no field '" + fa->field_name + "'", fa->line);
            return;
        }

        TypeInfo expected = {field_type, false, false};
        TypeInfo val_type = infer_type(state, *fa->value);

        if (!types_compatible(expected, val_type)) {
            error(state, "cannot assign '" + val_type.base_type + "' to field of type '" + field_type + "'", fa->line);
        }
    } else if (auto* ret = dynamic_cast<const ReturnStmt*>(&stmt)) {
        TypeInfo ret_type = {"void", false, true};

        if (ret->value) {
            ret_type = infer_type(state, *ret->value);
        }

        if (!types_compatible(state.current_return, ret_type)) {
            error(state, "return type '" + ret_type.base_type + "' does not match declared type '" + state.current_return.base_type + "'", ret->line);
        }
    } else if (auto* if_stmt = dynamic_cast<const IfStmt*>(&stmt)) {
        TypeInfo cond_type = infer_type(state, *if_stmt->condition);

        if (cond_type.base_type != "bool" && !cond_type.is_optional) {
            error(state, "if condition must be bool or optional type, got '" + cond_type.base_type + "'", if_stmt->line);
        }

        for (const auto& s : if_stmt->then_body) {
            check_statement(state, *s);
        }

        for (const auto& s : if_stmt->else_body) {
            check_statement(state, *s);
        }
    } else if (auto* while_stmt = dynamic_cast<const WhileStmt*>(&stmt)) {
        TypeInfo cond_type = infer_type(state, *while_stmt->condition);

        if (cond_type.base_type != "bool") {
            error(state, "while condition must be bool, got '" + cond_type.base_type + "'", while_stmt->line);
        }

        for (const auto& s : while_stmt->body) {
            check_statement(state, *s);
        }
    } else if (auto* for_stmt = dynamic_cast<const ForStmt*>(&stmt)) {
        if (for_stmt->kind == ForLoopKind::Range) {
            TypeInfo start_type = infer_type(state, *for_stmt->range_start);
            TypeInfo end_type = infer_type(state, *for_stmt->range_end);

            if (start_type.base_type != "int") {
                error(state, "for range start must be int, got '" + start_type.base_type + "'", for_stmt->line);
            }

            if (end_type.base_type != "int") {
                error(state, "for range end must be int, got '" + end_type.base_type + "'", for_stmt->line);
            }

            state.locals[for_stmt->loop_var] = {"int", false, false};
        } else {
            TypeInfo iter_type = infer_type(state, *for_stmt->iterable);

            if (iter_type.base_type.rfind("List<", 0) != 0) {
                error(state, "for-each requires a List, got '" + iter_type.base_type + "'", for_stmt->line);
            } else {
                size_t start = 5;
                size_t end = iter_type.base_type.find('>', start);
                string element_type = iter_type.base_type.substr(start, end - start);

                state.locals[for_stmt->loop_var] = {element_type, false, false};
            }
        }

        for (const auto& s : for_stmt->body) {
            check_statement(state, *s);
        }
    } else if (auto* select_stmt = dynamic_cast<const SelectStmt*>(&stmt)) {
        for (const auto& select_case : select_stmt->cases) {
            TypeInfo channel_type = infer_type(state, *select_case->channel);

            if (channel_type.base_type.rfind("Channel<", 0) != 0) {
                error(state, "select case requires a channel, got '" + channel_type.base_type + "'", select_case->line);
                continue;
            }

            size_t start = 8;
            size_t end = channel_type.base_type.find('>', start);
            string element_type = channel_type.base_type.substr(start, end - start);

            if (select_case->operation == "recv" && !select_case->binding_name.empty()) {
                state.locals[select_case->binding_name] = {element_type, false, false};
            }

            if (select_case->operation == "send" && select_case->send_value) {
                TypeInfo val_type = infer_type(state, *select_case->send_value);
                TypeInfo expected = {element_type, false, false};

                if (!types_compatible(expected, val_type)) {
                    error(state, "select send expects '" + element_type + "', got '" + val_type.base_type + "'", select_case->line);
                }
            }

            for (const auto& s : select_case->body) {
                check_statement(state, *s);
            }
        }
    } else if (auto* call = dynamic_cast<const FunctionCall*>(&stmt)) {
        infer_type(state, *call);
    } else if (auto* mcall = dynamic_cast<const MethodCall*>(&stmt)) {
        infer_type(state, *mcall);
    } else if (auto* await_expr = dynamic_cast<const AwaitExpr*>(&stmt)) {
        infer_type(state, *await_expr);
    }
}

} // namespace typechecker
