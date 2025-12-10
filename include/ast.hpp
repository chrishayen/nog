#pragma once
#include <string>
#include <vector>
#include <memory>
#include <utility>

using namespace std;

struct ASTNode {
    virtual ~ASTNode() = default;
};

// Literals
struct StringLiteral : ASTNode {
    string value;
    explicit StringLiteral(const string& v) : value(v) {}
};

struct NumberLiteral : ASTNode {
    string value;
    explicit NumberLiteral(const string& v) : value(v) {}
};

struct FloatLiteral : ASTNode {
    string value;
    explicit FloatLiteral(const string& v) : value(v) {}
};

struct BoolLiteral : ASTNode {
    bool value;
    explicit BoolLiteral(bool v) : value(v) {}
};

struct VariableRef : ASTNode {
    string name;
    explicit VariableRef(const string& n) : name(n) {}
};

// Expressions
struct BinaryExpr : ASTNode {
    string op;
    unique_ptr<ASTNode> left;
    unique_ptr<ASTNode> right;
};

struct FunctionCall : ASTNode {
    string name;
    vector<unique_ptr<ASTNode>> args;
    int line = 0;
};

// Statements
struct VariableDecl : ASTNode {
    string type;
    string name;
    unique_ptr<ASTNode> value;
};

struct ReturnStmt : ASTNode {
    unique_ptr<ASTNode> value;
};

// Function
struct FunctionParam {
    string type;
    string name;
};

struct FunctionDef : ASTNode {
    string name;
    vector<FunctionParam> params;
    string return_type;
    vector<unique_ptr<ASTNode>> body;
};

// Struct
struct StructField {
    string name;
    string type;
};

struct StructDef : ASTNode {
    string name;
    vector<StructField> fields;
};

struct StructLiteral : ASTNode {
    string struct_name;
    vector<pair<string, unique_ptr<ASTNode>>> field_values;
};

struct FieldAccess : ASTNode {
    unique_ptr<ASTNode> object;
    string field_name;
};

struct Program : ASTNode {
    vector<unique_ptr<StructDef>> structs;
    vector<unique_ptr<FunctionDef>> functions;
};
