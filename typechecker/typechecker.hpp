/**
 * @file typechecker.hpp
 * @brief Static type checker for the Nog language.
 *
 * Performs semantic analysis on the AST before code generation using
 * standalone functions in the typechecker namespace with explicit state passing.
 */

#pragma once
#include <string>
#include <vector>
#include <map>
#include "parser/ast.hpp"
#include "project/module.hpp"

/**
 * @brief A type error found during checking.
 * Contains the error message and source location for reporting.
 */
struct TypeError {
    std::string message;
    int line;
    std::string filename;
};

/**
 * @brief Represents the type of an expression or variable.
 */
struct TypeInfo {
    std::string base_type;
    bool is_optional = false;
    bool is_void = false;
    // Marks expressions that produce a value that must be awaited to get the
    // underlying type (e.g., async fn calls, Channel.send/recv).
    bool is_awaitable = false;

    bool operator==(const TypeInfo& other) const {
        return base_type == other.base_type && is_optional == other.is_optional &&
               is_void == other.is_void && is_awaitable == other.is_awaitable;
    }

    bool operator!=(const TypeInfo& other) const {
        return !(*this == other);
    }
};

/**
 * @brief Type checker state passed to all checking functions.
 */
struct TypeCheckerState {
    // Symbol tables
    std::map<std::string, const StructDef*> structs;
    std::map<std::string, std::vector<const MethodDef*>> methods;
    std::map<std::string, const FunctionDef*> functions;
    std::map<std::string, const ExternFunctionDef*> extern_functions;
    /**
     * Local variables are tracked with a lexical scope stack.
     *
     * - Each function/method body starts with a single scope containing parameters.
     * - Control-flow constructs with `{ ... }` (if/else/while/for/select cases) push a scope
     *   for the duration of their block, then pop it when leaving the block.
     * - Lookups search from innermost to outermost scope.
     *
     * This prevents accepting programs that would fail to compile after codegen
     * (e.g., using a variable declared inside an `if` block from outside that block).
     */
    std::vector<std::map<std::string, TypeInfo>> local_scopes;
    std::map<std::string, const Module*> imported_modules;

    // Current context
    std::string current_struct;
    TypeInfo current_return;
    std::string filename;
    bool in_async_context = false;

    std::vector<TypeError> errors;
};

namespace typechecker {

// Main entry point (typechecker.cpp)
bool check(TypeCheckerState& state, const Program& program, const std::string& filename);
void register_module(TypeCheckerState& state, const std::string& alias, const Module& module);

// -----------------------------------------------------------------------------
// Local scope helpers
// -----------------------------------------------------------------------------
//
// The typechecker models Nog's `{ ... }` blocks as lexical scopes, matching the
// generated C++ code. These helpers centralize scope operations so individual
// check_* functions don't need to manually manage maps.
void push_scope(TypeCheckerState& state);
void pop_scope(TypeCheckerState& state);
bool is_declared_in_current_scope(const TypeCheckerState& state, const std::string& name);
void declare_local(TypeCheckerState& state, const std::string& name, const TypeInfo& type, int line);
TypeInfo* lookup_local(TypeCheckerState& state, const std::string& name);
const TypeInfo* lookup_local(const TypeCheckerState& state, const std::string& name);

// Collection functions (typechecker.cpp)
void collect_structs(TypeCheckerState& state, const Program& program);
void collect_methods(TypeCheckerState& state, const Program& program);
void collect_functions(TypeCheckerState& state, const Program& program);
void collect_extern_functions(TypeCheckerState& state, const Program& program);

// Function/method checking (check_function.cpp)
void check_method(TypeCheckerState& state, const MethodDef& method);
void check_function(TypeCheckerState& state, const FunctionDef& func);
bool has_return(const std::vector<std::unique_ptr<ASTNode>>& stmts);

// Statement checking (check_statement.cpp)
void check_statement(TypeCheckerState& state, const ASTNode& stmt);

// Statement checking helpers
void check_variable_decl_stmt(TypeCheckerState& state, const VariableDecl& decl);
void check_assignment_stmt(TypeCheckerState& state, const Assignment& assign);
void check_field_assignment_stmt(TypeCheckerState& state, const FieldAssignment& fa);
void check_return_stmt(TypeCheckerState& state, const ReturnStmt& ret);
void check_if_stmt(TypeCheckerState& state, const IfStmt& if_stmt);
void check_while_stmt(TypeCheckerState& state, const WhileStmt& while_stmt);
void check_for_stmt(TypeCheckerState& state, const ForStmt& for_stmt);
void check_select_stmt(TypeCheckerState& state, const SelectStmt& select_stmt);

// Expression type inference (check_expression.cpp)
TypeInfo infer_type(TypeCheckerState& state, const ASTNode& expr);

// Literal type inference (check_literals.cpp)
TypeInfo check_number_literal(TypeCheckerState& state, const NumberLiteral& lit);
TypeInfo check_float_literal(TypeCheckerState& state, const FloatLiteral& lit);
TypeInfo check_string_literal(TypeCheckerState& state, const StringLiteral& lit);
TypeInfo check_bool_literal(TypeCheckerState& state, const BoolLiteral& lit);
TypeInfo check_none_literal(TypeCheckerState& state, const NoneLiteral& lit);
TypeInfo check_char_literal(TypeCheckerState& state, const CharLiteral& lit);

// Reference type inference (check_refs.cpp)
TypeInfo check_variable_ref(TypeCheckerState& state, const VariableRef& var);
TypeInfo check_function_ref(TypeCheckerState& state, const FunctionRef& fref);
TypeInfo check_qualified_ref(TypeCheckerState& state, const QualifiedRef& qref);

// Binary expression type inference (check_binary.cpp)
TypeInfo check_binary_expr(TypeCheckerState& state, const BinaryExpr& bin);
TypeInfo check_is_none(TypeCheckerState& state, const IsNone& expr);
TypeInfo check_not_expr(TypeCheckerState& state, const NotExpr& not_expr);
TypeInfo check_await_expr(TypeCheckerState& state, const AwaitExpr& await_expr);

// Channel type inference (check_channel.cpp)
TypeInfo check_channel_create(TypeCheckerState& state, const ChannelCreate& channel);
TypeInfo check_channel_method(TypeCheckerState& state, const MethodCall& mcall, const std::string& element_type);

// List type inference (check_list.cpp)
TypeInfo check_list_create(TypeCheckerState& state, const ListCreate& list);
TypeInfo check_list_literal(TypeCheckerState& state, const ListLiteral& list);
TypeInfo check_list_method(TypeCheckerState& state, const MethodCall& mcall, const std::string& element_type);

// Function call type inference (check_function_call.cpp)
TypeInfo check_function_call(TypeCheckerState& state, const FunctionCall& call);

// Method call type inference (check_method_call.cpp)
TypeInfo check_str_method(TypeCheckerState& state, const MethodCall& mcall);
TypeInfo check_struct_method(TypeCheckerState& state, const MethodCall& mcall, const TypeInfo& obj_type);
TypeInfo check_method_call(TypeCheckerState& state, const MethodCall& mcall);

// Field access type inference (check_field.cpp)
TypeInfo check_field_access(TypeCheckerState& state, const FieldAccess& access);

// Struct literal type inference (check_struct_literal.cpp)
TypeInfo check_struct_literal(TypeCheckerState& state, const StructLiteral& lit);

// Type utilities (typechecker.cpp)
bool is_primitive_type(const std::string& type);
bool is_valid_type(const TypeCheckerState& state, const std::string& type);
bool types_compatible(const TypeInfo& expected, const TypeInfo& actual);
std::string format_type(const TypeInfo& type);
TypeInfo make_awaitable(const TypeInfo& inner);
const StructDef* get_struct(const TypeCheckerState& state, const std::string& name);
const MethodDef* get_method(const TypeCheckerState& state, const std::string& struct_name, const std::string& method_name);
std::string get_field_type(const TypeCheckerState& state, const std::string& struct_name, const std::string& field_name);
void error(TypeCheckerState& state, const std::string& msg, int line);

// Module-aware lookups (typechecker.cpp)
const FunctionDef* get_qualified_function(const TypeCheckerState& state, const std::string& module, const std::string& name);
const StructDef* get_qualified_struct(const TypeCheckerState& state, const std::string& module, const std::string& name);
const MethodDef* get_qualified_method(const TypeCheckerState& state, const std::string& module, const std::string& struct_name, const std::string& method_name);

} // namespace typechecker

/**
 * @brief Legacy class API for backwards compatibility.
 */
class TypeChecker {
public:
    bool check(const Program& program, const std::string& filename);
    void register_module(const std::string& alias, const Module& module);
    const std::vector<TypeError>& get_errors() const { return state.errors; }

private:
    TypeCheckerState state;
};
