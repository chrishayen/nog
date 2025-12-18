/**
 * @file emit_select.cpp
 * @brief Select statement emission for the Nog code generator.
 */

#include "codegen.hpp"
#include <fmt/format.h>

using namespace std;

namespace codegen {

/**
 * Generates C++ for a select statement using polling with stackful coroutines.
 */
string generate_select(CodeGenState& state, const SelectStmt& stmt) {
    string out;

    // Generate a polling loop that checks each channel
    out += "while (true) {\n";

    size_t case_idx = 0;

    for (const auto& c : stmt.cases) {
        string channel_code = emit(state, *c->channel);

        if (c->operation == "recv") {
            out += "\t{\n";
            out += "\t\tauto _try_result = " + channel_code + ".try_recv();\n";
            out += "\t\tif (_try_result.first) {\n";

            if (!c->binding_name.empty()) {
                out += "\t\t\tauto " + c->binding_name + " = _try_result.second;\n";
            }

            for (const auto& s : c->body) {
                out += "\t\t\t" + generate_statement(state, *s) + "\n";
            }

            out += "\t\t\tbreak;\n";
            out += "\t\t}\n";
            out += "\t}\n";
        }

        case_idx++;
    }

    // Yield to let other goroutines run
    out += "\tboost::asio::post(nog::rt::io_context(), nog::rt::yield());\n";
    out += "}\n";

    return out;
}

} // namespace codegen
