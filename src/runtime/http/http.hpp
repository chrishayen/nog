/**
 * @file http.hpp
 * @brief Nog HTTP runtime library (header-only).
 *
 * Provides async HTTP server functionality for Nog programs.
 * Uses ASIO for async I/O and llhttp for HTTP parsing.
 */

#pragma once

#include <string>
#include <vector>
#include <tuple>
#include <functional>
#include <iostream>
#include <asio.hpp>
#include <asio/awaitable.hpp>
#include <llhttp.h>

namespace http {

/**
 * HTTP request structure.
 */
struct Request {
    std::string method;
    std::string path;
    std::string body;
};

/**
 * HTTP response structure.
 */
struct Response {
    int status;
    std::string content_type;
    std::string body;
};

/**
 * HTTP parser context for llhttp.
 */
struct HttpParserContext {
    std::string method;
    std::string url;
    std::string body;
    bool message_complete = false;
};

namespace detail {

/**
 * llhttp callback for method.
 */
inline int on_method(llhttp_t* parser, const char* at, size_t len) {
    auto* ctx = static_cast<HttpParserContext*>(parser->data);
    ctx->method.assign(at, len);
    return 0;
}

/**
 * llhttp callback for URL.
 */
inline int on_url(llhttp_t* parser, const char* at, size_t len) {
    auto* ctx = static_cast<HttpParserContext*>(parser->data);
    ctx->url.assign(at, len);
    return 0;
}

/**
 * llhttp callback for body.
 */
inline int on_body(llhttp_t* parser, const char* at, size_t len) {
    auto* ctx = static_cast<HttpParserContext*>(parser->data);
    ctx->body.append(at, len);
    return 0;
}

/**
 * llhttp callback for message complete.
 */
inline int on_message_complete(llhttp_t* parser) {
    auto* ctx = static_cast<HttpParserContext*>(parser->data);
    ctx->message_complete = true;
    return 0;
}

}  // namespace detail

/**
 * Creates a 200 OK text/plain response.
 */
inline Response text(const std::string& content) {
    return Response{200, "text/plain", content};
}

/**
 * Creates a 200 OK application/json response.
 */
inline Response json(const std::string& content) {
    return Response{200, "application/json", content};
}

/**
 * Creates a 404 Not Found response.
 */
inline Response not_found() {
    return Response{404, "text/plain", "Not Found"};
}

/**
 * Formats an HTTP response for sending over the wire.
 */
inline std::string format_response(const Response& resp) {
    std::string status_text;

    switch (resp.status) {
        case 200: status_text = "OK"; break;
        case 404: status_text = "Not Found"; break;
        case 500: status_text = "Internal Server Error"; break;
        default: status_text = "Unknown"; break;
    }

    std::string response = "HTTP/1.1 " + std::to_string(resp.status) + " " + status_text + "\r\n";
    response += "Content-Type: " + resp.content_type + "\r\n";
    response += "Content-Length: " + std::to_string(resp.body.size()) + "\r\n";
    response += "Connection: close\r\n";
    response += "\r\n";
    response += resp.body;
    return response;
}

/**
 * Async request reader - reads until complete HTTP message is received.
 */
inline asio::awaitable<Request> read_request(asio::ip::tcp::socket& socket) {
    llhttp_t parser;
    llhttp_settings_t settings;
    llhttp_settings_init(&settings);
    settings.on_method = detail::on_method;
    settings.on_url = detail::on_url;
    settings.on_body = detail::on_body;
    settings.on_message_complete = detail::on_message_complete;

    HttpParserContext ctx;
    llhttp_init(&parser, HTTP_REQUEST, &settings);
    parser.data = &ctx;

    char buffer[8192];

    // First read - usually gets entire request for small GETs
    size_t n = co_await socket.async_read_some(asio::buffer(buffer), asio::use_awaitable);
    llhttp_errno err = llhttp_execute(&parser, buffer, n);

    if (err != HPE_OK && !ctx.message_complete) {
        co_return Request{"GET", "/", ""};
    }

    // Continue reading only if message incomplete (large POST, slow client)
    while (!ctx.message_complete) {
        n = co_await socket.async_read_some(asio::buffer(buffer), asio::use_awaitable);
        err = llhttp_execute(&parser, buffer, n);

        if (err != HPE_OK && !ctx.message_complete) {
            co_return Request{"GET", "/", ""};
        }
    }

    co_return Request{ctx.method, ctx.url, ctx.body};
}

/**
 * Handle a single connection with an async handler.
 */
template<typename Handler>
asio::awaitable<void> handle_connection_async(asio::ip::tcp::socket socket, Handler handler) {
    try {
        Request req = co_await read_request(socket);
        Response resp = co_await handler(req);

        std::string response_str = format_response(resp);
        co_await asio::async_write(socket, asio::buffer(response_str), asio::use_awaitable);
    } catch (const std::exception& e) {
        // Connection closed or error
    }
}

/**
 * Main serve function - simple single-handler version.
 * Handler must be an async function returning asio::awaitable<Response>.
 */
template<typename Handler>
asio::awaitable<void> serve(int port, Handler handler) {
    auto executor = co_await asio::this_coro::executor;
    asio::ip::tcp::acceptor acceptor(executor);

    try {
        acceptor.open(asio::ip::tcp::v4());
        acceptor.set_option(asio::socket_base::reuse_address(true));
        acceptor.bind({asio::ip::tcp::v4(), static_cast<asio::ip::port_type>(port)});
        acceptor.listen();
    } catch (const std::system_error& e) {
        std::cerr << "Error: Failed to bind to port " << port << ": " << e.what() << std::endl;
        co_return;
    }

    std::cout << "HTTP server listening on port " << port << std::endl;

    while (true) {
        asio::ip::tcp::socket socket = co_await acceptor.async_accept(asio::use_awaitable);
        asio::co_spawn(executor, handle_connection_async(std::move(socket), handler), asio::detached);
    }
}

/**
 * App struct for routing-based HTTP server.
 */
struct App {
    std::vector<std::tuple<std::string, std::string, std::function<asio::awaitable<Response>(Request)>>> routes;

    /**
     * Register a GET route handler.
     */
    void get(const std::string& path, std::function<asio::awaitable<Response>(Request)> handler) {
        routes.push_back({"GET", path, handler});
    }

    /**
     * Register a POST route handler.
     */
    void post(const std::string& path, std::function<asio::awaitable<Response>(Request)> handler) {
        routes.push_back({"POST", path, handler});
    }

    /**
     * Route a request to the appropriate handler.
     */
    asio::awaitable<Response> route(const Request& req) {
        for (const auto& [method, path, handler] : routes) {
            if (req.method == method && req.path == path) {
                co_return co_await handler(req);
            }
        }

        co_return not_found();
    }

    /**
     * Start listening on the given port.
     */
    asio::awaitable<void> listen(int port) {
        auto executor = co_await asio::this_coro::executor;
        asio::ip::tcp::acceptor acceptor(executor);

        try {
            acceptor.open(asio::ip::tcp::v4());
            acceptor.set_option(asio::socket_base::reuse_address(true));
            acceptor.bind({asio::ip::tcp::v4(), static_cast<asio::ip::port_type>(port)});
            acceptor.listen();
        } catch (const std::system_error& e) {
            std::cerr << "Error: Failed to bind to port " << port << ": " << e.what() << std::endl;
            co_return;
        }

        std::cout << "HTTP server listening on port " << port << std::endl;

        while (true) {
            asio::ip::tcp::socket socket = co_await acceptor.async_accept(asio::use_awaitable);
            auto self = this;

            asio::co_spawn(executor, [self, s = std::move(socket)]() mutable -> asio::awaitable<void> {
                co_await handle_connection_async(std::move(s), [self](const Request& req) {
                    return self->route(req);
                });
            }(), asio::detached);
        }
    }
};

}  // namespace http
