/**
 * @file codegen.cpp
 * @brief C++ code generation for the Nog language.
 *
 * Transforms a type-checked AST into C++ source code. Uses the runtime
 * helper functions (nog::runtime) to generate idiomatic C++ output.
 */

#include "codegen.hpp"
#include "../runtime/runtime.hpp"

using namespace std;
namespace rt = nog::runtime;

/**
 * Emits C++ code for an expression AST node. Handles all expression types:
 * literals, variable refs, binary expressions, function calls, method calls,
 * field access, struct literals, etc.
 */
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
    if (dynamic_cast<const NoneLiteral*>(&node)) {
        return rt::none_literal();
    }
    if (auto* ref = dynamic_cast<const VariableRef*>(&node)) {
        return rt::variable_ref(ref->name);
    }
    if (auto* expr = dynamic_cast<const BinaryExpr*>(&node)) {
        return rt::binary_expr(emit(*expr->left), expr->op, emit(*expr->right));
    }
    if (auto* expr = dynamic_cast<const IsNone*>(&node)) {
        return rt::is_none(emit(*expr->value));
    }
    if (auto* qref = dynamic_cast<const QualifiedRef*>(&node)) {
        // Qualified reference: module.name -> module::name
        return qref->module_name + "::" + qref->name;
    }

    if (auto* call = dynamic_cast<const FunctionCall*>(&node)) {
        vector<string> args;
        for (const auto& arg : call->args) {
            args.push_back(emit(*arg));
        }

        // Handle qualified function call: module.func -> module::func
        string func_name = call->name;
        size_t dot_pos = func_name.find('.');

        if (dot_pos != string::npos) {
            func_name = func_name.substr(0, dot_pos) + "::" + func_name.substr(dot_pos + 1);
        }

        return rt::function_call(func_name, args);
    }
    if (auto* decl = dynamic_cast<const VariableDecl*>(&node)) {
        return rt::variable_decl(decl->type, decl->name, emit(*decl->value), decl->is_optional);
    }
    if (auto* assign = dynamic_cast<const Assignment*>(&node)) {
        return rt::assignment(assign->name, emit(*assign->value));
    }
    if (auto* ret = dynamic_cast<const ReturnStmt*>(&node)) {
        return rt::return_stmt(emit(*ret->value));
    }
    if (auto* lit = dynamic_cast<const StructLiteral*>(&node)) {
        vector<pair<string, string>> field_values;
        for (const auto& [name, value] : lit->field_values) {
            field_values.push_back({name, emit(*value)});
        }

        // Handle qualified struct name: module.Type -> module::Type
        string struct_name = lit->struct_name;
        size_t dot_pos = struct_name.find('.');

        if (dot_pos != string::npos) {
            struct_name = struct_name.substr(0, dot_pos) + "::" + struct_name.substr(dot_pos + 1);
        }

        return rt::struct_literal(struct_name, field_values);
    }
    if (auto* access = dynamic_cast<const FieldAccess*>(&node)) {
        // Handle self.field -> this->field in methods
        if (auto* ref = dynamic_cast<const VariableRef*>(access->object.get())) {
            if (ref->name == "self") {
                return "this->" + access->field_name;
            }
        }

        return rt::field_access(emit(*access->object), access->field_name);
    }

    if (auto* call = dynamic_cast<const MethodCall*>(&node)) {
        vector<string> args;

        for (const auto& arg : call->args) {
            args.push_back(emit(*arg));
        }

        return rt::method_call(emit(*call->object), call->method_name, args);
    }

    if (auto* fa = dynamic_cast<const FieldAssignment*>(&node)) {
        // Handle self.field = value -> this->field = value in methods
        if (auto* ref = dynamic_cast<const VariableRef*>(fa->object.get())) {
            if (ref->name == "self") {
                return "this->" + fa->field_name + " = " + emit(*fa->value);
            }
        }

        return rt::field_assignment(emit(*fa->object), fa->field_name, emit(*fa->value));
    }

    return "";
}

/**
 * Main code generation entry point. Generates complete C++ source with headers,
 * structs, and functions. In test mode, generates a test harness instead.
 */
string CodeGen::generate(const unique_ptr<Program>& program, bool test_mode) {
    this->test_mode = test_mode;
    this->current_program = program.get();
    this->imported_modules.clear();

    if (test_mode) {
        return generate_test_harness(program);
    }

    string out = "#include <iostream>\n#include <string>\n#include <optional>\n\n";

    for (const auto& s : program->structs) {
        out += generate_struct(*s) + "\n\n";
    }

    for (const auto& fn : program->functions) {
        out += generate_function(*fn);
    }

    return out;
}

/**
 * Generates C++ code for a program with imported modules.
 * Imported modules are generated as C++ namespaces.
 */
string CodeGen::generate_with_imports(
    const unique_ptr<Program>& program,
    const map<string, const Module*>& imports,
    bool test_mode
) {
    this->test_mode = test_mode;
    this->current_program = program.get();
    this->imported_modules = imports;

    string out = "#include <iostream>\n#include <string>\n#include <cstdint>\n#include <optional>\n\n";

    // Generate test harness infrastructure if in test mode
    if (test_mode) {
        out += "int _failures = 0;\n\n";
        out += "template<typename T, typename U>\n";
        out += "void _assert_eq(T a, U b, int line) {\n";
        out += "\tif (a != b) {\n";
        out += "\t\tstd::cerr << \"line \" << line << \": FAIL: \" << a << \" != \" << b << std::endl;\n";
        out += "\t\t_failures++;\n";
        out += "\t}\n";
        out += "}\n\n";
    }

    // Generate namespaces for imported modules
    for (const auto& [alias, mod] : imports) {
        out += generate_module_namespace(alias, *mod);
    }

    // Generate structs from the main program
    for (const auto& s : program->structs) {
        out += generate_struct(*s) + "\n\n";
    }

    // Generate functions from the main program
    for (const auto& fn : program->functions) {
        out += generate_function(*fn);
    }

    // Generate test harness main if in test mode
    if (test_mode) {
        vector<string> test_names;

        for (const auto& fn : program->functions) {
            if (fn->name.rfind("test_", 0) == 0) {
                test_names.push_back(fn->name);
            }
        }

        out += "\nint main() {\n";

        for (const auto& name : test_names) {
            out += "\t" + name + "();\n";
        }

        out += "\treturn _failures;\n";
        out += "}\n";
    }

    return out;
}

/**
 * Generates a C++ namespace for an imported module.
 * Only includes public structs and functions.
 */
string CodeGen::generate_module_namespace(const string& name, const Module& module) {
    string out = "namespace " + name + " {\n\n";

    // Save and set current program for method lookup
    const Program* saved_program = current_program;
    current_program = module.ast.get();

    // Generate public structs
    for (const auto* s : module.get_public_structs()) {
        out += generate_struct(*s) + "\n\n";
    }

    // Generate public functions
    for (const auto* f : module.get_public_functions()) {
        out += generate_function(*f);
    }

    // Restore current program
    current_program = saved_program;

    out += "} // namespace " + name + "\n\n";
    return out;
}

/**
 * Generates a C++ struct definition with fields and any associated methods.
 * Methods become member functions within the struct.
 */
string CodeGen::generate_struct(const StructDef& def) {
    vector<pair<string, string>> fields;

    for (const auto& f : def.fields) {
        fields.push_back({f.name, f.type});
    }

    // Collect methods for this struct
    vector<string> method_bodies;

    if (current_program) {
        for (const auto& m : current_program->methods) {
            if (m->struct_name == def.name) {
                method_bodies.push_back(generate_method(*m));
            }
        }
    }

    if (method_bodies.empty()) {
        return rt::struct_def(def.name, fields);
    }

    return rt::struct_def_with_methods(def.name, fields, method_bodies);
}

/**
 * Generates a C++ member function from a method definition.
 * Skips the 'self' parameter (becomes implicit 'this').
 */
string CodeGen::generate_method(const MethodDef& method) {
    // Params: skip 'self' since it becomes implicit 'this'
    vector<pair<string, string>> params;

    for (size_t i = 1; i < method.params.size(); i++) {
        params.push_back({method.params[i].type, method.params[i].name});
    }

    vector<string> body;

    for (const auto& stmt : method.body) {
        body.push_back(generate_statement(*stmt));
    }

    return rt::method_def(method.name, params, method.return_type, body);
}

/**
 * Generates a C++ function. Handles 'main' specially (adds return 0 if no return type).
 * Maps Nog types to C++ types for parameters and return type.
 */
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

/**
 * Generates C++ code for a statement. Handles print(), assert_eq() (in test mode),
 * if/while statements, method calls, field assignments, and other statements.
 */
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

        // Handle qualified function call: module.func -> module::func
        string func_name = call->name;
        size_t dot_pos = func_name.find('.');

        if (dot_pos != string::npos) {
            func_name = func_name.substr(0, dot_pos) + "::" + func_name.substr(dot_pos + 1);
        }

        return rt::function_call(func_name, args) + ";";
    }
    if (auto* stmt = dynamic_cast<const IfStmt*>(&node)) {
        vector<string> then_body;
        for (const auto& s : stmt->then_body) {
            then_body.push_back(generate_statement(*s));
        }
        vector<string> else_body;
        for (const auto& s : stmt->else_body) {
            else_body.push_back(generate_statement(*s));
        }
        return rt::if_stmt(emit(*stmt->condition), then_body, else_body);
    }
    if (auto* stmt = dynamic_cast<const WhileStmt*>(&node)) {
        vector<string> body;

        for (const auto& s : stmt->body) {
            body.push_back(generate_statement(*s));
        }

        return rt::while_stmt(emit(*stmt->condition), body);
    }

    if (auto* call = dynamic_cast<const MethodCall*>(&node)) {
        return emit(node) + ";";
    }

    if (auto* fa = dynamic_cast<const FieldAssignment*>(&node)) {
        return emit(node) + ";";
    }

    return emit(node);
}

/**
 * Generates a test harness main() that runs all test_* functions and tracks
 * failures. Includes the _assert_eq template for test assertions.
 */
string CodeGen::generate_test_harness(const unique_ptr<Program>& program) {
    this->current_program = program.get();
    string out = "#include <iostream>\n#include <string>\n#include <cstdint>\n#include <optional>\n\n";

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
