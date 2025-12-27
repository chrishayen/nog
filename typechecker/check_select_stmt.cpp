/**
 * @file check_select_stmt.cpp
 * @brief Select statement checking for the Bishop type checker.
 */

#include "typechecker.hpp"

using namespace std;

namespace typechecker {

/**
 * Type checks a select statement.
 */
void check_select_stmt(TypeCheckerState& state, const SelectStmt& select_stmt) {
    for (const auto& select_case : select_stmt.cases) {
        // Each case introduces its own scope so bindings and declarations inside one
        // case do not leak into other cases or after the select statement.
        push_scope(state);

        TypeInfo channel_type = infer_type(state, *select_case->channel);

        if (channel_type.base_type.rfind("Channel<", 0) != 0) {
            error(state, "select case requires a channel, got '" + format_type(channel_type) + "'", select_case->line);
            pop_scope(state);
            continue;
        }

        size_t start = 8;
        size_t end = channel_type.base_type.find('>', start);
        string element_type = channel_type.base_type.substr(start, end - start);

        if (select_case->operation == "recv" && !select_case->binding_name.empty()) {
            declare_local(state, select_case->binding_name, {element_type, false, false}, select_case->line);
        }

        if (select_case->operation == "send" && select_case->send_value) {
            TypeInfo val_type = infer_type(state, *select_case->send_value);
            TypeInfo expected = {element_type, false, false};

            if (!types_compatible(expected, val_type)) {
                error(state, "select send expects '" + element_type + "', got '" + format_type(val_type) + "'", select_case->line);
            }
        }

        for (const auto& s : select_case->body) {
            check_statement(state, *s);
        }

        pop_scope(state);
    }
}

} // namespace typechecker
