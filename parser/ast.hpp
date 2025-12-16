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
// Visibility - Controls access from other modules
//------------------------------------------------------------------------------

/** @brief Visibility modifier for declarations */
enum class Visibility {
    Public,   ///< Default - accessible from other modules
    Private   ///< @private - only accessible within same module
};

//------------------------------------------------------------------------------
// Imports - Module dependencies
//------------------------------------------------------------------------------

/** @brief Import statement: import math; or import utils.helpers; */
struct ImportStmt : ASTNode {
    string module_path;  ///< Full module path (e.g., "math" or "utils.helpers")
    string alias;        ///< Local name (last component: "math" or "helpers")

    explicit ImportStmt(const string& path) : module_path(path) {
        // Extract alias from last dot-separated component
        size_t last_dot = path.rfind('.');

        if (last_dot != string::npos) {
            alias = path.substr(last_dot + 1);
        } else {
            alias = path;
        }
    }
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

/** @brief Reference to a function by name (for passing as argument) */
struct FunctionRef : ASTNode {
    string name;         ///< Function name (may be qualified: module.func)
    explicit FunctionRef(const string& n) : name(n) {}
};

/** @brief Qualified reference to an imported item: module.name (e.g., math.add) */
struct QualifiedRef : ASTNode {
    string module_name;  ///< Module alias (e.g., "math")
    string name;         ///< Item name within module (e.g., "add")

    QualifiedRef(const string& mod, const string& n) : module_name(mod), name(n) {}
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

/** @brief Logical NOT expression: !expr */
struct NotExpr : ASTNode {
    unique_ptr<ASTNode> value;  ///< The expression to negate
};

/** @brief Await expression: await expr */
struct AwaitExpr : ASTNode {
    unique_ptr<ASTNode> value;  ///< The awaitable expression
};

//------------------------------------------------------------------------------
// Channels - Concurrent communication primitives
//------------------------------------------------------------------------------

/** @brief Channel creation: Channel<int>() */
struct ChannelCreate : ASTNode {
    string element_type;  ///< Type of elements the channel carries
};

/** @brief List creation: List<int>() */
struct ListCreate : ASTNode {
    string element_type;  ///< Type of elements the list holds
};

/** @brief List literal: [1, 2, 3] */
struct ListLiteral : ASTNode {
    vector<unique_ptr<ASTNode>> elements;  ///< List element expressions
};

/** @brief A single case in a select statement */
struct SelectCase : ASTNode {
    string binding_name;              ///< Variable to bind result (empty for send)
    unique_ptr<ASTNode> channel;      ///< Channel expression (e.g., ch1)
    string operation;                 ///< "recv" or "send"
    unique_ptr<ASTNode> send_value;   ///< Value to send (null for recv)
    vector<unique_ptr<ASTNode>> body; ///< Case body statements
};

/** @brief Select statement for multiplexing channel operations */
struct SelectStmt : ASTNode {
    vector<unique_ptr<SelectCase>> cases;  ///< All case branches
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
    mutable string object_type;         ///< Inferred type of object (set by type checker)
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

/** @brief Distinguishes for loop types */
enum class ForLoopKind {
    Range,    ///< for i in 0..10
    Foreach   ///< for item in collection
};

/** @brief For loop: for var in range/collection { } */
struct ForStmt : ASTNode {
    string loop_var;                     ///< Loop variable name
    ForLoopKind kind;                    ///< Range or Foreach
    unique_ptr<ASTNode> range_start;     ///< Start value (Range only)
    unique_ptr<ASTNode> range_end;       ///< End value (Range only)
    unique_ptr<ASTNode> iterable;        ///< Collection to iterate (Foreach only)
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

/** @brief Function definition: fn name(params) -> ret_type { body } or async fn ... */
struct FunctionDef : ASTNode {
    string name;                          ///< Function name
    vector<FunctionParam> params;         ///< Parameter list
    string return_type;                   ///< Return type (empty for void)
    vector<unique_ptr<ASTNode>> body;     ///< Function body statements
    Visibility visibility = Visibility::Public;  ///< Access modifier
    string doc_comment;                   ///< Documentation comment (from ///)
    bool is_async = false;                ///< True if declared with async keyword
};

/** @brief External function declaration: @extern("lib") fn name(params) -> ret_type; */
struct ExternFunctionDef : ASTNode {
    string name;                          ///< Function name
    vector<FunctionParam> params;         ///< Parameter list with C-compatible types
    string return_type;                   ///< Return type (cint, cstr, void, or empty)
    string library;                       ///< Library to link against ("c", "m", etc.)
    Visibility visibility = Visibility::Public;  ///< Access modifier
    string doc_comment;                   ///< Documentation comment (from ///)
};

/** @brief Method definition: StructName :: method_name(self, params) -> ret_type { body } or async ... */
struct MethodDef : ASTNode {
    string struct_name;                   ///< Struct this method belongs to
    string name;                          ///< Method name
    vector<FunctionParam> params;         ///< Includes self as first param
    string return_type;                   ///< Return type (empty for void)
    vector<unique_ptr<ASTNode>> body;     ///< Method body statements
    Visibility visibility = Visibility::Public;  ///< Access modifier
    string doc_comment;                   ///< Documentation comment (from ///)
    bool is_async = false;                ///< True if declared with async keyword
};

//------------------------------------------------------------------------------
// Structs - User-defined types
//------------------------------------------------------------------------------

/** @brief A field in a struct definition */
struct StructField {
    string name;        ///< Field name
    string type;        ///< Field type
    string doc_comment; ///< Documentation comment (from ///)
};

/** @brief Struct definition: Name :: struct { fields } */
struct StructDef : ASTNode {
    string name;                  ///< Struct name
    vector<StructField> fields;   ///< List of fields
    Visibility visibility = Visibility::Public;  ///< Access modifier
    string doc_comment;           ///< Documentation comment (from ///)
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
    vector<unique_ptr<ImportStmt>> imports;           ///< All import statements
    vector<unique_ptr<StructDef>> structs;            ///< All struct definitions
    vector<unique_ptr<FunctionDef>> functions;        ///< All function definitions
    vector<unique_ptr<MethodDef>> methods;            ///< All method definitions
    vector<unique_ptr<ExternFunctionDef>> externs;    ///< All extern function declarations
};
