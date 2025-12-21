/**
 * @file round_robin.hpp
 * @brief Fiber scheduler integrated with Boost.Asio.
 *
 * Based on Boost.Fiber examples by Oliver Kowalke.
 *
 * Copyright Oliver Kowalke 2013.
 * Distributed under the Boost Software License, Version 1.0.
 */

#ifndef NOG_FIBER_ASIO_ROUND_ROBIN_HPP
#define NOG_FIBER_ASIO_ROUND_ROBIN_HPP

#include <chrono>
#include <cstddef>
#include <memory>
#include <mutex>
#include <queue>

#include <boost/asio.hpp>
#include <boost/assert.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/config.hpp>

#include <boost/fiber/condition_variable.hpp>
#include <boost/fiber/context.hpp>
#include <boost/fiber/mutex.hpp>
#include <boost/fiber/operations.hpp>
#include <boost/fiber/scheduler.hpp>

#include "yield.hpp"

namespace boost {
namespace fibers {
namespace asio {

/**
 * Round-robin fiber scheduler integrated with Boost.Asio.
 *
 * This scheduler allows fibers to yield during async I/O operations.
 * When a fiber calls an async operation with boost::fibers::asio::yield,
 * it suspends and other fibers can run until the I/O completes.
 */
class round_robin : public algo::algorithm {
private:
    std::shared_ptr<boost::asio::io_context> io_ctx_;
    boost::asio::steady_timer suspend_timer_;
    boost::fibers::scheduler::ready_queue_type rqueue_{};
    boost::fibers::mutex mtx_{};
    boost::fibers::condition_variable cnd_{};
    std::size_t counter_{0};

public:
    /**
     * Service to keep io_context alive during fiber execution.
     */
    struct service : public boost::asio::execution_context::service {
        static boost::asio::execution_context::id id;

        using work_guard_type = boost::asio::executor_work_guard<
            boost::asio::io_context::executor_type>;
        std::unique_ptr<work_guard_type> work_;

        service(boost::asio::execution_context& ctx) :
            boost::asio::execution_context::service(ctx),
            work_{std::make_unique<work_guard_type>(
                static_cast<boost::asio::io_context&>(ctx).get_executor())} {
        }

        virtual ~service() {}

        service(service const&) = delete;
        service& operator=(service const&) = delete;

        void shutdown() noexcept override {
            work_.reset();
        }
    };

    /**
     * Constructs scheduler with given io_context.
     */
    round_robin(std::shared_ptr<boost::asio::io_context> const& io_ctx) :
        io_ctx_(io_ctx),
        suspend_timer_(*io_ctx_) {
        boost::asio::make_service<service>(*io_ctx_);
        boost::asio::post(*io_ctx_, [this]() mutable {
            while (!io_ctx_->stopped()) {
                if (has_ready_fibers()) {
                    while (io_ctx_->poll());
                    std::unique_lock<boost::fibers::mutex> lk(mtx_);
                    cnd_.wait(lk);
                } else {
                    if (!io_ctx_->run_one()) {
                        break;
                    }
                }
            }
        });
    }

    /**
     * Called when a fiber becomes ready to run.
     */
    void awakened(context* ctx) noexcept {
        BOOST_ASSERT(nullptr != ctx);
        BOOST_ASSERT(!ctx->ready_is_linked());
        ctx->ready_link(rqueue_);

        if (!ctx->is_context(boost::fibers::type::dispatcher_context)) {
            ++counter_;
        }
    }

    /**
     * Returns the next fiber to run.
     */
    context* pick_next() noexcept {
        context* ctx(nullptr);

        if (!rqueue_.empty()) {
            ctx = &rqueue_.front();
            rqueue_.pop_front();
            BOOST_ASSERT(nullptr != ctx);
            BOOST_ASSERT(context::active() != ctx);

            if (!ctx->is_context(boost::fibers::type::dispatcher_context)) {
                --counter_;
            }
        }

        return ctx;
    }

    /**
     * Returns true if any fibers are ready to run.
     */
    bool has_ready_fibers() const noexcept {
        return 0 < counter_;
    }

    /**
     * Suspends scheduler until given time point.
     */
    void suspend_until(std::chrono::steady_clock::time_point const& abs_time) noexcept {
        if ((std::chrono::steady_clock::time_point::max)() != abs_time) {
            suspend_timer_.expires_at(abs_time);
            suspend_timer_.async_wait([](boost::system::error_code const&) {
                this_fiber::yield();
            });
        }

        cnd_.notify_one();
    }

    /**
     * Notifies scheduler that a fiber is ready.
     */
    void notify() noexcept {
        suspend_timer_.async_wait([](boost::system::error_code const&) {
            this_fiber::yield();
        });
        suspend_timer_.expires_at(std::chrono::steady_clock::now());
    }
};

inline boost::asio::execution_context::id round_robin::service::id;

}  // namespace asio
}  // namespace fibers
}  // namespace boost

#endif  // NOG_FIBER_ASIO_ROUND_ROBIN_HPP
