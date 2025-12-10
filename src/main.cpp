#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <cstdlib>
#include "lexer.hpp"
#include "parser.hpp"
#include "codegen.hpp"

using namespace std;
namespace fs = filesystem;

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

string transpile(const string& source, bool test_mode) {
    Lexer lexer(source);
    auto tokens = lexer.tokenize();
    Parser parser(tokens);
    auto ast = parser.parse();
    CodeGen codegen;
    return codegen.generate(ast, test_mode);
}

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

        string cpp_code = transpile(source, true);
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

int main(int argc, char* argv[]) {
    if (argc < 2) {
        cerr << "Usage: nog <source.n>" << endl;
        cerr << "       nog test <path>" << endl;
        return 1;
    }

    string cmd = argv[1];

    if (cmd == "test") {
        string path = argc >= 3 ? argv[2] : "tests/";
        return run_tests(path);
    }

    string source = read_file(cmd);
    if (source.empty()) return 1;

    cout << transpile(source, false);
    return 0;
}
