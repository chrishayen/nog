#pragma once
#include <string>
#include <vector>
#include <memory>

struct ASTNode {
    virtual ~ASTNode() = default;
};

struct StringLiteral : ASTNode {
    std::string value;
    explicit StringLiteral(const std::string& v) : value(v) {}
};

struct FunctionCall : ASTNode {
    std::string name;
    std::vector<std::unique_ptr<ASTNode>> args;
    int line = 0;
};

struct FunctionDef : ASTNode {
    std::string name;
    std::vector<std::unique_ptr<ASTNode>> body;
};

struct Program : ASTNode {
    std::vector<std::unique_ptr<FunctionDef>> functions;
};
