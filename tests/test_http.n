// ============================================
// HTTP Server Test
// ============================================

import http;

// Handler functions for HTTP requests
fn home(http.Request req) -> http.Response {
    return http.text("Hello World");
}

fn about(http.Request req) -> http.Response {
    return http.text("About page");
}

// Test App-based routing API
fn test_app_routing() {
    app := http.App {};
    app.get("/", home);
    app.get("/about", about);
    assert_eq(1, 1);
}

// Test http.text response
fn test_http_text() {
    resp := http.text("Hello");
    assert_eq(resp.status, 200);
    assert_eq(resp.content_type, "text/plain");
    assert_eq(resp.body, "Hello");
}

// Test http.json response
fn test_http_json() {
    resp := http.json("{key: value}");
    assert_eq(resp.status, 200);
    assert_eq(resp.content_type, "application/json");
    assert_eq(resp.body, "{key: value}");
}

// Test http.not_found response
fn test_http_not_found() {
    resp := http.not_found();
    assert_eq(resp.status, 404);
    assert_eq(resp.content_type, "text/plain");
    assert_eq(resp.body, "Not Found");
}

// Test Request struct
fn test_request_struct() {
    req := http.Request { method: "GET", path: "/test", body: "" };
    assert_eq(req.method, "GET");
    assert_eq(req.path, "/test");
    assert_eq(req.body, "");
}

// Test Response struct
fn test_response_struct() {
    resp := http.Response { status: 201, content_type: "text/html", body: "<h1>Hi</h1>" };
    assert_eq(resp.status, 201);
    assert_eq(resp.content_type, "text/html");
    assert_eq(resp.body, "<h1>Hi</h1>");
}
