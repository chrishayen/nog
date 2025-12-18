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
                    const string& return_type, const vector<string>& body,
                    bool is_async) {
    string rt;

    if (is_async) {
        string inner_type = return_type.empty() ? "void" : map_type(return_type);
        rt = "asio::awaitable<" + inner_type + ">";
    } else {
        rt = return_type.empty() ? "void" : map_type(return_type);
    }

    vector<string> param_strs;

    for (const auto& p : params) {
        param_strs.push_back(fmt::format("{} {}", map_type(p.type), p.name));
    }

    string out = fmt::format("{} {}({}) {{\n", rt, name, fmt::join(param_strs, ", "));

    for (const auto& stmt : body) {
        out += fmt::format("\t{}\n", stmt);
    }

    // Async functions need at least one co_await/co_return to be recognized as coroutines
    // Add implicit co_return for void async functions without explicit return
    if (is_async && return_type.empty()) {
        out += "\tco_return;\n";
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
                  const vector<string>& body_stmts,
                  bool is_async) {
    string rt;

    if (is_async) {
        string inner_type = return_type.empty() ? "void" : map_type(return_type);
        rt = "asio::awaitable<" + inner_type + ">";
    } else {
        rt = return_type.empty() ? "void" : map_type(return_type);
    }

    vector<string> param_strs;

    for (const auto& [ptype, pname] : params) {
        param_strs.push_back(fmt::format("{} {}", map_type(ptype), pname));
    }

    string out = fmt::format("\t{} {}({}) {{\n", rt, name, fmt::join(param_strs, ", "));

    for (const auto& stmt : body_stmts) {
        out += fmt::format("\t\t{}\n", stmt);
    }

    // Async methods need at least one co_await/co_return to be recognized as coroutines
    if (is_async && return_type.empty()) {
        out += "\t\tco_return;\n";
    }

    out += "\t}\n";
    return out;
}

/**
 * Generates a C++ function from a Nog FunctionDef.
 * Maps Nog types to C++ types and handles main() specially.
 * Async main is wrapped with io_context runner.
 */
string generate_function(CodeGenState& state, const FunctionDef& fn) {
    bool is_main = (fn.name == "main" && !state.test_mode);
    bool is_async_main = is_main && fn.is_async;

    state.in_async_function = fn.is_async;

    vector<FunctionParam> params;

    for (const auto& p : fn.params) {
        params.push_back({p.type, p.name});
    }

    vector<string> body;

    for (const auto& stmt : fn.body) {
        body.push_back(generate_statement(state, *stmt));
    }

    string out;

    if (is_async_main) {
        // Generate the async function as _async_main()
        string rt_type = fn.return_type.empty() ? "" : fn.return_type;
        out = function_def("_async_main", params, rt_type, body, true);

        // Generate int main() that runs the coroutine
        out += "\nint main() {\n";
        out += "\tasio::io_context io_context;\n";
        out += "\tasio::co_spawn(io_context, _async_main(), asio::detached);\n";
        out += "\tio_context.run();\n";
        out += "\treturn 0;\n";
        out += "}\n";
    } else {
        string rt_type = is_main ? "int" : fn.return_type;
        out = function_def(fn.name, params, rt_type, body, fn.is_async);

        // For main without explicit return, add return 0
        if (is_main && fn.return_type.empty()) {
            auto pos = out.rfind("}\n");

            if (pos != string::npos) {
                out.insert(pos, "\treturn 0;\n");
            }
        }
    }

    state.in_async_function = false;
    return out;
}

/**
 * Generates a C++ member function from a Nog MethodDef.
 * Transforms self.field into this->field.
 */
string generate_method(CodeGenState& state, const MethodDef& method) {
    // Track whether we're in an async method
    state.in_async_function = method.is_async;

    // Skip 'self' parameter for C++ method (it becomes 'this')
    vector<pair<string, string>> params;

    for (size_t i = 1; i < method.params.size(); i++) {
        params.push_back({method.params[i].type, method.params[i].name});
    }

    vector<string> body;

    for (const auto& stmt : method.body) {
        body.push_back(generate_statement(state, *stmt));
    }

    state.in_async_function = false;

    return method_def(method.name, params, method.return_type, body, method.is_async);
}

/**
 * Checks if any function in the program is async.
 */
static bool has_async_functions(const Program& program) {
    for (const auto& fn : program.functions) {
        if (fn->is_async) {
            return true;
        }
    }

    return false;
}

/**
 * Checks if the program uses channels.
 */
static bool has_channels(const Program& program) {
    return has_async_functions(program);
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

    if (has_async_functions(*program)) {
        out += "#include <asio.hpp>\n#include <asio/awaitable.hpp>\n";
    }

    if (has_channels(*program)) {
        out += "#include <asio/experimental/channel.hpp>\n";
        out += "#include <asio/experimental/awaitable_operators.hpp>\n";
        out += "using namespace asio::experimental::awaitable_operators;\n";
    }

    out += "\n";

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

    vector<pair<string, bool>> test_funcs;

    for (const auto& fn : program->functions) {
        if (fn->name.rfind("test_", 0) == 0) {
            test_funcs.push_back({fn->name, fn->is_async});
        }

        out += generate_function(state, *fn);
    }

    out += "\nint main() {\n";

    bool has_async_tests = false;
    for (const auto& [_, is_async] : test_funcs) {
        if (is_async) {
            has_async_tests = true;
            break;
        }
    }

    if (has_async_tests) {
        out += "\tasio::io_context io_context;\n";
    }

    for (const auto& [name, is_async] : test_funcs) {
        if (is_async) {
            out += "\tasio::co_spawn(io_context, " + name + "(), asio::detached);\n";
        } else {
            out += "\t" + name + "();\n";
        }
    }

    if (has_async_tests) {
        out += "\tio_context.run();\n";
    }

    out += "\treturn _failures;\n";
    out += "}\n";

    return out;
}

} // namespace codegen
