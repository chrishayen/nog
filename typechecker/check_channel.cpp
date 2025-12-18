/**
 * @file check_channel.cpp
 * @brief Channel type inference for the Nog type checker.
 */

#include "typechecker.hpp"

using namespace std;

namespace typechecker {

/**
 * Infers the type of a channel creation expression.
 */
TypeInfo check_channel_create(TypeCheckerState& state, const ChannelCreate& channel) {
    if (!state.in_async_context) {
        // Channel creation uses `co_await asio::this_coro::executor` in codegen, so it
        // must occur inside a coroutine (i.e., an async function/method).
        error(state, "Channel creation can only be used inside async functions", channel.line);
    }

    if (!is_valid_type(state, channel.element_type)) {
        error(state, "unknown channel element type '" + channel.element_type + "'", channel.line);
    }

    return {"Channel<" + channel.element_type + ">", false, false};
}

/**
 * Type checks a method call on a channel.
 * Returns the result type or unknown if the method doesn't exist.
 */
TypeInfo check_channel_method(TypeCheckerState& state, const MethodCall& mcall, const string& element_type) {
    if (!state.in_async_context) {
        error(state, "Channel." + mcall.method_name + " can only be used inside async functions", mcall.line);
    }

    if (mcall.method_name == "send") {
        if (mcall.args.size() != 1) {
            error(state, "Channel.send expects 1 argument, got " + to_string(mcall.args.size()), mcall.line);
        } else {
            TypeInfo arg_type = infer_type(state, *mcall.args[0]);
            TypeInfo expected = {element_type, false, false};

            if (!types_compatible(expected, arg_type)) {
                error(state, "Channel.send expects '" + element_type + "', got '" + format_type(arg_type) + "'", mcall.line);
            }
        }

        return make_awaitable({"void", false, true});
    } else if (mcall.method_name == "recv") {
        if (!mcall.args.empty()) {
            error(state, "Channel.recv expects 0 arguments, got " + to_string(mcall.args.size()), mcall.line);
        }

        return make_awaitable({element_type, false, false});
    } else {
        error(state, "Channel has no method '" + mcall.method_name + "'", mcall.line);
        return {"unknown", false, false};
    }
}

} // namespace typechecker
