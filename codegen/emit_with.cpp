/**
 * @file emit_with.cpp
 * @brief With statement emission for the Bishop code generator.
 *
 * Generates C++ code for with statements that provide automatic
 * resource cleanup via RAII.
 */

#include "codegen.hpp"
#include <fmt/format.h>

using namespace std;

namespace codegen {

/**
 * Generates C++ code for a with statement.
 * Creates a scoped block with RAII guard that calls close() on exit.
 *
 * Input Nog:
 *   with fs.open("file.txt") as file {
 *       content := file.read();
 *   }
 *
 * Output C++:
 *   {
 *       auto file = fs::open("file.txt");
 *       struct _with_guard {
 *           decltype(file)& _res;
 *           ~_with_guard() { _res.close(); }
 *       } _guard{file};
 *       auto content = file.read();
 *   }
 */
string generate_with(CodeGenState& state, const WithStmt& stmt) {
    string out = "{\n";

    // Declare the resource variable
    string resource_expr = emit(state, *stmt.resource);
    out += fmt::format("\tauto {} = {};\n", stmt.binding_name, resource_expr);

    // Create RAII guard to call close() on scope exit
    out += fmt::format("\tstruct _with_guard_{} {{\n", stmt.binding_name);
    out += fmt::format("\t\tdecltype({})& _res;\n", stmt.binding_name);
    out += "\t\t~_with_guard_" + stmt.binding_name + "() { _res.close(); }\n";
    out += fmt::format("\t}} _guard_{}{{{}}};\n", stmt.binding_name, stmt.binding_name);

    // Generate body statements
    for (const auto& body_stmt : stmt.body) {
        out += fmt::format("\t{}\n", generate_statement(state, *body_stmt));
    }

    out += "}";
    return out;
}

} // namespace codegen
