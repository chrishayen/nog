// Error test: or fail err in non-fallible function

fn fallible() -> int or err {
    fail "error";
}

fn not_fallible() -> int {
    x := fallible() or fail err;
    return x;
}

fn main() {
}
