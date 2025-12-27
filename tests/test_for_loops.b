// ============================================
// Range-based For Loops
// ============================================

fn test_for_range_basic() {
    sum := 0;

    for i in 0..5 {
        sum = sum + i;
    }

    assert_eq(sum, 10);
}

fn test_for_range_start_nonzero() {
    sum := 0;

    for i in 3..6 {
        sum = sum + i;
    }

    assert_eq(sum, 12);
}

fn test_for_range_empty() {
    count := 0;

    for i in 5..5 {
        count = count + 1;
    }

    assert_eq(count, 0);
}

fn test_for_range_with_variable_bounds() {
    n := 4;
    sum := 0;

    for i in 0..n {
        sum = sum + i;
    }

    assert_eq(sum, 6);
}

// ============================================
// Foreach Loops
// ============================================

fn test_foreach_int_list() {
    nums := [10, 20, 30];
    sum := 0;

    for n in nums {
        sum = sum + n;
    }

    assert_eq(sum, 60);
}

fn test_foreach_str_list() {
    names := ["a", "b", "c"];
    count := 0;

    for name in names {
        count = count + 1;
    }

    assert_eq(count, 3);
}

fn test_foreach_empty_list() {
    nums := List<int>();
    count := 0;

    for n in nums {
        count = count + 1;
    }

    assert_eq(count, 0);
}

fn test_foreach_with_condition() {
    nums := [1, 2, 3, 4, 5];
    found := false;

    for n in nums {
        if n == 3 {
            found = true;
        }
    }

    assert_eq(found, true);
}

// ============================================
// Nested For Loops
// ============================================

fn test_nested_for_range() {
    sum := 0;

    for i in 0..3 {
        for j in 0..3 {
            sum = sum + 1;
        }
    }

    assert_eq(sum, 9);
}

fn test_for_in_while() {
    total := 0;
    outer := 0;

    while outer < 2 {
        for i in 0..3 {
            total = total + 1;
        }

        outer = outer + 1;
    }

    assert_eq(total, 6);
}

// ============================================
// Struct List Foreach
// ============================================

Point :: struct {
    x int,
    y int
}

fn test_foreach_struct_list() {
    points := List<Point>();
    points.append(Point { x: 1, y: 2 });
    points.append(Point { x: 3, y: 4 });

    sum := 0;

    for p in points {
        sum = sum + p.x + p.y;
    }

    assert_eq(sum, 10);
}
