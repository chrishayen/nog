#include "codegen.hpp"
#include "../runtime/runtime.hpp"

using namespace std;
namespace rt = nog::runtime;

string CodeGen::emit(const ASTNode& node) {
    if (auto* lit = dynamic_cast<const StringLiteral*>(&node)) {
        return rt::string_literal(lit->value);
    }
    if (auto* lit = dynamic_cast<const NumberLiteral*>(&node)) {
        return rt::number_literal(lit->value);
    }
    if (auto* lit = dynamic_cast<const FloatLiteral*>(&node)) {
        return rt::float_literal(lit->value);
    }
    if (auto* lit = dynamic_cast<const BoolLiteral*>(&node)) {
        return rt::bool_literal(lit->value);
    }
    if (auto* ref = dynamic_cast<const VariableRef*>(&node)) {
        return rt::variable_ref(ref->name);
    }
    if (auto* expr = dynamic_cast<const BinaryExpr*>(&node)) {
        return rt::binary_expr(emit(*expr->left), expr->op, emit(*expr->right));
    }
    if (auto* call = dynamic_cast<const FunctionCall*>(&node)) {
        vector<string> args;
        for (const auto& arg : call->args) {
            args.push_back(emit(*arg));
        }
        return rt::function_call(call->name, args);
    }
    if (auto* decl = dynamic_cast<const VariableDecl*>(&node)) {
        return rt::variable_decl(decl->type, decl->name, emit(*decl->value));
    }
    if (auto* ret = dynamic_cast<const ReturnStmt*>(&node)) {
        return rt::return_stmt(emit(*ret->value));
    }
    if (auto* lit = dynamic_cast<const StructLiteral*>(&node)) {
        vector<pair<string, string>> field_values;
        for (const auto& [name, value] : lit->field_values) {
            field_values.push_back({name, emit(*value)});
        }
        return rt::struct_literal(lit->struct_name, field_values);
    }
    if (auto* access = dynamic_cast<const FieldAccess*>(&node)) {
        return rt::field_access(emit(*access->object), access->field_name);
    }
    return "";
}

string CodeGen::generate(const unique_ptr<Program>& program, bool test_mode) {
    this->test_mode = test_mode;

    if (test_mode) {
        return generate_test_harness(program);
    }

    string out = "#include <iostream>\n#include <string>\n\n";

    for (const auto& s : program->structs) {
        out += generate_struct(*s) + "\n\n";
    }

    for (const auto& fn : program->functions) {
        out += generate_function(*fn);
    }
    return out;
}

string CodeGen::generate_struct(const StructDef& def) {
    vector<pair<string, string>> fields;
    for (const auto& f : def.fields) {
        fields.push_back({f.name, f.type});
    }
    return rt::struct_def(def.name, fields);
}

string CodeGen::generate_function(const FunctionDef& fn) {
    bool is_main = (fn.name == "main" && !test_mode);

    vector<rt::FunctionParam> params;
    for (const auto& p : fn.params) {
        params.push_back({p.type, p.name});
    }

    vector<string> body;
    for (const auto& stmt : fn.body) {
        body.push_back(generate_statement(*stmt));
    }

    string rt_type = is_main ? "int" : fn.return_type;

    string out = rt::function_def(fn.name, params, rt_type, body);

    // For main without explicit return, add return 0
    if (is_main && fn.return_type.empty()) {
        // Insert return 0 before closing brace
        auto pos = out.rfind("}\n");
        if (pos != string::npos) {
            out.insert(pos, "\treturn 0;\n");
        }
    }

    return out;
}

string CodeGen::generate_statement(const ASTNode& node) {
    if (auto* call = dynamic_cast<const FunctionCall*>(&node)) {
        if (call->name == "print") {
            vector<string> args;
            for (const auto& arg : call->args) {
                args.push_back(emit(*arg));
            }
            return rt::print_multi(args) + ";";
        }
        if (call->name == "assert_eq" && test_mode && call->args.size() >= 2) {
            return rt::assert_eq(emit(*call->args[0]), emit(*call->args[1]), call->line) + ";";
        }
        vector<string> args;
        for (const auto& arg : call->args) {
            args.push_back(emit(*arg));
        }
        return rt::function_call(call->name, args) + ";";
    }
    return emit(node);
}

string CodeGen::generate_test_harness(const unique_ptr<Program>& program) {
    string out = "#include <iostream>\n#include <string>\n#include <cstdint>\n\n";

    out += "int _failures = 0;\n\n";
    out += "template<typename T, typename U>\n";
    out += "void _assert_eq(T a, U b, int line) {\n";
    out += "\tif (a != b) {\n";
    out += "\t\tstd::cerr << \"line \" << line << \": FAIL: \" << a << \" != \" << b << std::endl;\n";
    out += "\t\t_failures++;\n";
    out += "\t}\n";
    out += "}\n\n";

    for (const auto& s : program->structs) {
        out += generate_struct(*s) + "\n\n";
    }

    vector<string> test_names;
    for (const auto& fn : program->functions) {
        if (fn->name.rfind("test_", 0) == 0) {
            test_names.push_back(fn->name);
        }
        out += generate_function(*fn);
    }

    out += "\nint main() {\n";
    for (const auto& name : test_names) {
        out += "\t" + name + "();\n";
    }
    out += "\t// exit code signals pass/fail\n";
    out += "\treturn _failures;\n";
    out += "}\n";

    return out;
}
