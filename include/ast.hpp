/**
 * @file ast.hpp
 * @brief Abstract Syntax Tree node definitions for Nog.
 *
 * Defines all AST node types used to represent parsed Nog programs. The hierarchy:
 * - ASTNode: Base class with line number tracking
 * - Literals: StringLiteral, NumberLiteral, FloatLiteral, BoolLiteral, NoneLiteral
 * - Expressions: VariableRef, BinaryExpr, IsNone, FunctionCall, MethodCall, FieldAccess, StructLiteral
 * - Statements: VariableDecl, Assignment, FieldAssignment, ReturnStmt, IfStmt, WhileStmt
 * - Definitions: FunctionDef, MethodDef, StructDef
 * - Program: Top-level container for all definitions
 */

#pragma once
#include <string>
#include <vector>
#include <memory>
#include <utility>

using namespace std;

/**
 * @brief Base class for all AST nodes.
 * Provides line number tracking for error reporting.
 */
struct ASTNode {
    int line = 0;  ///< Source line number (0 if not set)
    virtual ~ASTNode() = default;
};

//------------------------------------------------------------------------------
// Literals - Constant values in source code
//------------------------------------------------------------------------------

/** @brief String literal: "hello" */
struct StringLiteral : ASTNode {
    string value;
    explicit StringLiteral(const string& v) : value(v) {}
};

/** @brief Integer literal: 42 */
struct NumberLiteral : ASTNode {
    string value;  ///< String representation of the number
    explicit NumberLiteral(const string& v) : value(v) {}
};

/** @brief Floating-point literal: 3.14 */
struct FloatLiteral : ASTNode {
    string value;  ///< String representation of the float
    explicit FloatLiteral(const string& v) : value(v) {}
};

/** @brief Boolean literal: true or false */
struct BoolLiteral : ASTNode {
    bool value;
    explicit BoolLiteral(bool v) : value(v) {}
};

/** @brief The none literal for optional types */
struct NoneLiteral : ASTNode {};

/** @brief Reference to a variable by name */
struct VariableRef : ASTNode {
    string name;
    explicit VariableRef(const string& n) : name(n) {}
};

//------------------------------------------------------------------------------
// Expressions - Nodes that produce values
//------------------------------------------------------------------------------

/** @brief Binary operation: left op right (e.g., a + b, x == y) */
struct BinaryExpr : ASTNode {
    string op;
    unique_ptr<ASTNode> left;
    unique_ptr<ASTNode> right;
};

/** @brief Check if optional value is none: x is none */
struct IsNone : ASTNode {
    unique_ptr<ASTNode> value;  ///< The optional value to check
};

/** @brief Function call: func(arg1, arg2) */
struct FunctionCall : ASTNode {
    string name;                        ///< Function name
    vector<unique_ptr<ASTNode>> args;   ///< Arguments to pass
};

/** @brief Method call on an object: obj.method(args) */
struct MethodCall : ASTNode {
    unique_ptr<ASTNode> object;         ///< The object to call method on
    string method_name;                 ///< Method name
    vector<unique_ptr<ASTNode>> args;   ///< Arguments (excluding self)
};

//------------------------------------------------------------------------------
// Statements - Nodes that perform actions
//------------------------------------------------------------------------------

/** @brief Variable declaration: int x = 5 or x := 5 or int? x = none */
struct VariableDecl : ASTNode {
    string type;                   ///< Type name (empty for type inference)
    string name;                   ///< Variable name
    unique_ptr<ASTNode> value;     ///< Initial value expression
    bool is_optional = false;      ///< True if declared with ? (e.g., int?)
};

/** @brief Assignment to existing variable: x = value */
struct Assignment : ASTNode {
    string name;                   ///< Variable name
    unique_ptr<ASTNode> value;     ///< New value expression
};

/** @brief Field assignment: obj.field = value */
struct FieldAssignment : ASTNode {
    unique_ptr<ASTNode> object;    ///< Object containing the field
    string field_name;             ///< Field to assign
    unique_ptr<ASTNode> value;     ///< New value expression
};

/** @brief Return statement: return expr */
struct ReturnStmt : ASTNode {
    unique_ptr<ASTNode> value;     ///< Return value (may be null for void)
};

/** @brief If statement with optional else: if cond { } else { } */
struct IfStmt : ASTNode {
    unique_ptr<ASTNode> condition;           ///< Condition expression
    vector<unique_ptr<ASTNode>> then_body;   ///< Statements if true
    vector<unique_ptr<ASTNode>> else_body;   ///< Statements if false (may be empty)
};

/** @brief While loop: while cond { } */
struct WhileStmt : ASTNode {
    unique_ptr<ASTNode> condition;       ///< Loop condition
    vector<unique_ptr<ASTNode>> body;    ///< Loop body statements
};

//------------------------------------------------------------------------------
// Definitions - Top-level declarations
//------------------------------------------------------------------------------

/** @brief A parameter in a function or method signature */
struct FunctionParam {
    string type;   ///< Parameter type
    string name;   ///< Parameter name
};

/** @brief Function definition: fn name(params) -> ret_type { body } */
struct FunctionDef : ASTNode {
    string name;                          ///< Function name
    vector<FunctionParam> params;         ///< Parameter list
    string return_type;                   ///< Return type (empty for void)
    vector<unique_ptr<ASTNode>> body;     ///< Function body statements
};

/** @brief Method definition: StructName :: method_name(self, params) -> ret_type { body } */
struct MethodDef : ASTNode {
    string struct_name;                   ///< Struct this method belongs to
    string name;                          ///< Method name
    vector<FunctionParam> params;         ///< Includes self as first param
    string return_type;                   ///< Return type (empty for void)
    vector<unique_ptr<ASTNode>> body;     ///< Method body statements
};

//------------------------------------------------------------------------------
// Structs - User-defined types
//------------------------------------------------------------------------------

/** @brief A field in a struct definition */
struct StructField {
    string name;   ///< Field name
    string type;   ///< Field type
};

/** @brief Struct definition: Name :: struct { fields } */
struct StructDef : ASTNode {
    string name;                  ///< Struct name
    vector<StructField> fields;   ///< List of fields
};

/** @brief Struct literal: TypeName { field: value, ... } */
struct StructLiteral : ASTNode {
    string struct_name;   ///< Name of struct type
    vector<pair<string, unique_ptr<ASTNode>>> field_values;  ///< Field initializers
};

/** @brief Field access expression: obj.field */
struct FieldAccess : ASTNode {
    unique_ptr<ASTNode> object;   ///< Object to access field on
    string field_name;            ///< Field name
};

//------------------------------------------------------------------------------
// Program - Root of the AST
//------------------------------------------------------------------------------

/** @brief The complete program: all structs, functions, and methods */
struct Program : ASTNode {
    vector<unique_ptr<StructDef>> structs;      ///< All struct definitions
    vector<unique_ptr<FunctionDef>> functions;  ///< All function definitions
    vector<unique_ptr<MethodDef>> methods;      ///< All method definitions
};
