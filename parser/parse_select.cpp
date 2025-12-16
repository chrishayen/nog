/**
 * @file parse_select.cpp
 * @brief Select statement parsing for the Nog parser.
 */

#include "parser.hpp"

using namespace std;

namespace parser {

/**
 * @nog_syntax select
 * @category Channels
 * @order 4
 * @description Wait on multiple channel operations.
 * @syntax select { case val := ch.recv() { ... } }
 * @example
 * select {
 *     case val := ch1.recv() {
 *         x := val + 1;
 *     }
 *     case val := ch2.recv() {
 *         x := val + 2;
 *     }
 * }
 */
unique_ptr<SelectStmt> parse_select(ParserState& state) {
    int start_line = current(state).line;
    consume(state, TokenType::SELECT);
    consume(state, TokenType::LBRACE);

    auto stmt = make_unique<SelectStmt>();
    stmt->line = start_line;

    while (check(state, TokenType::CASE)) {
        auto select_case = make_unique<SelectCase>();
        select_case->line = current(state).line;
        advance(state);  // consume 'case'

        // Parse case: either "val := ch.recv()" or "ch.send(value)"
        // First check for binding: val := ...
        if (check(state, TokenType::IDENT)) {
            size_t saved_pos = state.pos;
            string first_ident = current(state).value;
            advance(state);

            if (check(state, TokenType::COLON_ASSIGN)) {
                // This is a recv with binding: val := ch.recv()
                select_case->binding_name = first_ident;
                advance(state);  // consume :=

                // Now parse the channel.recv()
                select_case->channel = parse_primary(state);
                select_case->channel = parse_postfix(state, move(select_case->channel));

                // The channel expression should be a MethodCall with method_name "recv"
                if (auto* method_call = dynamic_cast<MethodCall*>(select_case->channel.get())) {
                    select_case->operation = method_call->method_name;

                    // For recv, we need to keep the channel object, not the method call
                    auto channel_obj = move(method_call->object);
                    select_case->channel = move(channel_obj);
                }
            } else if (check(state, TokenType::DOT)) {
                // This is either ch.send(value) or ch.recv() without binding
                state.pos = saved_pos;

                // Parse channel expression (identifier + method call)
                auto expr = parse_primary(state);
                expr = parse_postfix(state, move(expr));

                if (auto* method_call = dynamic_cast<MethodCall*>(expr.get())) {
                    select_case->operation = method_call->method_name;

                    if (method_call->method_name == "send" && !method_call->args.empty()) {
                        select_case->send_value = move(method_call->args[0]);
                    }

                    select_case->channel = move(method_call->object);
                }
            } else {
                state.pos = saved_pos;
            }
        }

        // Parse the case body: { statements }
        consume(state, TokenType::LBRACE);

        while (!check(state, TokenType::RBRACE) && !check(state, TokenType::EOF_TOKEN)) {
            auto s = parse_statement(state);

            if (s) {
                select_case->body.push_back(move(s));
            }
        }

        consume(state, TokenType::RBRACE);
        stmt->cases.push_back(move(select_case));
    }

    consume(state, TokenType::RBRACE);
    return stmt;
}

} // namespace parser
