#pragma once
#include <string>
#include <vector>
#include <memory>

inline std::string map_type(const std::string& t) {
    if (t == "int") return "int";
    if (t == "str") return "std::string";
    if (t == "bool") return "bool";
    if (t == "char") return "char";
    if (t == "f32") return "float";
    if (t == "f64") return "double";
    if (t == "u32") return "uint32_t";
    if (t == "u64") return "uint64_t";
    return "void";
}

struct ASTNode {
    virtual ~ASTNode() = default;
    virtual std::string cpp() const = 0;
};

// Literals
struct StringLiteral : ASTNode {
    std::string value;
    explicit StringLiteral(const std::string& v) : value(v) {}
    std::string cpp() const override { return "\"" + value + "\""; }
};

struct NumberLiteral : ASTNode {
    std::string value;
    explicit NumberLiteral(const std::string& v) : value(v) {}
    std::string cpp() const override { return value; }
};

struct FloatLiteral : ASTNode {
    std::string value;
    explicit FloatLiteral(const std::string& v) : value(v) {}
    std::string cpp() const override { return value; }
};

struct BoolLiteral : ASTNode {
    bool value;
    explicit BoolLiteral(bool v) : value(v) {}
    std::string cpp() const override { return value ? "true" : "false"; }
};

struct VariableRef : ASTNode {
    std::string name;
    explicit VariableRef(const std::string& n) : name(n) {}
    std::string cpp() const override { return name; }
};

// Expressions
struct BinaryExpr : ASTNode {
    std::string op;
    std::unique_ptr<ASTNode> left;
    std::unique_ptr<ASTNode> right;
    std::string cpp() const override {
        return left->cpp() + " " + op + " " + right->cpp();
    }
};

struct FunctionCall : ASTNode {
    std::string name;
    std::vector<std::unique_ptr<ASTNode>> args;
    int line = 0;

    std::string cpp() const override {
        std::string out = name + "(";
        for (size_t i = 0; i < args.size(); i++) {
            if (i > 0) out += ", ";
            out += args[i]->cpp();
        }
        return out + ")";
    }
};

// Statements
struct VariableDecl : ASTNode {
    std::string type;
    std::string name;
    std::unique_ptr<ASTNode> value;

    std::string cpp() const override {
        std::string t = type.empty() ? "auto" : map_type(type);
        return t + " " + name + " = " + value->cpp() + ";";
    }
};

struct ReturnStmt : ASTNode {
    std::unique_ptr<ASTNode> value;
    std::string cpp() const override {
        return "return " + value->cpp() + ";";
    }
};

// Function
struct FunctionParam {
    std::string type;
    std::string name;
};

struct FunctionDef : ASTNode {
    std::string name;
    std::vector<FunctionParam> params;
    std::string return_type;
    std::vector<std::unique_ptr<ASTNode>> body;

    std::string cpp() const override {
        std::string rt = return_type.empty() ? "void" : map_type(return_type);
        std::string out = rt + " " + name + "(";
        for (size_t i = 0; i < params.size(); i++) {
            if (i > 0) out += ", ";
            out += map_type(params[i].type) + " " + params[i].name;
        }
        out += ") {\n";
        for (const auto& stmt : body) {
            out += "    " + stmt->cpp() + "\n";
        }
        out += "}\n";
        return out;
    }
};

struct Program : ASTNode {
    std::vector<std::unique_ptr<FunctionDef>> functions;
    std::string cpp() const override {
        std::string out;
        for (const auto& fn : functions) {
            out += fn->cpp();
        }
        return out;
    }
};
