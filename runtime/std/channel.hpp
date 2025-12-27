/**
 * @file channel.hpp
 * @brief Channel implementation for Bishop fiber communication.
 *
 * This header includes boost fiber headers. Only included when
 * the program uses Channel types.
 */

#pragma once

#include <boost/fiber/all.hpp>
#include <utility>

namespace bishop::rt {

/**
 * Typed channel for communication between fibers.
 * Wraps boost::fibers::buffered_channel with send/recv API.
 */
template<typename T>
class Channel {
public:
    Channel() : ch_(2) {}  // capacity must be power of 2, min 2 for boost::fibers

    /**
     * Send a value through the channel. Blocks until space available.
     */
    void send(const T& value) {
        ch_.push(value);
    }

    /**
     * Receive a value from the channel. Blocks until available.
     */
    T recv() {
        T value;
        ch_.pop(value);
        return value;
    }

    /**
     * Try to receive a value without blocking. Returns pair<bool, T>.
     */
    std::pair<bool, T> try_recv() {
        T value;
        auto status = ch_.try_pop(value);
        if (status == boost::fibers::channel_op_status::success) {
            return {true, value};
        }
        return {false, T{}};
    }

    /**
     * Close the channel.
     */
    void close() {
        ch_.close();
    }

private:
    boost::fibers::buffered_channel<T> ch_;
};

}  // namespace bishop::rt
