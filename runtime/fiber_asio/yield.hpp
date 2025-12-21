/**
 * @file yield.hpp
 * @brief Fiber yield token for Boost.Asio async operations.
 *
 * Based on Boost.Fiber examples by Christopher M. Kohlhoff, Oliver Kowalke,
 * and Nat Goodspeed.
 *
 * Copyright 2003-2013 Christopher M. Kohlhoff
 * Copyright Oliver Kowalke, Nat Goodspeed 2015.
 * Distributed under the Boost Software License, Version 1.0.
 */

#ifndef NOG_FIBER_ASIO_YIELD_HPP
#define NOG_FIBER_ASIO_YIELD_HPP

#include <boost/config.hpp>
#include <boost/system/error_code.hpp>

namespace boost {
namespace fibers {
namespace asio {

/**
 * Yield token for fiber-aware async operations.
 *
 * Pass this to async operations to suspend the current fiber
 * until the operation completes.
 *
 * Usage:
 *   socket.async_read_some(buffer, boost::fibers::asio::yield);
 *
 * With error code binding:
 *   boost::system::error_code ec;
 *   socket.async_read_some(buffer, boost::fibers::asio::yield[ec]);
 */
class yield_t {
public:
    yield_t() = default;

    /**
     * Binds an error_code to receive errors instead of throwing.
     */
    yield_t operator[](boost::system::error_code& ec) const {
        yield_t tmp;
        tmp.ec_ = &ec;
        return tmp;
    }

    boost::system::error_code* ec_{nullptr};
};

/**
 * Thread-local yield instance for fiber async operations.
 */
thread_local inline yield_t yield{};

}  // namespace asio
}  // namespace fibers
}  // namespace boost

#include "detail/yield.hpp"

#endif  // NOG_FIBER_ASIO_YIELD_HPP
