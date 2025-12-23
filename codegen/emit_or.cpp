/**
 * @file emit_or.cpp
 * @brief Or expression emission for the Nog code generator.
 *
 * Generates C++ code for or expressions that handle errors.
 * This is complex because or expressions need to generate statements
 * (temp variable, error check, value extraction) not just expressions.
 */

#include "codegen.hpp"
#include <fmt/format.h>
#include <fmt/ranges.h>

using namespace std;

namespace codegen {

static int or_temp_counter = 0;

/**
 * Get a unique temp variable name.
 */
static string get_temp_name() {
    return "_or_tmp" + to_string(++or_temp_counter);
}

/**
 * Emit the handler code for an OrReturn.
 */
string emit_or_return_handler(CodeGenState& state, const OrReturn& handler) {
    if (handler.value) {
        return "return " + emit(state, *handler.value) + ";";
    }

    // Use {} for Result<void> functions, plain return for void
    return state.in_fallible_function ? "return {};" : "return;";
}

/**
 * Emit the handler code for an OrFail.
 */
string emit_or_fail_handler(CodeGenState& state, const OrFail& handler) {
    // The err variable is bound in the generated code
    if (auto* var = dynamic_cast<const VariableRef*>(handler.error_expr.get())) {
        if (var->name == "err") {
            return "return err;";
        }
    }

    return "return " + emit(state, *handler.error_expr) + ";";
}

/**
 * Emit the handler code for an OrBlock.
 */
string emit_or_block_handler(CodeGenState& state, const OrBlock& handler) {
    string out;

    for (const auto& stmt : handler.body) {
        out += "\t\t" + generate_statement(state, *stmt) + "\n";
    }

    return out;
}

/**
 * Emit the handler code for an OrMatch.
 * The var_name parameter is used to assign values from match arms.
 */
string emit_or_match_handler(CodeGenState& state, const OrMatch& handler, const string& var_name) {
    string out;
    bool first = true;

    for (const auto& arm : handler.arms) {
        if (arm.error_type == "_") {
            // Default arm - close previous typed arm and start else
            if (!first) {
                out += "\t\t} else {\n";
            } else {
                out += "{\n";
            }

            if (auto* fail = dynamic_cast<const FailStmt*>(arm.body.get())) {
                out += "\t\t\t" + emit_fail(state, *fail) + ";\n";
            } else {
                out += fmt::format("\t\t\t{} = {};\n", var_name, emit(state, *arm.body));
            }

            out += "\t\t}";
        } else {
            // Typed arm
            string check = first ? "if" : "\t\t} else if";
            out += fmt::format("{} (dynamic_cast<{}*>(err.get())) {{\n",
                               check, arm.error_type);

            if (auto* fail = dynamic_cast<const FailStmt*>(arm.body.get())) {
                out += "\t\t\t" + emit_fail(state, *fail) + ";\n";
            } else {
                out += fmt::format("\t\t\t{} = {};\n", var_name, emit(state, *arm.body));
            }
        }

        first = false;
    }

    // Close last arm if it wasn't the default
    if (!out.empty() && out.back() == '\n') {
        out += "\t\t}";
    }

    return out;
}

/**
 * Generates code for an OrExpr as part of a variable declaration.
 * Returns the statements that should be emitted before the variable,
 * and sets the value expression to use for the variable.
 */
OrEmitResult emit_or_for_decl(CodeGenState& state, const OrExpr& expr, const string& var_name) {
    OrEmitResult result;
    string temp = get_temp_name();
    result.temp_var = temp;

    // Generate the temp variable with the result
    result.preamble = fmt::format("auto {} = {};", temp, emit(state, *expr.expr));

    // Generate error check
    string handler_code;

    if (auto* ret = dynamic_cast<const OrReturn*>(expr.handler.get())) {
        handler_code = emit_or_return_handler(state, *ret);
        result.check = fmt::format("if ({}.is_error()) {{ {} }}", temp, handler_code);
        result.value_expr = temp + ".value()";
    } else if (auto* fail = dynamic_cast<const OrFail*>(expr.handler.get())) {
        // For or fail, we need to define err before returning it
        handler_code = "auto err = " + temp + ".error(); " + emit_or_fail_handler(state, *fail);
        result.check = fmt::format("if ({}.is_error()) {{ {} }}", temp, handler_code);
        result.value_expr = temp + ".value()";
    } else if (auto* block = dynamic_cast<const OrBlock*>(expr.handler.get())) {
        handler_code = "auto err = " + temp + ".error();\n" + emit_or_block_handler(state, *block);
        result.check = fmt::format("if ({}.is_error()) {{ {} }}", temp, handler_code);
        result.value_expr = temp + ".value()";
    } else if (auto* match = dynamic_cast<const OrMatch*>(expr.handler.get())) {
        // Match is special - we need to declare var first, then assign in branches
        string match_code = emit_or_match_handler(state, *match, var_name);
        result.check = fmt::format(
            "if ({}.is_error()) {{\n\t\tauto err = {}.error();\n\t\t{}\n\t}} else {{\n\t\t{} = {}.value();\n\t}}",
            temp, temp, match_code, var_name, temp);
        result.value_expr = "";  // Empty - variable is assigned in the check
        result.is_match = true;
    }

    return result;
}

/**
 * Emit a standalone or expression (not part of a declaration).
 * This is unusual but could happen in some contexts.
 */
string emit_or_expr(CodeGenState& state, const OrExpr& expr) {
    // For standalone use, just emit the expression
    // The caller will need to handle the Result type
    return emit(state, *expr.expr);
}

/**
 * Emit a default expression.
 */
string emit_default_expr(CodeGenState& state, const DefaultExpr& expr) {
    string value = emit(state, *expr.expr);
    string fallback = emit(state, *expr.fallback);

    // Use ternary for falsy check
    return fmt::format("({} ? {} : {})", value, value, fallback);
}

} // namespace codegen
