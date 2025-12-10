#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <cstdlib>
#include "lexer.hpp"
#include "parser.hpp"
#include "codegen.hpp"

namespace fs = std::filesystem;

std::string read_file(const std::string& path) {
    std::ifstream file(path);
    if (!file) {
        std::cerr << "Error: Could not open file " << path << std::endl;
        return "";
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

std::string transpile(const std::string& source, bool test_mode) {
    Lexer lexer(source);
    auto tokens = lexer.tokenize();
    Parser parser(tokens);
    auto ast = parser.parse();
    CodeGen codegen;
    return codegen.generate(ast, test_mode);
}

int run_tests(const std::string& path) {
    std::vector<fs::path> test_files;

    if (fs::is_directory(path)) {
        for (const auto& entry : fs::recursive_directory_iterator(path)) {
            if (entry.path().extension() == ".n") {
                test_files.push_back(entry.path());
            }
        }
    } else if (fs::exists(path)) {
        test_files.push_back(path);
    } else {
        std::cerr << "Error: Path does not exist: " << path << std::endl;
        return 1;
    }

    if (test_files.empty()) {
        std::cerr << "No .n files found" << std::endl;
        return 1;
    }

    int total_failures = 0;

    for (const auto& test_file : test_files) {
        std::string source = read_file(test_file.string());
        if (source.empty()) continue;

        std::string cpp_code = transpile(source, true);

        std::string tmp_cpp = "/tmp/nog_test.cpp";
        std::string tmp_bin = "/tmp/nog_test";

        std::ofstream out(tmp_cpp);
        out << cpp_code;
        out.close();

        std::string compile_cmd = "g++ -o " + tmp_bin + " " + tmp_cpp + " 2>&1";
        int compile_result = std::system(compile_cmd.c_str());

        if (compile_result != 0) {
            std::cerr << "Compile failed for " << test_file << std::endl;
            total_failures++;
            continue;
        }

        std::cout << test_file.string() << ": ";
        std::cout.flush();

        int test_result = std::system(tmp_bin.c_str());
        if (test_result != 0) {
            total_failures++;
        }
    }

    return total_failures > 0 ? 1 : 0;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: nog <source.n>" << std::endl;
        std::cerr << "       nog test <path>" << std::endl;
        return 1;
    }

    std::string cmd = argv[1];

    if (cmd == "test") {
        if (argc < 3) {
            std::cerr << "Usage: nog test <file or directory>" << std::endl;
            return 1;
        }
        return run_tests(argv[2]);
    }

    std::string source = read_file(cmd);
    if (source.empty()) return 1;

    std::cout << transpile(source, false);
    return 0;
}
