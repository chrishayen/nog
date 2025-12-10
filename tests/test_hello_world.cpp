#include <gtest/gtest.h>
#include "lexer.hpp"
#include "parser.hpp"
#include "codegen.hpp"

TEST(Transpile, HelloWorld) {
    std::string source = R"(
fn main() {
    print("Hello, World!")
}
)";

    Lexer lexer(source);
    auto tokens = lexer.tokenize();

    Parser parser(tokens);
    auto ast = parser.parse();

    CodeGen codegen;
    std::string output = codegen.generate(ast);

    std::string expected = R"(#include <iostream>

int main() {
    std::cout << "Hello, World!" << std::endl;
    return 0;
}
)";

    EXPECT_EQ(output, expected);
}
