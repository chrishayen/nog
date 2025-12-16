/**
 * @file emit_while.cpp
 * @brief While statement emission for the Nog code generator.
 */

#include "codegen.hpp"
#include <fmt/format.h>

using namespace std;

namespace codegen {

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

} // namespace codegen
