/**
 * @file http.cpp
 * @brief Nog HTTP runtime library implementation.
 *
 * Contains non-template function implementations for the HTTP module.
 * Compiled into libnog_http.a for linking with user programs.
 */

#include <nog/http.hpp>

namespace http {

namespace detail {

int on_method(llhttp_t* parser, const char* at, size_t len) {
    auto* ctx = static_cast<HttpParserContext*>(parser->data);
    ctx->method.assign(at, len);
    return 0;
}

int on_url(llhttp_t* parser, const char* at, size_t len) {
    auto* ctx = static_cast<HttpParserContext*>(parser->data);
    ctx->url.assign(at, len);
    return 0;
}

int on_body(llhttp_t* parser, const char* at, size_t len) {
    auto* ctx = static_cast<HttpParserContext*>(parser->data);
    ctx->body.append(at, len);
    return 0;
}

int on_message_complete(llhttp_t* parser) {
    auto* ctx = static_cast<HttpParserContext*>(parser->data);
    ctx->message_complete = true;
    return 0;
}

}  // namespace detail

Response text(const std::string& content) {
    return Response{200, "text/plain", content};
}

Response json(const std::string& content) {
    return Response{200, "application/json", content};
}

Response not_found() {
    return Response{404, "text/plain", "Not Found"};
}

std::string format_response(const Response& resp) {
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

asio::awaitable<Request> read_request(asio::ip::tcp::socket& socket) {
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

void App::get(const std::string& path, std::function<asio::awaitable<Response>(Request)> handler) {
    routes.push_back({"GET", path, handler});
}

void App::post(const std::string& path, std::function<asio::awaitable<Response>(Request)> handler) {
    routes.push_back({"POST", path, handler});
}

asio::awaitable<Response> App::route(const Request& req) {
    for (const auto& [method, path, handler] : routes) {
        if (req.method == method && req.path == path) {
            co_return co_await handler(req);
        }
    }

    co_return not_found();
}

asio::awaitable<void> App::listen(int port) {
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

}  // namespace http
