/**
 * @file main.cpp
 * @brief Entry point for the Nog compiler.
 *
 * Provides the main() function and CLI handling for the Nog compiler.
 * Supports two modes:
 *   - nog <source.n>     : Transpile to C++ and output to stdout
 *   - nog test <path>    : Run tests on .n files, compiling and executing them
 */

#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <cstdlib>
#include "lexer/lexer.hpp"
#include "parser/parser.hpp"
#include "codegen/codegen.hpp"
#include "typechecker/typechecker.hpp"
#include "project/project.hpp"
#include "project/module.hpp"

using namespace std;
namespace fs = filesystem;

/**
 * Result of transpiling a Nog source file.
 * Contains both the generated C++ code and metadata about module usage.
 */
struct TranspileResult {
    string cpp_code;        ///< Generated C++ source code
    bool uses_http = false; ///< True if http module is imported
    bool uses_fs = false;   ///< True if fs module is imported
};

/**
 * Gets the directory where the nog executable is located.
 * Used to find runtime libraries relative to the compiler.
 */
fs::path get_executable_dir() {
    return fs::read_symlink("/proc/self/exe").parent_path();
}

/**
 * Gets the library and include paths based on where nog is installed.
 * Development: build/nog -> build/lib, build/include
 * Installed: ~/.local/bin/nog -> ~/.local/lib/nog, ~/.local/include
 *
 * Include path points to parent so #include <nog/http.hpp> works.
 */
pair<fs::path, fs::path> get_runtime_paths() {
    fs::path exe_dir = get_executable_dir();

    // Check if we're in a build directory (has lib/ subdirectory)
    fs::path build_lib = exe_dir / "lib";
    fs::path build_include = exe_dir / "include";

    if (fs::exists(build_lib) && fs::exists(build_include)) {
        return {build_lib, build_include};
    }

    // Otherwise assume installed layout: bin/nog -> lib/nog, include
    // Include path is ~/.local/include (so nog/http.hpp is found)
    // Lib path is ~/.local/lib/nog (where our libs are)
    fs::path install_base = exe_dir.parent_path();
    return {install_base / "lib" / "nog", install_base / "include"};
}

/**
 * Builds the g++ compile command with appropriate flags.
 * Adds runtime libraries based on which modules are used.
 *
 * Precompiled headers: GCC automatically uses .gch files when found
 * alongside the .hpp file in the include path.
 */
string build_compile_cmd(const TranspileResult& result, const string& output, const string& input) {
    string cmd = "g++ -std=c++23 -o " + output + " " + input;

    // Always add include path for std.hpp PCH
    auto [lib_path, include_path] = get_runtime_paths();
    cmd += " -I" + include_path.string();

    if (result.uses_http) {
        cmd += " -L" + lib_path.string();
        cmd += " -lnog_http_runtime";
        cmd += " -lllhttp";
    }

    cmd += " 2>&1";
    return cmd;
}

/**
 * Reads the entire contents of a file into a string.
 * Returns empty string and prints error if file cannot be opened.
 */
string read_file(const string& path) {
    ifstream file(path);
    if (!file) {
        cerr << "Error: Could not open file " << path << endl;
        return "";
    }
    stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

/**
 * Transpiles Nog source to C++ code.
 * Runs lexer -> parser -> module loading -> type checker -> code generator pipeline.
 * Returns result with empty cpp_code and prints errors if type checking or module loading fails.
 */
TranspileResult transpile(const string& source, const string& filename, bool test_mode) {
    TranspileResult result;

    Lexer lexer(source);
    auto tokens = lexer.tokenize();
    Parser parser(tokens);

    unique_ptr<Program> ast;

    try {
        ast = parser.parse();
    } catch (const runtime_error& e) {
        cerr << filename << ": parse error: " << e.what() << endl;
        return result;
    }

    // Detect which builtin modules are used from the parsed AST
    for (const auto& imp : ast->imports) {
        if (imp->module_path == "http") {
            result.uses_http = true;
        } else if (imp->module_path == "fs") {
            result.uses_fs = true;
        }
    }

    // Find project configuration (for module resolution)
    auto config = find_project(fs::path(filename));

    // Load imported modules if we have a project config
    map<string, const Module*> imports;
    unique_ptr<ModuleManager> module_manager;

    if (config && !ast->imports.empty()) {
        module_manager = make_unique<ModuleManager>(*config);

        for (const auto& imp : ast->imports) {
            const Module* mod = module_manager->load_module(imp->module_path);

            if (!mod) {
                for (const auto& err : module_manager->get_errors()) {
                    cerr << filename << ": error: " << err << endl;
                }

                return result;
            }

            imports[imp->alias] = mod;
        }
    } else if (!ast->imports.empty()) {
        cerr << filename << ": error: imports require a nog.toml file (run 'nog init')" << endl;
        return result;
    }

    // Type check with imports
    TypeChecker checker;

    for (const auto& [alias, mod] : imports) {
        checker.register_module(alias, *mod);
    }

    if (!checker.check(*ast, filename)) {
        for (const auto& err : checker.get_errors()) {
            cerr << err.filename << ":" << err.line << ": error: " << err.message << endl;
        }

        return result;
    }

    // Generate code with imports
    CodeGen codegen;

    if (imports.empty()) {
        result.cpp_code = codegen.generate(ast, test_mode);
    } else {
        result.cpp_code = codegen.generate_with_imports(ast, imports, test_mode);
    }

    return result;
}

/**
 * Checks if a path component is "errors" directory.
 * Used to identify negative test files.
 */
bool is_error_test(const fs::path& path) {
    for (const auto& part : path) {
        if (part == "errors") {
            return true;
        }
    }

    return false;
}

/**
 * Runs tests on all .n files in a directory (or a single file).
 * Each test file is transpiled, compiled with g++, and executed.
 * Test functions (test_*) use assert_eq for assertions.
 * Files in tests/errors/ are expected to fail with specific error messages.
 * Returns 0 if all tests pass, 1 if any fail.
 */
int run_tests(const string& path) {
    vector<fs::path> test_files;
    vector<fs::path> error_test_files;

    if (fs::is_directory(path)) {
        for (const auto& entry : fs::recursive_directory_iterator(path)) {
            if (entry.path().extension() == ".n") {
                if (is_error_test(entry.path())) {
                    error_test_files.push_back(entry.path());
                } else {
                    test_files.push_back(entry.path());
                }
            }
        }
    } else if (fs::exists(path)) {
        if (is_error_test(path)) {
            error_test_files.push_back(path);
        } else {
            test_files.push_back(path);
        }
    } else {
        cerr << "Error: Path does not exist: " << path << endl;
        return 1;
    }

    if (test_files.empty() && error_test_files.empty()) {
        cerr << "No .n files found" << endl;
        return 1;
    }

    int total_failures = 0;

    // Run positive tests (expect compilation to succeed)
    for (const auto& test_file : test_files) {
        string source = read_file(test_file.string());
        if (source.empty()) continue;

        TranspileResult result = transpile(source, test_file.string(), true);

        if (result.cpp_code.empty()) {
            cout << "\033[31mFAIL\033[0m " << test_file.string() << " (type errors)" << endl;
            total_failures++;
            continue;
        }

        string tmp_cpp = "/tmp/nog_test.cpp";
        string tmp_bin = "/tmp/nog_test";

        ofstream out(tmp_cpp);
        out << result.cpp_code;
        out.close();

        string compile_cmd = build_compile_cmd(result, tmp_bin, tmp_cpp);
        int compile_result = system(compile_cmd.c_str());

        if (compile_result != 0) {
            cerr << "Compile failed for " << test_file << endl;
            total_failures++;
            continue;
        }

        int test_result = system(tmp_bin.c_str());

        if (test_result != 0) {
            cout << "\033[31mFAIL\033[0m " << test_file.string() << endl;
            total_failures++;
        } else {
            cout << "\033[32mPASS\033[0m " << test_file.string() << endl;
        }
    }

    // Run negative tests (expect compilation to fail with specific error)
    for (const auto& test_file : error_test_files) {
        string source = read_file(test_file.string());
        if (source.empty()) continue;

        // Capture stderr to check error message
        stringstream error_capture;
        streambuf* old_cerr = cerr.rdbuf(error_capture.rdbuf());

        TranspileResult result = transpile(source, test_file.string(), false);

        cerr.rdbuf(old_cerr);
        string error_output = error_capture.str();

        // Extract expected error from filename (replace underscores with spaces)
        string expected_error = test_file.stem().string();

        for (char& c : expected_error) {
            if (c == '_') {
                c = ' ';
            }
        }

        if (!result.cpp_code.empty()) {
            cout << "\033[31mFAIL\033[0m " << test_file.string()
                 << " (expected error, but compiled)" << endl;
            total_failures++;
        } else if (error_output.find(expected_error) == string::npos) {
            cout << "\033[31mFAIL\033[0m " << test_file.string()
                 << " (expected '" << expected_error << "', got: " << error_output << ")" << endl;
            total_failures++;
        } else {
            cout << "\033[32mPASS\033[0m " << test_file.string() << endl;
        }
    }

    return total_failures > 0 ? 1 : 0;
}

/**
 * Initializes a new Nog project by creating a nog.toml file in the current directory.
 */
int init_project(const string& project_name) {
    if (project_name.empty()) {
        cerr << "Usage: nog init <project_name>" << endl;
        return 1;
    }

    fs::path init_file = fs::current_path() / "nog.toml";

    if (fs::exists(init_file)) {
        cerr << "Error: nog.toml already exists" << endl;
        return 1;
    }

    ofstream out(init_file);

    if (!out) {
        cerr << "Error: Could not create nog.toml" << endl;
        return 1;
    }

    out << "[project]" << endl;
    out << "name = \"" << project_name << "\"" << endl;

    cout << "Initialized project '" << project_name << "'" << endl;
    return 0;
}

/**
 * Compiles and runs a nog source file or project directory.
 */
int run_file(const string& path) {
    string filename;

    if (fs::is_directory(path)) {
        fs::path dir_path = fs::absolute(path);
        fs::path toml_path = dir_path / "nog.toml";

        if (!fs::exists(toml_path)) {
            cerr << "Error: No nog.toml found in " << path << endl;
            return 1;
        }

        auto config = parse_init_file(toml_path);

        if (!config) {
            cerr << "Error: Could not parse nog.toml" << endl;
            return 1;
        }

        if (!config->entry) {
            cerr << "Error: No entry field in nog.toml" << endl;
            return 1;
        }

        fs::path entry_path = dir_path / *config->entry;

        if (!fs::exists(entry_path)) {
            cerr << "Error: Entry file not found: " << *config->entry << endl;
            return 1;
        }

        filename = entry_path.string();
    } else {
        filename = path;
    }

    string source = read_file(filename);

    if (source.empty()) {
        return 1;
    }

    TranspileResult result = transpile(source, filename, false);

    if (result.cpp_code.empty()) {
        return 1;
    }

    // Write to temp file
    string cpp_file = "/tmp/nog_run.cpp";
    string exe_file = "/tmp/nog_run";

    ofstream out(cpp_file);

    if (!out) {
        cerr << "Error: Could not create temp file" << endl;
        return 1;
    }

    out << result.cpp_code;
    out.close();

    // Compile
    string compile_cmd = build_compile_cmd(result, exe_file, cpp_file);
    int compile_result = system(compile_cmd.c_str());

    if (compile_result != 0) {
        cerr << "Compile failed" << endl;
        return 1;
    }

    // Run (static linking, no LD_LIBRARY_PATH needed)
    return system(exe_file.c_str());
}

/**
 * Builds a nog source file to an executable.
 * If a directory is provided, looks for nog.toml with entry field.
 */
int build_file(const string& path) {
    string filename;
    string exe_name;

    if (fs::is_directory(path)) {
        fs::path dir_path = fs::absolute(path);
        fs::path toml_path = dir_path / "nog.toml";

        if (!fs::exists(toml_path)) {
            cerr << "Error: No nog.toml found in " << path << endl;
            return 1;
        }

        auto config = parse_init_file(toml_path);

        if (!config) {
            cerr << "Error: Could not parse nog.toml" << endl;
            return 1;
        }

        if (!config->entry) {
            cerr << "Error: No entry field in nog.toml" << endl;
            return 1;
        }

        fs::path entry_path = dir_path / *config->entry;

        if (!fs::exists(entry_path)) {
            cerr << "Error: Entry file not found: " << *config->entry << endl;
            return 1;
        }

        filename = entry_path.string();
        exe_name = config->name;
    } else {
        filename = path;
        exe_name = fs::path(path).stem().string();
    }

    string source = read_file(filename);

    if (source.empty()) {
        return 1;
    }

    TranspileResult result = transpile(source, filename, false);

    if (result.cpp_code.empty()) {
        return 1;
    }

    string cpp_file = "/tmp/nog_build.cpp";
    ofstream out(cpp_file);

    if (!out) {
        cerr << "Error: Could not create temp file" << endl;
        return 1;
    }

    out << result.cpp_code;
    out.close();

    // Build compile command
    string compile_cmd = build_compile_cmd(result, exe_name, cpp_file);

    // Remove 2>&1 suffix for build output visibility
    if (compile_cmd.size() > 5 && compile_cmd.substr(compile_cmd.size() - 5) == " 2>&1") {
        compile_cmd = compile_cmd.substr(0, compile_cmd.size() - 5);
    }

    if (system(compile_cmd.c_str()) != 0) {
        cerr << "Compile failed" << endl;
        return 1;
    }

    return 0;
}

/**
 * Main entry point. Usage:
 *   nog <file|dir>       - Build executable from source file or project directory
 *   nog run <file|dir>   - Build and run
 *   nog test <path>      - Run tests
 *   nog init <name>      - Initialize project
 */
int main(int argc, char* argv[]) {
    if (argc < 2) {
        cerr << "Usage: nog <file|dir>" << endl;
        cerr << "       nog run <file|dir>" << endl;
        cerr << "       nog test <path>" << endl;
        cerr << "       nog init <name>" << endl;
        return 1;
    }

    string cmd = argv[1];

    if (cmd == "test") {
        string path = argc >= 3 ? argv[2] : "tests/";
        return run_tests(path);
    }

    if (cmd == "init") {
        string name = argc >= 3 ? argv[2] : "";
        return init_project(name);
    }

    if (cmd == "run") {
        if (argc < 3) {
            cerr << "Usage: nog run <file>" << endl;
            return 1;
        }

        return run_file(argv[2]);
    }

    return build_file(cmd);
}
