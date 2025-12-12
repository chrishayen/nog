/**
 * @file http.cpp
 * @brief Built-in HTTP module implementation.
 *
 * Creates the AST definitions for the http module and generates
 * the C++ runtime code for HTTP serving.
 */

#include "http.hpp"

using namespace std;

namespace nog::stdlib {

/**
 * Checks if a module name is a built-in stdlib module.
 */
bool is_builtin_module(const string& name) {
    return name == "http";
}

/**
 * Creates the AST for the built-in http module.
 */
unique_ptr<Program> create_http_module() {
    auto program = make_unique<Program>();

    // Request :: struct { method str, path str, body str }
    auto request = make_unique<StructDef>();
    request->name = "Request";
    request->visibility = Visibility::Public;
    request->fields.push_back({"method", "str", ""});
    request->fields.push_back({"path", "str", ""});
    request->fields.push_back({"body", "str", ""});
    program->structs.push_back(move(request));

    // Response :: struct { status int, content_type str, body str }
    auto response = make_unique<StructDef>();
    response->name = "Response";
    response->visibility = Visibility::Public;
    response->fields.push_back({"status", "int", ""});
    response->fields.push_back({"content_type", "str", ""});
    response->fields.push_back({"body", "str", ""});
    program->structs.push_back(move(response));

    // fn text(str content) -> http.Response
    auto text_fn = make_unique<FunctionDef>();
    text_fn->name = "text";
    text_fn->visibility = Visibility::Public;
    text_fn->params.push_back({"str", "content"});
    text_fn->return_type = "http.Response";
    program->functions.push_back(move(text_fn));

    // fn json(str content) -> http.Response
    auto json_fn = make_unique<FunctionDef>();
    json_fn->name = "json";
    json_fn->visibility = Visibility::Public;
    json_fn->params.push_back({"str", "content"});
    json_fn->return_type = "http.Response";
    program->functions.push_back(move(json_fn));

    // fn not_found() -> http.Response
    auto not_found_fn = make_unique<FunctionDef>();
    not_found_fn->name = "not_found";
    not_found_fn->visibility = Visibility::Public;
    not_found_fn->return_type = "http.Response";
    program->functions.push_back(move(not_found_fn));

    // async fn serve(int port, fn(http.Request) -> http.Response handler)
    auto serve_fn = make_unique<FunctionDef>();
    serve_fn->name = "serve";
    serve_fn->visibility = Visibility::Public;
    serve_fn->is_async = true;
    serve_fn->params.push_back({"int", "port"});
    serve_fn->params.push_back({"fn(http.Request) -> http.Response", "handler"});
    program->functions.push_back(move(serve_fn));

    // App :: struct { } (internal routing state managed in C++)
    auto app_struct = make_unique<StructDef>();
    app_struct->name = "App";
    app_struct->visibility = Visibility::Public;
    program->structs.push_back(move(app_struct));

    // App :: get(self, str path, fn(http.Request) -> http.Response handler)
    auto get_method = make_unique<MethodDef>();
    get_method->struct_name = "App";
    get_method->name = "get";
    get_method->visibility = Visibility::Public;
    get_method->params.push_back({"http.App", "self"});
    get_method->params.push_back({"str", "path"});
    get_method->params.push_back({"fn(http.Request) -> http.Response", "handler"});
    program->methods.push_back(move(get_method));

    // App :: post(self, str path, fn(http.Request) -> http.Response handler)
    auto post_method = make_unique<MethodDef>();
    post_method->struct_name = "App";
    post_method->name = "post";
    post_method->visibility = Visibility::Public;
    post_method->params.push_back({"http.App", "self"});
    post_method->params.push_back({"str", "path"});
    post_method->params.push_back({"fn(http.Request) -> http.Response", "handler"});
    program->methods.push_back(move(post_method));

    // App :: listen(self, int port) async
    auto listen_method = make_unique<MethodDef>();
    listen_method->struct_name = "App";
    listen_method->name = "listen";
    listen_method->visibility = Visibility::Public;
    listen_method->is_async = true;
    listen_method->params.push_back({"http.App", "self"});
    listen_method->params.push_back({"int", "port"});
    program->methods.push_back(move(listen_method));

    return program;
}

/**
 * Generates the C++ runtime code for the http module.
 */
string generate_http_runtime() {
    return R"(
namespace http {

struct Request {
    std::string method;
    std::string path;
    std::string body;
};

struct Response {
    int status;
    std::string content_type;
    std::string body;
};

inline Response text(const std::string& content) {
    return Response{200, "text/plain", content};
}

inline Response json(const std::string& content) {
    return Response{200, "application/json", content};
}

inline Response not_found() {
    return Response{404, "text/plain", "Not Found"};
}

// HTTP parser context for llhttp
struct HttpParserContext {
    std::string method;
    std::string url;
    std::string body;
    std::string current_header_field;
    std::string current_header_value;
    bool message_complete = false;
};

// llhttp callbacks
static int on_method(llhttp_t* parser, const char* at, size_t len) {
    auto* ctx = static_cast<HttpParserContext*>(parser->data);
    ctx->method.assign(at, len);
    return 0;
}

static int on_url(llhttp_t* parser, const char* at, size_t len) {
    auto* ctx = static_cast<HttpParserContext*>(parser->data);
    ctx->url.assign(at, len);
    return 0;
}

static int on_body(llhttp_t* parser, const char* at, size_t len) {
    auto* ctx = static_cast<HttpParserContext*>(parser->data);
    ctx->body.append(at, len);
    return 0;
}

static int on_message_complete(llhttp_t* parser) {
    auto* ctx = static_cast<HttpParserContext*>(parser->data);
    ctx->message_complete = true;
    return 0;
}

// Format HTTP response
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

// Async request reader - reads until complete HTTP message is received
inline asio::awaitable<Request> read_request(asio::ip::tcp::socket& socket) {
    llhttp_t parser;
    llhttp_settings_t settings;
    llhttp_settings_init(&settings);
    settings.on_method = on_method;
    settings.on_url = on_url;
    settings.on_body = on_body;
    settings.on_message_complete = on_message_complete;

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

// Handle a single connection (async handler)
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

// Main serve function (simple single-handler version)
// Handler must be an async function returning asio::awaitable<Response>
template<typename Handler>
asio::awaitable<void> serve(int port, Handler handler) {
    auto executor = co_await asio::this_coro::executor;
    asio::ip::tcp::acceptor acceptor(executor, {asio::ip::tcp::v4(), static_cast<asio::ip::port_type>(port)});

    std::cout << "HTTP server listening on port " << port << std::endl;

    while (true) {
        asio::ip::tcp::socket socket = co_await acceptor.async_accept(asio::use_awaitable);
        asio::co_spawn(executor, handle_connection_async(std::move(socket), handler), asio::detached);
    }
}

// App struct for routing-based HTTP server
struct App {
    std::vector<std::tuple<std::string, std::string, std::function<asio::awaitable<Response>(Request)>>> routes;

    void get(const std::string& path, std::function<asio::awaitable<Response>(Request)> handler) {
        routes.push_back({"GET", path, handler});
    }

    void post(const std::string& path, std::function<asio::awaitable<Response>(Request)> handler) {
        routes.push_back({"POST", path, handler});
    }

    asio::awaitable<Response> route(const Request& req) {
        for (const auto& [method, path, handler] : routes) {
            if (req.method == method && req.path == path) {
                co_return co_await handler(req);
            }
        }
        co_return not_found();
    }

    asio::awaitable<void> listen(int port) {
        auto executor = co_await asio::this_coro::executor;
        asio::ip::tcp::acceptor acceptor(executor, {asio::ip::tcp::v4(), static_cast<asio::ip::port_type>(port)});

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
)";
}

}  // namespace nog::stdlib
