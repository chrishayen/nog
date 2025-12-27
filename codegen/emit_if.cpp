/**
 * @file emit_if.cpp
 * @brief If statement emission for the Bishop code generator.
 */

#include "codegen.hpp"
#include <fmt/format.h>

using namespace std;

namespace codegen {

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

} // namespace codegen
