#include "codegen.hpp"
#include "../runtime/builtins.hpp"

std::string CodeGen::generate(const std::unique_ptr<Program>& program, bool test_mode) {
    this->test_mode = test_mode;

    if (test_mode) {
        return generate_test_harness(program);
    }

    std::string out = "#include <iostream>\n#include <string>\n\n";
    for (const auto& fn : program->functions) {
        out += generate_function(*fn);
    }
    return out;
}

std::string CodeGen::generate_function(const FunctionDef& fn) {
    bool is_main = (fn.name == "main" && !test_mode);

    std::string rt = is_main ? "int" : (fn.return_type.empty() ? "void" : map_type(fn.return_type));
    std::string out = rt + " " + fn.name + "(";

    for (size_t i = 0; i < fn.params.size(); i++) {
        if (i > 0) out += ", ";
        out += map_type(fn.params[i].type) + " " + fn.params[i].name;
    }
    out += ") {\n";

    for (const auto& stmt : fn.body) {
        out += "    " + generate_statement(*stmt) + "\n";
    }

    if (is_main && fn.return_type.empty()) {
        out += "    return 0;\n";
    }

    out += "}\n";
    return out;
}

std::string CodeGen::generate_statement(const ASTNode& node) {
    if (auto* call = dynamic_cast<const FunctionCall*>(&node)) {
        if (call->name == "print") {
            std::vector<std::string> args;
            for (const auto& arg : call->args) {
                args.push_back(arg->cpp());
            }
            return nog::runtime::print_multi(args) + ";";
        }
        if (call->name == "assert_eq" && test_mode && call->args.size() >= 2) {
            return nog::runtime::assert_eq(call->args[0]->cpp(), call->args[1]->cpp(), call->line) + ";";
        }
        return call->cpp() + ";";
    }
    return node.cpp();
}

std::string CodeGen::generate_test_harness(const std::unique_ptr<Program>& program) {
    std::string out = "#include <iostream>\n#include <string>\n#include <cstdint>\n\n";

    out += "int _failures = 0;\n\n";
    out += "template<typename T>\n";
    out += "void _assert_eq(T a, T b, int line) {\n";
    out += "    if (a != b) {\n";
    out += "        std::cerr << \"line \" << line << \": FAIL: \" << a << \" != \" << b << std::endl;\n";
    out += "        _failures++;\n";
    out += "    }\n";
    out += "}\n\n";

    std::vector<std::string> test_names;
    for (const auto& fn : program->functions) {
        if (fn->name.rfind("test_", 0) == 0) {
            test_names.push_back(fn->name);
        }
        out += generate_function(*fn);
    }

    out += "\nint main() {\n";
    for (const auto& name : test_names) {
        out += "    " + name + "();\n";
    }
    out += "    // exit code signals pass/fail\n";
    out += "    return _failures;\n";
    out += "}\n";

    return out;
}
