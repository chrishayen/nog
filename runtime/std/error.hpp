/**
 * @file error.hpp
 * @brief Error handling primitives for Nog.
 *
 * Provides the base Error type and Result<T> wrapper for fallible functions.
 * All error types inherit from Error and can be checked with dynamic_cast.
 */

#pragma once

#include <string>
#include <memory>
#include <variant>
#include <optional>

namespace nog::rt {

/**
 * Base error type that all errors inherit from.
 * Contains message and optional cause for error chaining.
 */
struct Error {
    std::string message;
    std::shared_ptr<Error> cause;

    /**
     * Construct error with message only.
     */
    Error(const std::string& msg) : message(msg), cause(nullptr) {}

    /**
     * Construct error with message and cause.
     */
    Error(const std::string& msg, std::shared_ptr<Error> c)
        : message(msg), cause(c) {}

    /**
     * Get the root cause of the error chain.
     * Returns this error if there is no cause.
     */
    Error* root_cause() {
        if (!cause) {
            return this;
        }

        return cause->root_cause();
    }

    virtual ~Error() = default;
};

/**
 * Result type for fallible functions.
 * Holds either a value of type T or an error.
 */
template<typename T>
class Result {
private:
    std::variant<T, std::shared_ptr<Error>> data;

public:
    /**
     * Construct with success value.
     */
    Result(T val) : data(std::move(val)) {}

    /**
     * Construct with error.
     */
    Result(std::shared_ptr<Error> err) : data(std::move(err)) {}

    /**
     * Check if result contains an error.
     */
    bool is_error() const {
        return std::holds_alternative<std::shared_ptr<Error>>(data);
    }

    /**
     * Check if result contains a value.
     */
    bool is_ok() const {
        return !is_error();
    }

    /**
     * Get the success value. Undefined if is_error().
     */
    T& value() {
        return std::get<T>(data);
    }

    /**
     * Get the success value (const). Undefined if is_error().
     */
    const T& value() const {
        return std::get<T>(data);
    }

    /**
     * Get the error. Undefined if is_ok().
     */
    std::shared_ptr<Error>& error() {
        return std::get<std::shared_ptr<Error>>(data);
    }

    /**
     * Get the error (const). Undefined if is_ok().
     */
    const std::shared_ptr<Error>& error() const {
        return std::get<std::shared_ptr<Error>>(data);
    }

    /**
     * Bool conversion returns true if result is ok.
     */
    explicit operator bool() const {
        return is_ok();
    }
};

/**
 * Specialization for void return type.
 * Just holds optional error.
 */
template<>
class Result<void> {
private:
    std::shared_ptr<Error> err;

public:
    /**
     * Construct success (no error).
     */
    Result() : err(nullptr) {}

    /**
     * Construct with error.
     */
    Result(std::shared_ptr<Error> e) : err(std::move(e)) {}

    /**
     * Check if result contains an error.
     */
    bool is_error() const {
        return err != nullptr;
    }

    /**
     * Check if result is successful.
     */
    bool is_ok() const {
        return !is_error();
    }

    /**
     * Get the error. Undefined if is_ok().
     */
    std::shared_ptr<Error>& error() {
        return err;
    }

    /**
     * Get the error (const). Undefined if is_ok().
     */
    const std::shared_ptr<Error>& error() const {
        return err;
    }

    /**
     * Bool conversion returns true if result is ok.
     */
    explicit operator bool() const {
        return is_ok();
    }
};

/**
 * Helper to create error result.
 */
template<typename T>
Result<T> make_error(const std::string& msg) {
    return Result<T>(std::make_shared<Error>(msg));
}

/**
 * Helper to create error result with existing error pointer.
 */
template<typename T, typename E>
Result<T> make_error(std::shared_ptr<E> err) {
    return Result<T>(std::static_pointer_cast<Error>(err));
}

}  // namespace nog::rt
