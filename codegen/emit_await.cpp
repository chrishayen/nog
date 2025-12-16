/**
 * @file emit_await.cpp
 * @brief Await expression emission for the Nog code generator.
 */

#include "codegen.hpp"
#include <fmt/format.h>

using namespace std;

namespace codegen {

/**
 * Emits an await expression. Handles channel recv specially.
 */
string emit_await(CodeGenState& state, const AwaitExpr& expr) {
    // Special handling for channel recv: await ch.recv() -> std::get<1>(co_await ch.async_receive(...))
    if (auto* mcall = dynamic_cast<const MethodCall*>(expr.value.get())) {
        if (mcall->method_name == "recv") {
            return "std::get<1>(co_await " + emit(state, *mcall->object) +
                   ".async_receive(asio::as_tuple(asio::use_awaitable)))";
        }
    }

    // Wrap in parentheses since co_await has low precedence in C++
    return "(co_await " + emit(state, *expr.value) + ")";
}

} // namespace codegen
