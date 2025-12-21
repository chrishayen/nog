// ============================================
// Goroutine Tests - Stackful Coroutines
// ============================================

// ============================================
// Regular Functions (no async keyword needed)
// ============================================

fn returns_int() -> int {
    return 42;
}

fn add(int a, int b) -> int {
    return a + b;
}

fn with_call() -> int {
    result := returns_int();
    return result;
}

fn void_fn() {
    x := 1;
}

fn typed_call() -> int {
    int val = returns_int();
    return val;
}

fn multiple_calls() -> int {
    a := returns_int();
    b := add(a, 10);
    return b;
}

fn call_in_expr() -> int {
    result := returns_int() + 10;
    return result;
}

fn nested_calls() -> int {
    result := add(returns_int(), 5);
    return result;
}

// ============================================
// Methods (no async keyword needed)
// ============================================

Counter :: struct {
    value int
}

Counter :: get_value(self) -> int {
    return self.value;
}

Counter :: add_amount(self, int amount) -> int {
    current := self.get_value();
    return current + amount;
}

Counter :: increment(self) {
    self.value = self.value + 1;
}

// ============================================
// Function Tests
// ============================================

fn test_functions() {
    assert_eq(returns_int(), 42);
    assert_eq(add(2, 3), 5);
    assert_eq(with_call(), 42);
    assert_eq(typed_call(), 42);
    assert_eq(multiple_calls(), 52);
    assert_eq(call_in_expr(), 52);
    assert_eq(nested_calls(), 47);
}

fn test_methods() {
    c := Counter{ value: 41 };
    assert_eq(c.get_value(), 41);
    assert_eq(c.add_amount(1), 42);
    c.increment();
    assert_eq(c.value, 42);
}

// ============================================
// Channel Creation
// ============================================

fn test_channel_create() {
    ch := Channel<int>();
}

fn test_channel_types() {
    ch_int := Channel<int>();
    ch_str := Channel<str>();
    ch_bool := Channel<bool>();
}

// ============================================
// Channel Send/Recv with Goroutines
// ============================================

fn sender(Channel<int> ch, int val) {
    ch.send(val);
}

fn test_send_recv() {
    ch := Channel<int>();

    // Spawn a goroutine to send
    go sender(ch, 42);

    // Receive blocks until value is available
    val := ch.recv();
    assert_eq(val, 42);
}

// ============================================
// Select Statement
// ============================================

fn test_select_recv() {
    ch1 := Channel<int>();
    ch2 := Channel<int>();

    // Spawn a goroutine to send on ch1
    go sender(ch1, 41);

    chosen := 0;
    selected := 0;

    select {
        case val := ch1.recv() {
            chosen = 1;
            selected = val;
        }
        case val := ch2.recv() {
            chosen = 2;
            selected = val;
        }
    }

    assert_eq(chosen, 1);
    assert_eq(selected, 41);
}

// ============================================
// Sync test
// ============================================

fn test_sync_still_works() {
    x := 1;
    assert_eq(x, 1);
}
