#pragma once
#include <string>
#include <memory>
#include "ast.hpp"

class CodeGen {
public:
    std::string generate(const std::unique_ptr<Program>& program, bool test_mode = false);

private:
    bool test_mode = false;
    std::string generate_function(const FunctionDef& func);
    std::string generate_statement(const ASTNode& node);
    std::string generate_test_harness(const std::unique_ptr<Program>& program);
};
