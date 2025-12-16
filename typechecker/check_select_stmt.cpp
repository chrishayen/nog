/**
 * @file check_select_stmt.cpp
 * @brief Select statement checking for the Nog type checker.
 */

#include "typechecker.hpp"

using namespace std;

namespace typechecker {

/**
 * Type checks a select statement.
 */
void check_select_stmt(TypeCheckerState& state, const SelectStmt& select_stmt) {
    for (const auto& select_case : select_stmt.cases) {
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
}

} // namespace typechecker
