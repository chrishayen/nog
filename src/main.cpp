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
#include "lexer.hpp"
#include "parser.hpp"
#include "codegen.hpp"
#include "typechecker.hpp"
#include "project.hpp"
#include "module.hpp"

using namespace std;
namespace fs = filesystem;

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
 * Returns empty string and prints errors if type checking or module loading fails.
 */
string transpile(const string& source, const string& filename, bool test_mode) {
    Lexer lexer(source);
    auto tokens = lexer.tokenize();
    Parser parser(tokens);
    auto ast = parser.parse();

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

                return "";
            }

            imports[imp->alias] = mod;
        }
    } else if (!ast->imports.empty()) {
        cerr << filename << ": error: imports require a nog.toml file (run 'nog init')" << endl;
        return "";
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

        return "";
    }

    // Generate code with imports
    CodeGen codegen;

    if (imports.empty()) {
        return codegen.generate(ast, test_mode);
    }

    return codegen.generate_with_imports(ast, imports, test_mode);
}

/**
 * Runs tests on all .n files in a directory (or a single file).
 * Each test file is transpiled, compiled with g++, and executed.
 * Test functions (test_*) use assert_eq for assertions.
 * Returns 0 if all tests pass, 1 if any fail.
 */
int run_tests(const string& path) {
    vector<fs::path> test_files;

    if (fs::is_directory(path)) {
        for (const auto& entry : fs::recursive_directory_iterator(path)) {
            if (entry.path().extension() == ".n") {
                test_files.push_back(entry.path());
            }
        }
    } else if (fs::exists(path)) {
        test_files.push_back(path);
    } else {
        cerr << "Error: Path does not exist: " << path << endl;
        return 1;
    }

    if (test_files.empty()) {
        cerr << "No .n files found" << endl;
        return 1;
    }

    int total_failures = 0;

    for (const auto& test_file : test_files) {
        string source = read_file(test_file.string());
        if (source.empty()) continue;

        string cpp_code = transpile(source, test_file.string(), true);

        if (cpp_code.empty()) {
            cout << "\033[31mFAIL\033[0m " << test_file.string() << " (type errors)" << endl;
            total_failures++;
            continue;
        }

        string tmp_cpp = "/tmp/nog_test.cpp";
        string tmp_bin = "/tmp/nog_test";

        ofstream out(tmp_cpp);
        out << cpp_code;
        out.close();

        string compile_cmd = "g++ -o " + tmp_bin + " " + tmp_cpp + " 2>&1";
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

    return total_failures > 0 ? 1 : 0;
}

/**
 * Initializes a new Nog project by creating a nog.toml file.
 * Uses the directory name as the project name.
 */
int init_project(const string& path) {
    fs::path project_path = path.empty() ? fs::current_path() : fs::path(path);

    if (!fs::is_directory(project_path)) {
        cerr << "Error: " << project_path << " is not a directory" << endl;
        return 1;
    }

    if (create_init_file(project_path)) {
        cout << "Initialized project '" << project_path.filename().string() << "'" << endl;
        return 0;
    }

    fs::path init_file = project_path / "nog.toml";

    if (fs::exists(init_file)) {
        cerr << "Error: nog.toml already exists" << endl;
    } else {
        cerr << "Error: Could not create nog.toml" << endl;
    }

    return 1;
}

/**
 * Main entry point. Usage:
 *   nog <source.n>   - Transpile to C++ and print to stdout
 *   nog test <path>  - Run tests in directory or single file
 *   nog init [path]  - Initialize a new project
 */
int main(int argc, char* argv[]) {
    if (argc < 2) {
        cerr << "Usage: nog <source.n>" << endl;
        cerr << "       nog test <path>" << endl;
        cerr << "       nog init [path]" << endl;
        return 1;
    }

    string cmd = argv[1];

    if (cmd == "test") {
        string path = argc >= 3 ? argv[2] : "tests/";
        return run_tests(path);
    }

    if (cmd == "init") {
        string path = argc >= 3 ? argv[2] : "";
        return init_project(path);
    }

    string source = read_file(cmd);
    if (source.empty()) return 1;

    string result = transpile(source, cmd, false);

    if (result.empty()) {
        return 1;
    }

    cout << result;
    return 0;
}
