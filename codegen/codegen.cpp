/**
 * @file codegen.cpp
 * @brief Main C++ code generation entry points for the Nog language.
 *
 * Contains the generate() and generate_with_imports() functions that
 * orchestrate the code generation process.
 */

/**
 * @nog_fn print
 * @module builtins
 * @description Prints values to standard output, followed by a newline.
 * @param args ... - One or more values to print (separated by spaces)
 * @example
 * print("Hello, World!");
 * print("x =", x, "y =", y);
 */

/**
 * @nog_fn assert_eq
 * @module builtins
 * @description Asserts that two values are equal. Only available in test mode.
 * @param expected T - The expected value
 * @param actual T - The actual value to compare
 * @note Fails the test with line number and values if not equal
 * @example
 * assert_eq(5, add(2, 3));
 * assert_eq("hello", greet());
 */

/**
 * @nog_struct Channel
 * @module builtins
 * @description A typed channel for communication between async functions.
 * @example
 * ch := Channel<int>();
 * await ch.send(42);
 * val := await ch.recv();
 */

/**
 * @nog_method send
 * @type Channel
 * @async
 * @description Sends a value through the channel.
 * @param value T - The value to send
 * @example await ch.send(42);
 */

/**
 * @nog_method recv
 * @type Channel
 * @async
 * @description Receives a value from the channel.
 * @returns T - The received value
 * @example val := await ch.recv();
 */

#include "codegen.hpp"
#include "stdlib/http.hpp"
#include "stdlib/fs.hpp"
#include <fmt/format.h>
#include <fmt/ranges.h>

using namespace std;

namespace codegen {

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
 * Checks if the program uses channels (simple heuristic: async functions assumed to use channels).
 */
static bool has_channels(const Program& program) {
    return has_async_functions(program);
}

/**
 * Checks if any function has function type parameters (requires <functional>).
 */
static bool has_function_types(const Program& program) {
    for (const auto& fn : program.functions) {
        for (const auto& param : fn->params) {
            if (param.type.rfind("fn(", 0) == 0) {
                return true;
            }
        }
    }

    for (const auto& m : program.methods) {
        for (const auto& param : m->params) {
            if (param.type.rfind("fn(", 0) == 0) {
                return true;
            }
        }
    }

    return false;
}

/**
 * Checks if the program imports the http module.
 */
static bool has_http_import(const map<string, const Module*>& imports) {
    return imports.find("http") != imports.end();
}

/**
 * Checks if the program imports the fs module.
 */
static bool has_fs_import(const map<string, const Module*>& imports) {
    return imports.find("fs") != imports.end();
}

/**
 * Generates a C++ namespace for an imported module.
 */
string generate_module_namespace(CodeGenState& state, const string& name, const Module& module) {
    if (name == "http") {
        return nog::stdlib::generate_http_runtime();
    }

    if (name == "fs") {
        return nog::stdlib::generate_fs_runtime();
    }

    string out = "namespace " + name + " {\n\n";

    const Program* saved_program = state.current_program;
    state.current_program = module.ast.get();

    for (const auto* s : module.get_public_structs()) {
        out += generate_struct(state, *s) + "\n\n";
    }

    for (const auto* f : module.get_public_functions()) {
        out += generate_function(state, *f);
    }

    state.current_program = saved_program;

    out += "} // namespace " + name + "\n\n";
    return out;
}

/**
 * Main code generation entry point. Generates complete C++ source.
 */
string generate(CodeGenState& state, const unique_ptr<Program>& program, bool test_mode) {
    state.test_mode = test_mode;
    state.current_program = program.get();
    state.imported_modules.clear();

    for (const auto& ext : program->externs) {
        state.extern_functions[ext->name] = ext.get();
    }

    if (test_mode) {
        return generate_test_harness(state, program);
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

    out += generate_extern_declarations(program);

    for (const auto& s : program->structs) {
        out += generate_struct(state, *s) + "\n\n";
    }

    for (const auto& fn : program->functions) {
        out += generate_function(state, *fn);
    }

    return out;
}

/**
 * Generates C++ code for a program with imported modules.
 */
string generate_with_imports(
    CodeGenState& state,
    const unique_ptr<Program>& program,
    const map<string, const Module*>& imports,
    bool test_mode
) {
    state.test_mode = test_mode;
    state.current_program = program.get();
    state.imported_modules = imports;

    for (const auto& ext : program->externs) {
        state.extern_functions[ext->name] = ext.get();
    }

    string out;

    if (has_http_import(imports)) {
        out += "#include <nog/http.hpp>\n";
        out += "using namespace asio::experimental::awaitable_operators;\n";
        out += "\n";
    } else {
        out += "#include <nog/std.hpp>\n";

        if (has_async_functions(*program)) {
            out += "#include <asio.hpp>\n#include <asio/awaitable.hpp>\n";
        }

        if (has_channels(*program)) {
            out += "#include <asio/experimental/channel.hpp>\n";
            out += "#include <asio/experimental/awaitable_operators.hpp>\n";
            out += "using namespace asio::experimental::awaitable_operators;\n";
        }

        out += "\n";
    }

    if (has_fs_import(imports)) {
        out += "#include <nog/fs.hpp>\n\n";
    }

    out += generate_extern_declarations(program);

    if (test_mode) {
        out += "int _failures = 0;\n\n";
        out += "template<typename T, typename U>\n";
        out += "void _assert_eq(T a, U b, int line) {\n";
        out += "\tif (a != b) {\n";
        out += "\t\tstd::cerr << \"line \" << line << \": FAIL: \" << a << \" != \" << b << std::endl;\n";
        out += "\t\t_failures++;\n";
        out += "\t}\n";
        out += "}\n\n";
    }

    for (const auto& [alias, mod] : imports) {
        out += generate_module_namespace(state, alias, *mod);
    }

    for (const auto& s : program->structs) {
        out += generate_struct(state, *s) + "\n\n";
    }

    for (const auto& fn : program->functions) {
        out += generate_function(state, *fn);
    }

    if (test_mode) {
        vector<pair<string, bool>> test_funcs;

        for (const auto& fn : program->functions) {
            if (fn->name.rfind("test_", 0) == 0) {
                test_funcs.push_back({fn->name, fn->is_async});
            }
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
    }

    return out;
}

} // namespace codegen

// Legacy class API for backwards compatibility
string CodeGen::generate(const unique_ptr<Program>& program, bool test_mode) {
    CodeGenState state;
    return codegen::generate(state, program, test_mode);
}

string CodeGen::generate_with_imports(
    const unique_ptr<Program>& program,
    const map<string, const Module*>& imports,
    bool test_mode
) {
    CodeGenState state;
    return codegen::generate_with_imports(state, program, imports, test_mode);
}
