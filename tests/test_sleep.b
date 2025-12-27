// ============================================
// Sleep Tests
// ============================================

fn test_sleep_basic() {
    sleep(10);
}

fn test_sleep_zero() {
    sleep(0);
}

fn test_sleep_in_loop() {
    i := 0;

    while i < 3 {
        sleep(1);
        i = i + 1;
    }

    assert_eq(i, 3);
}
