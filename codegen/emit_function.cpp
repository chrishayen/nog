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
 * Generates a C++ function from a Nog FunctionDef.
 * Maps Nog types to C++ types and handles main() specially.
 * Main is wrapped with boost::asio::spawn for goroutine support.
 */
string generate_function(CodeGenState& state, const FunctionDef& fn) {
    bool is_main = (fn.name == "main" && !state.test_mode);

    vector<FunctionParam> params;

    for (const auto& p : fn.params) {
        params.push_back({p.type, p.name});
    }

    vector<string> body;

    for (const auto& stmt : fn.body) {
        body.push_back(generate_statement(state, *stmt));
    }

    string out;

    if (is_main) {
        // Generate the function as _nog_main()
        string rt_type = fn.return_type.empty() ? "" : fn.return_type;
        out = function_def("_nog_main", params, rt_type, body);

        // Generate int main() with thread pool for parallel goroutines
        out += "\nint main() {\n";
        out += "\tboost::asio::io_context io_context;\n";
        out += "\tnog::rt::global_io_context = &io_context;\n\n";
        out += "\tauto work = boost::asio::make_work_guard(io_context);\n";
        out += "\tstd::vector<std::thread> threads;\n";
        out += "\tfor (unsigned i = 0; i < std::thread::hardware_concurrency(); i++) {\n";
        out += "\t\tthreads.emplace_back([&io_context] { io_context.run(); });\n";
        out += "\t}\n\n";
        out += "\tboost::asio::spawn(io_context, [](boost::asio::yield_context yield) {\n";
        out += "\t\tnog::rt::YieldScope scope(yield);\n";
        out += "\t\t_nog_main();\n";
        out += "\t}, boost::asio::detached);\n\n";
        out += "\twork.reset();\n";
        out += "\tfor (auto& t : threads) t.join();\n";
        out += "\treturn 0;\n";
        out += "}\n";
    } else {
        out = function_def(fn.name, params, fn.return_type, body);
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

    string out = "#include <nog/std.hpp>\n";
    out += "#include <boost/asio.hpp>\n";
    out += "#include <boost/asio/spawn.hpp>\n\n";

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

    vector<string> test_funcs;

    for (const auto& fn : program->functions) {
        if (fn->name.rfind("test_", 0) == 0) {
            test_funcs.push_back(fn->name);
        }

        out += generate_function(state, *fn);
    }

    out += "\nint main() {\n";
    out += "\tboost::asio::io_context io_context;\n";
    out += "\tnog::rt::global_io_context = &io_context;\n\n";
    out += "\tauto work = boost::asio::make_work_guard(io_context);\n";
    out += "\tstd::vector<std::thread> threads;\n";
    out += "\tfor (unsigned i = 0; i < std::thread::hardware_concurrency(); i++) {\n";
    out += "\t\tthreads.emplace_back([&io_context] { io_context.run(); });\n";
    out += "\t}\n\n";

    for (const auto& name : test_funcs) {
        out += "\tboost::asio::spawn(io_context, [](boost::asio::yield_context yield) {\n";
        out += "\t\tnog::rt::YieldScope scope(yield);\n";
        out += "\t\t" + name + "();\n";
        out += "\t}, boost::asio::detached);\n";
    }

    out += "\n\twork.reset();\n";
    out += "\tfor (auto& t : threads) t.join();\n";
    out += "\treturn _failures;\n";
    out += "}\n";

    return out;
}

} // namespace codegen
