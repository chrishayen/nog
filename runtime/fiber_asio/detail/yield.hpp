/**
 * @file detail/yield.hpp
 * @brief Boost.Fiber async_result integration for Boost.Asio.
 *
 * Based on Boost.Fiber examples by Oliver Kowalke and Nat Goodspeed.
 * Modified for boost.asio >= 1.70.
 *
 * Copyright Oliver Kowalke, Nat Goodspeed 2015.
 * Distributed under the Boost Software License, Version 1.0.
 */

#ifndef NOG_FIBER_ASIO_DETAIL_YIELD_HPP
#define NOG_FIBER_ASIO_DETAIL_YIELD_HPP

#include <mutex>

#include <boost/asio/async_result.hpp>
#include <boost/asio/detail/config.hpp>
#include <boost/assert.hpp>
#include <boost/atomic.hpp>
#include <boost/intrusive_ptr.hpp>
#include <boost/system/error_code.hpp>
#include <boost/system/system_error.hpp>
#include <boost/throw_exception.hpp>
#include <boost/fiber/all.hpp>

#include "../yield.hpp"

namespace boost {
namespace fibers {
namespace asio {
namespace detail {

/**
 * Tracks completion state of an async operation.
 */
struct yield_completion {
    enum state_t { init, waiting, complete };

    typedef fibers::detail::spinlock mutex_t;
    typedef std::unique_lock<mutex_t> lock_t;
    typedef boost::intrusive_ptr<yield_completion> ptr_t;

    std::atomic<std::size_t> use_count_{0};
    mutex_t mtx_{};
    state_t state_{init};

    /**
     * Suspends fiber until async operation completes.
     */
    void wait() {
        lock_t lk{mtx_};

        if (complete != state_) {
            state_ = waiting;
            fibers::context::active()->suspend(lk);
        }
    }

    friend void intrusive_ptr_add_ref(yield_completion* yc) noexcept {
        BOOST_ASSERT(nullptr != yc);
        yc->use_count_.fetch_add(1, std::memory_order_relaxed);
    }

    friend void intrusive_ptr_release(yield_completion* yc) noexcept {
        BOOST_ASSERT(nullptr != yc);

        if (1 == yc->use_count_.fetch_sub(1, std::memory_order_release)) {
            std::atomic_thread_fence(std::memory_order_acquire);
            delete yc;
        }
    }
};

/**
 * Base handler for async operations that suspends fiber.
 */
class yield_handler_base {
public:
    yield_handler_base(yield_t const& y) :
        ctx_{boost::fibers::context::active()},
        yt_(y) {
    }

    void operator()(boost::system::error_code const& ec) {
        BOOST_ASSERT_MSG(ycomp_, "Must inject yield_completion* before calling");
        BOOST_ASSERT_MSG(yt_.ec_, "Must inject boost::system::error_code*");

        yield_completion::lock_t lk{ycomp_->mtx_};
        yield_completion::state_t state = ycomp_->state_;
        ycomp_->state_ = yield_completion::complete;
        *yt_.ec_ = ec;
        lk.unlock();

        if (yield_completion::waiting == state) {
            fibers::context::active()->schedule(ctx_);
        }
    }

    boost::fibers::context* ctx_;
    yield_t yt_;
    yield_completion::ptr_t ycomp_{};
};

/**
 * Typed handler for async operations returning a value.
 */
template<typename T>
class yield_handler : public yield_handler_base {
public:
    explicit yield_handler(yield_t const& y) :
        yield_handler_base{y} {
    }

    void operator()(T t) {
        (*this)(boost::system::error_code(), std::move(t));
    }

    void operator()(boost::system::error_code const& ec, T t) {
        BOOST_ASSERT_MSG(value_, "Must inject value ptr before calling");
        *value_ = std::move(t);
        yield_handler_base::operator()(ec);
    }

    T* value_{nullptr};
};

/**
 * Specialized handler for void async operations.
 */
template<>
class yield_handler<void> : public yield_handler_base {
public:
    explicit yield_handler(yield_t const& y) :
        yield_handler_base{y} {
    }

    void operator()() {
        (*this)(boost::system::error_code());
    }

    using yield_handler_base::operator();
};

template<typename Fn, typename T>
void asio_handler_invoke(Fn&& fn, yield_handler<T>*) {
    fn();
}

/**
 * Base async_result that manages fiber suspension.
 */
class async_result_base {
public:
    explicit async_result_base(yield_handler_base& h) :
        ycomp_{new yield_completion{}} {
        h.ycomp_ = this->ycomp_;

        if (!h.yt_.ec_) {
            h.yt_.ec_ = &ec_;
        }
    }

    void get() {
        ycomp_->wait();

        if (ec_) {
            throw_exception(boost::system::system_error{ec_});
        }
    }

private:
    boost::system::error_code ec_{};
    yield_completion::ptr_t ycomp_;
};

}  // namespace detail
}  // namespace asio
}  // namespace fibers
}  // namespace boost

namespace boost {
namespace asio {

/**
 * async_result specialization for fiber yield with return value.
 */
template<typename ReturnType, typename T>
class async_result<boost::fibers::asio::yield_t, ReturnType(boost::system::error_code, T)> :
    public boost::fibers::asio::detail::async_result_base {
public:
    using return_type = T;
    using completion_handler_type = fibers::asio::detail::yield_handler<T>;

    explicit async_result(boost::fibers::asio::detail::yield_handler<T>& h) :
        boost::fibers::asio::detail::async_result_base{h} {
        h.value_ = &value_;
    }

    return_type get() {
        boost::fibers::asio::detail::async_result_base::get();
        return std::move(value_);
    }

private:
    return_type value_{};
};

/**
 * async_result specialization for fiber yield with void return.
 */
template<>
class async_result<boost::fibers::asio::yield_t, void(boost::system::error_code)> :
    public boost::fibers::asio::detail::async_result_base {
public:
    using return_type = void;
    using completion_handler_type = fibers::asio::detail::yield_handler<void>;

    explicit async_result(boost::fibers::asio::detail::yield_handler<void>& h) :
        boost::fibers::asio::detail::async_result_base{h} {
    }
};

}  // namespace asio
}  // namespace boost

#endif  // NOG_FIBER_ASIO_DETAIL_YIELD_HPP
