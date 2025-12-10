#include "codegen.hpp"

std::string CodeGen::generate(const std::unique_ptr<Program>& program, bool test_mode) {
    this->test_mode = test_mode;

    if (test_mode) {
        return generate_test_harness(program);
    }

    std::string output = "#include <iostream>\n\n";
    for (const auto& func : program->functions) {
        output += generate_function(*func);
    }
    return output;
}

std::string CodeGen::generate_test_harness(const std::unique_ptr<Program>& program) {
    std::string output = "#include <iostream>\n#include <string>\n\n";

    output += "int _failures = 0;\n\n";
    output += "void _assert_eq(const std::string& a, const std::string& b, int line) {\n";
    output += "    if (a != b) {\n";
    output += "        std::cerr << \"line \" << line << \": FAIL: \\\"\" << a << \"\\\" != \\\"\" << b << \"\\\"\" << std::endl;\n";
    output += "        _failures++;\n";
    output += "    }\n";
    output += "}\n\n";

    std::vector<std::string> test_names;
    for (const auto& func : program->functions) {
        if (func->name.rfind("test_", 0) == 0) {
            test_names.push_back(func->name);
        }
        output += generate_function(*func);
    }

    output += "\nint main() {\n";
    for (const auto& name : test_names) {
        output += "    " + name + "();\n";
    }
    output += "    std::cout << (_failures == 0 ? \"PASS\" : \"FAIL\") << std::endl;\n";
    output += "    return _failures;\n";
    output += "}\n";

    return output;
}

std::string CodeGen::generate_function(const FunctionDef& func) {
    std::string output;

    if (func.name == "main" && !test_mode) {
        output += "int main() {\n";
    } else {
        output += "void " + func.name + "() {\n";
    }

    for (const auto& stmt : func.body) {
        if (stmt) {
            output += "    " + generate_statement(*stmt) + "\n";
        }
    }

    if (func.name == "main" && !test_mode) {
        output += "    return 0;\n";
    }

    output += "}\n";
    return output;
}

std::string CodeGen::generate_statement(const ASTNode& node) {
    if (auto* call = dynamic_cast<const FunctionCall*>(&node)) {
        if (call->name == "print") {
            std::string output = "std::cout << ";
            for (size_t i = 0; i < call->args.size(); i++) {
                if (auto* str = dynamic_cast<const StringLiteral*>(call->args[i].get())) {
                    output += "\"" + str->value + "\"";
                }
            }
            output += " << std::endl;";
            return output;
        }
        if (call->name == "assert_eq" && test_mode) {
            if (call->args.size() >= 2) {
                auto* a = dynamic_cast<const StringLiteral*>(call->args[0].get());
                auto* b = dynamic_cast<const StringLiteral*>(call->args[1].get());
                if (a && b) {
                    return "_assert_eq(\"" + a->value + "\", \"" + b->value + "\", " + std::to_string(call->line) + ");";
                }
            }
        }
    }
    return "";
}
