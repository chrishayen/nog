/**
 * @file codegen.hpp
 * @brief C++ code generator for the Nog language.
 *
 * Transforms a type-checked AST into C++ source code using
 * standalone functions in the codegen namespace with explicit state passing.
 */

#pragma once
#include <string>
#include <memory>
#include <map>
#include <vector>
#include "parser/ast.hpp"
#include "project/module.hpp"

/**
 * @brief Code generator state passed to all generation functions.
 *
 * Contains mode flags and context needed during code generation.
 */
struct CodeGenState {
    bool test_mode = false;
    bool in_async_function = false;
    const Program* current_program = nullptr;
    std::map<std::string, const Module*> imported_modules;
    std::map<std::string, const ExternFunctionDef*> extern_functions;
};

namespace codegen {

// Main entry points (codegen.cpp)
std::string generate(CodeGenState& state, const std::unique_ptr<Program>& program, bool test_mode = false);
std::string generate_with_imports(
    CodeGenState& state,
    const std::unique_ptr<Program>& program,
    const std::map<std::string, const Module*>& imports,
    bool test_mode = false
);
std::string generate_module_namespace(CodeGenState& state, const std::string& name, const Module& module);

// FFI emission (emit_ffi.cpp)
std::string generate_extern_declarations(const std::unique_ptr<Program>& program);

// Type utilities (emit_type.cpp)
std::string map_type(const std::string& t);

// Expression emission (emit_expression.cpp)
std::string emit(CodeGenState& state, const ASTNode& node);

// Literals (emit_literals.cpp)
std::string string_literal(const std::string& value);
std::string number_literal(const std::string& value);
std::string float_literal(const std::string& value);
std::string bool_literal(bool value);
std::string none_literal();

// References (emit_refs.cpp)
std::string variable_ref(const std::string& name);
std::string emit_function_ref(const FunctionRef& ref);
std::string emit_qualified_ref(const QualifiedRef& ref);

// Binary expressions (emit_binary.cpp)
std::string binary_expr(const std::string& left, const std::string& op, const std::string& right);
std::string is_none(const std::string& value);
std::string emit_not_expr(CodeGenState& state, const NotExpr& expr);

// Await (emit_await.cpp)
std::string emit_await(CodeGenState& state, const AwaitExpr& expr);

// Channel (emit_channel.cpp)
std::string emit_channel_create(const ChannelCreate& channel);

// List (emit_list.cpp)
std::string emit_list_create(const ListCreate& list);
std::string emit_list_literal(CodeGenState& state, const ListLiteral& list);
std::string emit_list_method_call(CodeGenState& state, const MethodCall& call, const std::string& obj_str, const std::vector<std::string>& args);

// Method call (emit_method_call.cpp)
std::string method_call(const std::string& object, const std::string& method, const std::vector<std::string>& args);
std::string emit_method_call(CodeGenState& state, const MethodCall& call);

// Function call (emit_function_call.cpp)
std::string function_call(const std::string& name, const std::vector<std::string>& args);
std::string emit_function_call(CodeGenState& state, const FunctionCall& call);

// Field access (emit_field.cpp)
std::string emit_field_access(CodeGenState& state, const FieldAccess& access);
std::string emit_field_assignment(CodeGenState& state, const FieldAssignment& fa);

// Statement emission (emit_statement.cpp)
std::string generate_statement(CodeGenState& state, const ASTNode& node);
std::string generate_select(CodeGenState& state, const SelectStmt& stmt);
std::string variable_decl(const std::string& type, const std::string& name, const std::string& value, bool is_optional = false);
std::string return_stmt(const std::string& value);
std::string assignment(const std::string& name, const std::string& value);
std::string if_stmt(const std::string& condition, const std::vector<std::string>& then_body, const std::vector<std::string>& else_body);
std::string while_stmt(const std::string& condition, const std::vector<std::string>& body);
std::string for_range_stmt(const std::string& var, const std::string& start, const std::string& end, const std::vector<std::string>& body);
std::string for_each_stmt(const std::string& var, const std::string& collection, const std::vector<std::string>& body);
std::string print_multi(const std::vector<std::string>& args);
std::string assert_eq(const std::string& a, const std::string& b, int line);

// Function emission (emit_function.cpp)
struct FunctionParam {
    std::string type;
    std::string name;
};
std::string generate_function(CodeGenState& state, const FunctionDef& fn);
std::string generate_method(CodeGenState& state, const MethodDef& method);
std::string generate_test_harness(CodeGenState& state, const std::unique_ptr<Program>& program);
std::string function_def(const std::string& name, const std::vector<FunctionParam>& params, const std::string& return_type, const std::vector<std::string>& body, bool is_async = false);
std::string method_def(const std::string& name, const std::vector<std::pair<std::string, std::string>>& params, const std::string& return_type, const std::vector<std::string>& body_stmts, bool is_async = false);

// Struct emission (emit_struct.cpp)
std::string generate_struct(CodeGenState& state, const StructDef& def);
std::string struct_def(const std::string& name, const std::vector<std::pair<std::string, std::string>>& fields);
std::string struct_def_with_methods(const std::string& name, const std::vector<std::pair<std::string, std::string>>& fields, const std::vector<std::string>& method_bodies);
std::string struct_literal(const std::string& name, const std::vector<std::pair<std::string, std::string>>& field_values);
std::string field_access(const std::string& object, const std::string& field);
std::string field_assignment(const std::string& object, const std::string& field, const std::string& value);

} // namespace codegen

/**
 * @brief Legacy class API for backwards compatibility.
 *
 * Wraps the codegen namespace functions for existing code.
 */
class CodeGen {
public:
    std::string generate(const std::unique_ptr<Program>& program, bool test_mode = false);
    std::string generate_with_imports(
        const std::unique_ptr<Program>& program,
        const std::map<std::string, const Module*>& imports,
        bool test_mode = false
    );
};
