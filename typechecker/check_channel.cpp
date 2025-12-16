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
    (void)state;
    return {"Channel<" + channel.element_type + ">", false, false};
}

/**
 * Type checks a method call on a channel.
 * Returns the result type or unknown if the method doesn't exist.
 */
TypeInfo check_channel_method(TypeCheckerState& state, const MethodCall& mcall, const string& element_type) {
    if (mcall.method_name == "send") {
        if (mcall.args.size() != 1) {
            error(state, "Channel.send expects 1 argument, got " + to_string(mcall.args.size()), mcall.line);
        } else {
            TypeInfo arg_type = infer_type(state, *mcall.args[0]);
            TypeInfo expected = {element_type, false, false};

            if (!types_compatible(expected, arg_type)) {
                error(state, "Channel.send expects '" + element_type + "', got '" + arg_type.base_type + "'", mcall.line);
            }
        }

        return {"void", false, true};
    } else if (mcall.method_name == "recv") {
        if (!mcall.args.empty()) {
            error(state, "Channel.recv expects 0 arguments, got " + to_string(mcall.args.size()), mcall.line);
        }

        return {element_type, false, false};
    } else {
        error(state, "Channel has no method '" + mcall.method_name + "'", mcall.line);
        return {"unknown", false, false};
    }
}

} // namespace typechecker
