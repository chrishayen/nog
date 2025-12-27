fn test_mul_precedence_over_add() {
    assert_eq(1 + 2 * 3, 7);
}

fn test_div_precedence_over_add() {
    assert_eq(8 / 4 + 1, 3);
}

fn test_chain_precedence() {
    assert_eq(1 + 2 * 3 + 4, 11);
}

fn test_left_associativity_sub() {
    assert_eq(10 - 3 - 2, 5);
}

fn test_left_associativity_div() {
    assert_eq(20 / 5 / 2, 2);
}

