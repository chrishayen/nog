/**
 * @file main.cpp
 * @brief Entry point for the Bishop compiler.
 *
 * Provides the main() function and CLI handling for the Bishop compiler.
 * Supports two modes:
 *   - bishop <source.b>     : Transpile to C++ and output to stdout
 *   - bishop test <path>    : Run tests on .b files, compiling and executing them
 */

#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <cstdlib>
#include <set>
#include <thread>
#include <future>
#include <mutex>
#include <atomic>
#include "lexer/lexer.hpp"
#include "parser/parser.hpp"
#include "codegen/codegen.hpp"
#include "typechecker/typechecker.hpp"
#include "project/project.hpp"
#include "project/module.hpp"

using namespace std;
namespace fs = filesystem;

/**
 * Result of transpiling a Bishop source file.
 * Contains both the generated C++ code and metadata about module usage.
 */
struct TranspileResult {
    string cpp_code;           ///< Generated C++ source code
    bool uses_http = false;    ///< True if http module is imported
    bool uses_fs = false;      ///< True if fs module is imported
    set<string> extern_libs;   ///< Libraries needed by extern functions
};

/**
 * Gets the directory where the bishop executable is located.
 * Used to find runtime libraries relative to the compiler.
 */
fs::path get_executable_dir() {
    return fs::read_symlink("/proc/self/exe").parent_path();
}

/**
 * Gets the library and include paths based on where bishop is installed.
 * Development: build/bishop -> build/lib, build/include
 * Installed: ~/.local/bin/bishop -> ~/.local/lib/bishop, ~/.local/include
 *
 * Include path points to parent so #include <bishop/http.hpp> works.
 */
pair<fs::path, fs::path> get_runtime_paths() {
    fs::path exe_dir = get_executable_dir();

    // Check if we're in a build directory (has lib/ subdirectory)
    fs::path build_lib = exe_dir / "lib";
    fs::path build_include = exe_dir / "include";

    if (fs::exists(build_lib) && fs::exists(build_include)) {
        return {build_lib, build_include};
    }

    // Otherwise assume installed layout: bin/bishop -> lib/bishop, include
    // Include path is ~/.local/include (so bishop/http.hpp is found)
    // Lib path is ~/.local/lib/bishop (where our libs are)
    fs::path install_base = exe_dir.parent_path();
    return {install_base / "lib" / "bishop", install_base / "include"};
}

/**
 * Builds the g++ compile command (source to object file).
 * Uses ccache for caching compiled objects.
 *
 * Precompiled headers: GCC automatically uses .gch files when found
 * alongside the .hpp file in the include path.
 */
string build_compile_cmd(const string& obj_output, const string& input) {
    auto [lib_path, include_path] = get_runtime_paths();
    string cmd = "CCACHE_SLOPPINESS=pch_defines,time_macros CCACHE_DEPEND=1 ccache g++ -std=c++23 -pipe -c -MD -o " + obj_output + " " + input;
    cmd += " -I" + include_path.string();
    cmd += " 2>&1";
    return cmd;
}

/**
 * Builds the g++ link command (object file to executable).
 * Adds runtime libraries based on which modules are used.
 *
 * When static_link is true, links boost statically for portable binaries.
 * When false, uses dynamic linking for faster dev builds.
 */
string build_link_cmd(const TranspileResult& result, const string& exe_output,
                      const string& obj_input, bool static_link = false) {
    auto [lib_path, include_path] = get_runtime_paths();
    string cmd = "g++ -pipe -o " + exe_output + " " + obj_input;

    // Always add library path and link bishop_std_runtime (contains fiber runtime)
    cmd += " -L" + lib_path.string();
    cmd += " -lbishop_std_runtime";

    if (result.uses_http) {
        cmd += " -lbishop_http_runtime";
        cmd += " -lllhttp";
    }

    // Add extern library flags (skip "c" as libc is implicit)
    for (const auto& lib : result.extern_libs) {
        if (lib != "c") {
            cmd += " -l" + lib;
        }
    }

    // Link boost_fiber and boost_context for fiber-based concurrency
    if (static_link) {
        // Static linking for portable standalone binaries
        cmd += " -l:libboost_fiber.a -l:libboost_context.a -lpthread";
    } else {
        // Dynamic linking for faster dev builds
        cmd += " -lboost_fiber -lboost_context -lpthread";
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
 * Transpiles Bishop source to C++ code.
 * Runs lexer -> parser -> module loading -> type checker -> code generator pipeline.
 * Returns result with empty cpp_code and prints errors if type checking or module loading fails.
 */
TranspileResult transpile(const string& source, const string& filename, bool test_mode) {
    TranspileResult result;

    Lexer lexer(source);
    auto tokens = lexer.tokenize();
    ParserState state(tokens);

    unique_ptr<Program> ast;

    try {
        ast = parser::parse(state);
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

    // Collect libraries from extern functions
    for (const auto& ext : ast->externs) {
        result.extern_libs.insert(ext->library);
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
        cerr << filename << ": error: imports require a bishop.toml file (run 'bishop init')" << endl;
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
 * Result of running a single test.
 */
struct TestResult {
    fs::path file;
    bool passed;
    string message;
};

/**
 * Runs a single positive test (expects success).
 * Thread-safe: uses unique temp files based on test_id.
 */
TestResult run_positive_test(const fs::path& test_file, int test_id) {
    TestResult result{test_file, false, ""};

    string source = read_file(test_file.string());

    if (source.empty()) {
        result.message = "could not read file";
        return result;
    }

    TranspileResult transpile_result = transpile(source, test_file.string(), true);

    if (transpile_result.cpp_code.empty()) {
        result.message = "type errors";
        return result;
    }

    string tmp_cpp = "/tmp/bishop_test_" + to_string(test_id) + ".cpp";
    string tmp_obj = "/tmp/bishop_test_" + to_string(test_id) + ".o";
    string tmp_bin = "/tmp/bishop_test_" + to_string(test_id);

    ofstream out(tmp_cpp);
    out << transpile_result.cpp_code;
    out.close();

    string compile_cmd = build_compile_cmd(tmp_obj, tmp_cpp);

    if (system(compile_cmd.c_str()) != 0) {
        result.message = "compile failed";
        return result;
    }

    string link_cmd = build_link_cmd(transpile_result, tmp_bin, tmp_obj);

    if (system(link_cmd.c_str()) != 0) {
        result.message = "link failed";
        return result;
    }

    string run_cmd = tmp_bin + " 2>&1";
    int test_status = system(run_cmd.c_str());

    if (test_status != 0) {
        result.message = "test failed";
        return result;
    }

    result.passed = true;
    return result;
}

/// Mutex to protect cerr redirection in negative tests
static mutex cerr_mutex;

/**
 * Runs a single negative test (expects failure with specific error).
 * Thread-safe: uses mutex to protect cerr redirection.
 */
TestResult run_negative_test(const fs::path& test_file) {
    TestResult result{test_file, false, ""};

    string source = read_file(test_file.string());

    if (source.empty()) {
        result.message = "could not read file";
        return result;
    }

    string error_output;
    {
        // Protect cerr redirection with mutex
        lock_guard<mutex> lock(cerr_mutex);
        stringstream error_capture;
        streambuf* old_cerr = cerr.rdbuf(error_capture.rdbuf());

        TranspileResult transpile_result = transpile(source, test_file.string(), false);

        cerr.rdbuf(old_cerr);
        error_output = error_capture.str();

        if (!transpile_result.cpp_code.empty()) {
            result.message = "expected error, but compiled";
            return result;
        }
    }

    // Extract expected error from filename (replace underscores with spaces)
    string expected_error = test_file.stem().string();

    for (char& c : expected_error) {
        if (c == '_') {
            c = ' ';
        }
    }

    if (error_output.find(expected_error) == string::npos) {
        result.message = "expected '" + expected_error + "', got: " + error_output;
        return result;
    }

    result.passed = true;
    return result;
}

/**
 * Runs tests on all .b files in a directory (or a single file).
 * Each test file is transpiled, compiled with g++, and executed.
 * Test functions (test_*) use assert_eq for assertions.
 * Files in tests/errors/ are expected to fail with specific error messages.
 * Uses parallel execution for faster test runs.
 * Returns 0 if all tests pass, 1 if any fail.
 */
int run_tests(const string& path) {
    vector<fs::path> test_files;
    vector<fs::path> error_test_files;

    if (fs::is_directory(path)) {
        for (const auto& entry : fs::recursive_directory_iterator(path)) {
            if (entry.path().extension() == ".b") {
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
        cerr << "No .b files found" << endl;
        return 1;
    }

    // Launch positive tests in parallel
    vector<future<TestResult>> positive_futures;

    for (size_t i = 0; i < test_files.size(); i++) {
        positive_futures.push_back(
            async(launch::async, run_positive_test, test_files[i], static_cast<int>(i))
        );
    }

    // Launch negative tests in parallel
    vector<future<TestResult>> negative_futures;

    for (const auto& test_file : error_test_files) {
        negative_futures.push_back(
            async(launch::async, run_negative_test, test_file)
        );
    }

    // Collect and print results
    int total_failures = 0;

    for (auto& fut : positive_futures) {
        TestResult result = fut.get();

        if (result.passed) {
            cout << "\033[32mPASS\033[0m " << result.file.string() << endl;
        } else {
            cout << "\033[31mFAIL\033[0m " << result.file.string()
                 << " (" << result.message << ")" << endl;
            total_failures++;
        }
    }

    for (auto& fut : negative_futures) {
        TestResult result = fut.get();

        if (result.passed) {
            cout << "\033[32mPASS\033[0m " << result.file.string() << endl;
        } else {
            cout << "\033[31mFAIL\033[0m " << result.file.string()
                 << " (" << result.message << ")" << endl;
            total_failures++;
        }
    }

    return total_failures > 0 ? 1 : 0;
}

/**
 * Initializes a new Bishop project by creating a bishop.toml file in the current directory.
 */
int init_project(const string& project_name) {
    if (project_name.empty()) {
        cerr << "Usage: bishop init <project_name>" << endl;
        return 1;
    }

    fs::path init_file = fs::current_path() / "bishop.toml";

    if (fs::exists(init_file)) {
        cerr << "Error: bishop.toml already exists" << endl;
        return 1;
    }

    ofstream out(init_file);

    if (!out) {
        cerr << "Error: Could not create bishop.toml" << endl;
        return 1;
    }

    out << "[project]" << endl;
    out << "name = \"" << project_name << "\"" << endl;

    cout << "Initialized project '" << project_name << "'" << endl;
    return 0;
}

/**
 * Compiles and runs a bishop source file or project directory.
 */
int run_file(const string& path) {
    string filename;

    if (fs::is_directory(path)) {
        fs::path dir_path = fs::absolute(path);
        fs::path toml_path = dir_path / "bishop.toml";

        if (!fs::exists(toml_path)) {
            cerr << "Error: No bishop.toml found in " << path << endl;
            return 1;
        }

        auto config = parse_init_file(toml_path);

        if (!config) {
            cerr << "Error: Could not parse bishop.toml" << endl;
            return 1;
        }

        if (!config->entry) {
            cerr << "Error: No entry field in bishop.toml" << endl;
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
    string cpp_file = "/tmp/bishop_run.cpp";
    string obj_file = "/tmp/bishop_run.o";
    string exe_file = "/tmp/bishop_run";

    ofstream out(cpp_file);

    if (!out) {
        cerr << "Error: Could not create temp file" << endl;
        return 1;
    }

    out << result.cpp_code;
    out.close();

    // Compile (cached)
    string compile_cmd = build_compile_cmd(obj_file, cpp_file);

    if (system(compile_cmd.c_str()) != 0) {
        cerr << "Compile failed" << endl;
        return 1;
    }

    // Link
    string link_cmd = build_link_cmd(result, exe_file, obj_file);

    if (system(link_cmd.c_str()) != 0) {
        cerr << "Link failed" << endl;
        return 1;
    }

    // Run (static linking, no LD_LIBRARY_PATH needed)
    return system(exe_file.c_str());
}

/**
 * Builds a bishop source file to an executable.
 * If a directory is provided, looks for bishop.toml with entry field.
 */
int build_file(const string& path) {
    string filename;
    string exe_name;

    if (fs::is_directory(path)) {
        fs::path dir_path = fs::absolute(path);
        fs::path toml_path = dir_path / "bishop.toml";

        if (!fs::exists(toml_path)) {
            cerr << "Error: No bishop.toml found in " << path << endl;
            return 1;
        }

        auto config = parse_init_file(toml_path);

        if (!config) {
            cerr << "Error: Could not parse bishop.toml" << endl;
            return 1;
        }

        if (!config->entry) {
            cerr << "Error: No entry field in bishop.toml" << endl;
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

    string cpp_file = "/tmp/bishop_build.cpp";
    string obj_file = "/tmp/bishop_build.o";
    ofstream out(cpp_file);

    if (!out) {
        cerr << "Error: Could not create temp file" << endl;
        return 1;
    }

    out << result.cpp_code;
    out.close();

    // Compile (cached)
    string compile_cmd = build_compile_cmd(obj_file, cpp_file);

    if (system(compile_cmd.c_str()) != 0) {
        cerr << "Compile failed" << endl;
        return 1;
    }

    // Link with static boost for standalone binary
    string link_cmd = build_link_cmd(result, exe_name, obj_file, true);

    if (system(link_cmd.c_str()) != 0) {
        cerr << "Link failed" << endl;
        return 1;
    }

    return 0;
}

/**
 * Main entry point. Usage:
 *   bishop <file|dir>       - Build executable from source file or project directory
 *   bishop run <file|dir>   - Build and run
 *   bishop test <path>      - Run tests
 *   bishop init <name>      - Initialize project
 */
int main(int argc, char* argv[]) {
    if (argc < 2) {
        cerr << "Usage: bishop <file|dir>" << endl;
        cerr << "       bishop run <file|dir>" << endl;
        cerr << "       bishop test <path>" << endl;
        cerr << "       bishop init <name>" << endl;
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
            cerr << "Usage: bishop run <file>" << endl;
            return 1;
        }

        return run_file(argv[2]);
    }

    return build_file(cmd);
}
