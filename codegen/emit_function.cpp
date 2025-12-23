/**
 * @file emit_function.cpp
 * @brief Function and method emission for the Nog code generator.
 *
 * Handles emitting C++ code for function definitions, method definitions,
 * and the test harness.
 */

#include "codegen.hpp"
#include <fmt/format.h>
#include <fmt/ranges.h>

using namespace std;

namespace codegen {

/**
 * Emits a complete function definition with parameters, return type, and body.
 */
string function_def(const string& name, const vector<FunctionParam>& params,
                    const string& return_type, const vector<string>& body) {
    string rt = return_type.empty() ? "void" : map_type(return_type);

    vector<string> param_strs;

    for (const auto& p : params) {
        param_strs.push_back(fmt::format("{} {}", map_type(p.type), p.name));
    }

    string out = fmt::format("{} {}({}) {{\n", rt, name, fmt::join(param_strs, ", "));

    for (const auto& stmt : body) {
        out += fmt::format("\t{}\n", stmt);
    }

    out += "}\n";
    return out;
}

/**
 * Emits a method definition as a C++ member function.
 */
string method_def(const string& name,
                  const vector<pair<string, string>>& params,
                  const string& return_type,
                  const vector<string>& body_stmts) {
    string rt = return_type.empty() ? "void" : map_type(return_type);

    vector<string> param_strs;

    for (const auto& [ptype, pname] : params) {
        param_strs.push_back(fmt::format("{} {}", map_type(ptype), pname));
    }

    string out = fmt::format("\t{} {}({}) {{\n", rt, name, fmt::join(param_strs, ", "));

    for (const auto& stmt : body_stmts) {
        out += fmt::format("\t\t{}\n", stmt);
    }

    out += "\t}\n";
    return out;
}

/**
 * Returns the C++ return type for a function, accounting for fallibility.
 * Fallible functions return nog::rt::Result<T>.
 */
static string get_cpp_return_type(const string& return_type, const string& error_type) {
    if (error_type.empty()) {
        return return_type.empty() ? "void" : map_type(return_type);
    }

    // Fallible function - return Result<T>
    if (return_type.empty()) {
        return "nog::rt::Result<void>";
    }

    return "nog::rt::Result<" + map_type(return_type) + ">";
}

/**
 * Generates a C++ function from a Nog FunctionDef.
 * Maps Nog types to C++ types and handles main() specially.
 * Main is wrapped in a fiber for goroutine support.
 */
string generate_function(CodeGenState& state, const FunctionDef& fn) {
    bool is_main = (fn.name == "main" && !state.test_mode);
    bool is_fallible = !fn.error_type.empty();

    // Track fallibility for or-return handling
    bool prev_fallible = state.in_fallible_function;
    state.in_fallible_function = is_fallible;

    vector<FunctionParam> params;

    for (const auto& p : fn.params) {
        params.push_back({p.type, p.name});
    }

    vector<string> body;

    for (const auto& stmt : fn.body) {
        body.push_back(generate_statement(state, *stmt));
    }

    // Add implicit return {} for Result<void> functions without explicit return
    if (is_fallible && fn.return_type.empty() && !body.empty()) {
        // Check if last statement is a return
        string last = body.back();

        if (last.find("return") == string::npos) {
            body.push_back("return {};");
        }
    }

    state.in_fallible_function = prev_fallible;

    string out;

    if (is_main) {
        // Generate the function as _nog_main()
        string rt_type = fn.return_type.empty() ? "" : fn.return_type;
        out = function_def("_nog_main", params, rt_type, body);

        // Generate int main() with fiber-asio scheduler
        out += "\nint main() {\n";
        out += "\t// Initialize fiber-asio scheduler\n";
        out += "\tnog::rt::io_ctx = std::make_shared<boost::asio::io_context>();\n";
        out += "\tboost::fibers::use_scheduling_algorithm<\n";
        out += "\t\tboost::fibers::asio::round_robin>(nog::rt::io_ctx);\n";
        out += "\n";
        out += "\t// Run main as a fiber\n";
        out += "\tboost::fibers::fiber(_nog_main).detach();\n";
        out += "\n";
        out += "\t// Drive fiber scheduler and async I/O\n";
        out += "\tnog::rt::io_ctx->run();\n";
        out += "\treturn 0;\n";
        out += "}\n";
    } else {
        // Use raw return type string for fallible functions
        string cpp_rt = get_cpp_return_type(fn.return_type, fn.error_type);

        vector<string> param_strs;

        for (const auto& p : params) {
            param_strs.push_back(fmt::format("{} {}", map_type(p.type), p.name));
        }

        out = fmt::format("{} {}({}) {{\n", cpp_rt, fn.name, fmt::join(param_strs, ", "));

        for (const auto& stmt : body) {
            out += fmt::format("\t{}\n", stmt);
        }

        out += "}\n";
    }

    return out;
}

/**
 * Generates a C++ member function from a Nog MethodDef.
 * Transforms self.field into this->field.
 */
string generate_method(CodeGenState& state, const MethodDef& method) {
    // Skip 'self' parameter for C++ method (it becomes 'this')
    vector<pair<string, string>> params;

    for (size_t i = 1; i < method.params.size(); i++) {
        params.push_back({method.params[i].type, method.params[i].name});
    }

    vector<string> body;

    for (const auto& stmt : method.body) {
        body.push_back(generate_statement(state, *stmt));
    }

    return method_def(method.name, params, method.return_type, body);
}

/**
 * Generates the test harness main() function.
 * Calls all test_* functions and returns failure count.
 */
string generate_test_harness(CodeGenState& state, const unique_ptr<Program>& program) {
    state.current_program = program.get();

    // Collect extern functions for FFI handling
    for (const auto& ext : program->externs) {
        state.extern_functions[ext->name] = ext.get();
    }

    string out = "#include <nog/std.hpp>\n\n";

    // Generate extern "C" declarations for FFI
    out += generate_extern_declarations(program);

    out += "int _failures = 0;\n\n";
    out += "template<typename T, typename U>\n";
    out += "void _assert_eq(T a, U b, int line) {\n";
    out += "\tif (a != b) {\n";
    out += "\t\tstd::cerr << \"line \" << line << \": FAIL: \" << a << \" != \" << b << std::endl;\n";
    out += "\t\t_failures++;\n";
    out += "\t}\n";
    out += "}\n\n";

    for (const auto& s : program->structs) {
        out += generate_struct(state, *s) + "\n\n";
    }

    for (const auto& e : program->errors) {
        out += generate_error(state, *e) + "\n";
    }

    vector<pair<string, bool>> test_funcs;  // name, is_fallible

    for (const auto& fn : program->functions) {
        if (fn->name.rfind("test_", 0) == 0) {
            test_funcs.push_back({fn->name, !fn->error_type.empty()});
        }

        out += generate_function(state, *fn);
    }

    out += "\nint main() {\n";
    out += "\t// Initialize fiber-asio scheduler for tests\n";
    out += "\tnog::rt::io_ctx = std::make_shared<boost::asio::io_context>();\n";
    out += "\tboost::fibers::use_scheduling_algorithm<\n";
    out += "\t\tboost::fibers::asio::round_robin>(nog::rt::io_ctx);\n";
    out += "\n";

    // Run each test in a fiber, join to wait for completion
    for (const auto& [name, is_fallible] : test_funcs) {
        if (is_fallible) {
            // Fallible test: check for errors
            out += "\tboost::fibers::fiber([]() {\n";
            out += "\t\tauto result = " + name + "();\n";
            out += "\t\tif (result.is_error()) {\n";
            out += "\t\t\tstd::cerr << \"" + name + ": FAIL: \" << result.error()->message << std::endl;\n";
            out += "\t\t\t_failures++;\n";
            out += "\t\t}\n";
            out += "\t}).join();\n";
        } else {
            out += "\tboost::fibers::fiber(" + name + ").join();\n";
        }
    }

    out += "\treturn _failures;\n";
    out += "}\n";

    return out;
}

} // namespace codegen
