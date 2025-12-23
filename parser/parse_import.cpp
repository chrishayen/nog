/**
 * @file parse_import.cpp
 * @brief Import parsing and definition prescanning for the Nog parser.
 *
 * Handles import statements, prescan for forward references,
 * doc comment collection, and name lookup utilities.
 */

#include "parser.hpp"

using namespace std;

namespace parser {

/**
 * Checks if the given name is an imported module alias.
 */
bool is_imported_module(const ParserState& state, const string& name) {
    for (const auto& m : state.imported_modules) {
        if (m == name) return true;
    }

    return false;
}

/**
 * Checks if the given name is a known function.
 */
bool is_function_name(const ParserState& state, const string& name) {
    for (const auto& f : state.function_names) {
        if (f == name) return true;
    }

    return false;
}

/**
 * Pre-scans the token stream to collect function and struct names.
 * This allows forward references to functions defined later in the file.
 */
void prescan_definitions(ParserState& state) {
    size_t saved_pos = state.pos;

    while (!check(state, TokenType::EOF_TOKEN)) {
        // Function: fn name(...)
        if (check(state, TokenType::FN)) {
            advance(state);

            if (check(state, TokenType::IDENT)) {
                state.function_names.push_back(current(state).value);
            }
        }

        // Struct: Name :: struct or Error: Name :: err
        if (check(state, TokenType::IDENT)) {
            string name = current(state).value;
            advance(state);

            if (check(state, TokenType::DOUBLE_COLON)) {
                advance(state);

                // Both structs and errors can be instantiated with { field: value }
                if (check(state, TokenType::STRUCT) || check(state, TokenType::ERR)) {
                    state.struct_names.push_back(name);
                }
            }
        }

        advance(state);
    }

    state.pos = saved_pos;
}

/**
 * @nog_syntax Doc Comments
 * @category Visibility
 * @order 2
 * @description Document functions, structs, and fields with /// comments.
 * @syntax /// description
 * @example
 * /// This is a doc comment for the function
 * fn add(int a, int b) -> int {
 *     return a + b;
 * }
 */
string collect_doc_comments(ParserState& state) {
    string doc;

    while (check(state, TokenType::DOC_COMMENT)) {
        if (!doc.empty()) {
            doc += "\n";
        }
        doc += current(state).value;
        advance(state);
    }

    return doc;
}

/**
 * @nog_syntax import
 * @category Imports
 * @order 1
 * @description Import a module to use its types and functions.
 * @syntax import module_name;
 * @example
 * import http;
 * import myproject.utils;
 * @note Use dot notation for nested modules
 */
unique_ptr<ImportStmt> parse_import(ParserState& state) {
    Token import_tok = consume(state, TokenType::IMPORT);

    // Parse module path: math or utils.helpers
    string module_path = consume(state, TokenType::IDENT).value;

    while (check(state, TokenType::DOT)) {
        advance(state);
        module_path += "." + consume(state, TokenType::IDENT).value;
    }

    consume(state, TokenType::SEMICOLON);

    auto import = make_unique<ImportStmt>(module_path);
    import->line = import_tok.line;
    state.imported_modules.push_back(import->alias);
    return import;
}

} // namespace parser
