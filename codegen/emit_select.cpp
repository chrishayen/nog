/**
 * @file emit_select.cpp
 * @brief Select statement emission for the Nog code generator.
 */

#include "codegen.hpp"
#include <fmt/format.h>

using namespace std;

namespace codegen {

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

} // namespace codegen
