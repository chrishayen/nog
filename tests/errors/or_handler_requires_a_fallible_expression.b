// Error test: or handler on non-fallible function

fn not_fallible() -> int {
    return 42;
}

fn main() {
    x := not_fallible() or return;
}
