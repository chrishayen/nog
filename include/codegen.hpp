#pragma once
#include <string>
#include <memory>
#include "ast.hpp"

using namespace std;

class CodeGen {
public:
    string generate(const unique_ptr<Program>& program, bool test_mode = false);

private:
    bool test_mode = false;
    string emit(const ASTNode& node);
    string generate_function(const FunctionDef& fn);
    string generate_struct(const StructDef& def);
    string generate_statement(const ASTNode& node);
    string generate_test_harness(const unique_ptr<Program>& program);
};
