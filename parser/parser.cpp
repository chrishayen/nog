/**
 * @file parser.cpp
 * @brief Main parser entry point and token navigation.
 *
 * Contains the main parse() function and token navigation helpers.
 * Grammar parsing is split across separate files by concern.
 */

#include "parser.hpp"
#include <stdexcept>

using namespace std;

namespace parser {

/**
 * Returns the current token, or EOF_TOKEN if past end.
 */
Token current(const ParserState& state) {
    if (state.pos >= state.tokens.size()) {
        return {TokenType::EOF_TOKEN, "", 0};
    }

    return state.tokens[state.pos];
}

/**
 * Checks if current token matches the given type.
 */
bool check(const ParserState& state, TokenType type) {
    return current(state).type == type;
}

/**
 * Advances to the next token.
 */
void advance(ParserState& state) {
    state.pos++;
}

/**
 * Consumes and returns the current token if it matches type.
 * Throws runtime_error if the token doesn't match.
 */
Token consume(ParserState& state, TokenType type) {
    if (!check(state, type)) {
        Token tok = current(state);
        string msg = "unexpected token";

        if (!tok.value.empty()) {
            msg += " '" + tok.value + "'";
        }

        msg += " at line " + to_string(tok.line);
        throw runtime_error(msg);
    }

    Token tok = current(state);
    advance(state);
    return tok;
}

/**
 * Main parsing entry point. Parses the complete token stream into a Program AST.
 * Parses imports first, then function definitions (fn), struct definitions (Name :: struct),
 * and method definitions (Name :: method_name).
 */
unique_ptr<Program> parse(ParserState& state) {
    auto program = make_unique<Program>();

    // Pre-scan to collect all function and struct names for forward references
    prescan_definitions(state);

    // Parse imports first (must be at top of file)
    while (check(state, TokenType::IMPORT)) {
        program->imports.push_back(parse_import(state));
    }

    while (!check(state, TokenType::EOF_TOKEN)) {
        // Collect any doc comments before the definition
        string doc = collect_doc_comments(state);

        // Check for @extern("lib") annotation
        if (check(state, TokenType::AT)) {
            size_t at_pos = state.pos;
            advance(state);

            if (check(state, TokenType::EXTERN)) {
                advance(state);
                consume(state, TokenType::LPAREN);
                string library = consume(state, TokenType::STRING).value;
                consume(state, TokenType::RPAREN);

                auto ext = parse_extern_function(state, library);
                ext->doc_comment = doc;
                program->externs.push_back(move(ext));
                continue;
            }

            // Not @extern, restore position for visibility parsing
            state.pos = at_pos;
        }

        // Check for visibility annotation
        Visibility vis = parse_visibility(state);

        if (check(state, TokenType::FN)) {
            auto fn = parse_function(state, vis);
            fn->doc_comment = doc;
            program->functions.push_back(move(fn));
            continue;
        }

        if (!check(state, TokenType::IDENT)) {
            advance(state);
            continue;
        }

        // Check for struct definition: Name :: struct { ... }
        // or method definition: Name :: method_name(...) -> type { ... }
        size_t saved_pos = state.pos;
        Token name_tok = current(state);
        string name = name_tok.value;
        advance(state);

        if (!check(state, TokenType::DOUBLE_COLON)) {
            state.pos = saved_pos;
            advance(state);
            continue;
        }

        advance(state);

        if (check(state, TokenType::STRUCT)) {
            auto s = parse_struct_def(state, name, vis);
            s->doc_comment = doc;
            program->structs.push_back(move(s));
            continue;
        }

        if (check(state, TokenType::IDENT)) {
            auto m = parse_method_def(state, name, vis);
            m->doc_comment = doc;
            program->methods.push_back(move(m));
            continue;
        }

        state.pos = saved_pos;
        advance(state);
    }

    return program;
}

} // namespace parser
