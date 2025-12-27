// ============================================
// List Creation
// ============================================

fn test_list_create_empty_int() {
    nums := List<int>();
    assert_eq(nums.length(), 0);
}

fn test_list_create_empty_str() {
    names := List<str>();
    assert_eq(names.is_empty(), true);
}

// ============================================
// List Literals
// ============================================

fn test_list_literal_int() {
    nums := [1, 2, 3];
    assert_eq(nums.length(), 3);
}

fn test_list_literal_str() {
    names := ["a", "b", "c"];
    assert_eq(names.length(), 3);
}

fn test_list_literal_single() {
    nums := [42];
    assert_eq(nums.length(), 1);
}

fn test_list_literal_bool() {
    flags := [true, false, true];
    assert_eq(flags.length(), 3);
}

// ============================================
// List Access Methods
// ============================================

fn test_list_get() {
    nums := [10, 20, 30];
    assert_eq(nums.get(0), 10);
    assert_eq(nums.get(1), 20);
    assert_eq(nums.get(2), 30);
}

fn test_list_first() {
    nums := [10, 20, 30];
    assert_eq(nums.first(), 10);
}

fn test_list_last() {
    nums := [10, 20, 30];
    assert_eq(nums.last(), 30);
}

// ============================================
// List Query Methods
// ============================================

fn test_list_length() {
    nums := [1, 2, 3, 4, 5];
    assert_eq(nums.length(), 5);
}

fn test_list_is_empty_false() {
    nums := [1];
    assert_eq(nums.is_empty(), false);
}

fn test_list_is_empty_true() {
    nums := List<int>();
    assert_eq(nums.is_empty(), true);
}

fn test_list_contains_true() {
    nums := [1, 2, 3];
    assert_eq(nums.contains(2), true);
}

fn test_list_contains_false() {
    nums := [1, 2, 3];
    assert_eq(nums.contains(5), false);
}

fn test_list_contains_str() {
    names := ["alice", "bob"];
    assert_eq(names.contains("bob"), true);
    assert_eq(names.contains("charlie"), false);
}

// ============================================
// List Modification Methods
// ============================================

fn test_list_append() {
    nums := [1, 2];
    nums.append(3);
    assert_eq(nums.length(), 3);
    assert_eq(nums.get(2), 3);
}

fn test_list_pop() {
    nums := [1, 2, 3];
    nums.pop();
    assert_eq(nums.length(), 2);
}

fn test_list_set() {
    nums := [1, 2, 3];
    nums.set(1, 99);
    assert_eq(nums.get(1), 99);
}

fn test_list_clear() {
    nums := [1, 2, 3];
    nums.clear();
    assert_eq(nums.length(), 0);
}

fn test_list_insert() {
    nums := [1, 3];
    nums.insert(1, 2);
    assert_eq(nums.get(1), 2);
    assert_eq(nums.length(), 3);
}

fn test_list_remove() {
    nums := [1, 2, 3];
    nums.remove(1);
    assert_eq(nums.length(), 2);
    assert_eq(nums.get(1), 3);
}

// ============================================
// List with Structs
// ============================================

Point :: struct {
    x int,
    y int
}

fn test_list_of_structs() {
    points := List<Point>();
    p := Point { x: 1, y: 2 };
    points.append(p);
    assert_eq(points.length(), 1);
}

// ============================================
// Typed Variable Declaration
// ============================================

fn test_list_typed_decl() {
    List<int> nums = [1, 2, 3];
    assert_eq(nums.length(), 3);
}

fn test_list_typed_empty() {
    List<str> names = List<str>();
    assert_eq(names.is_empty(), true);
}

// ============================================
// Chained Operations
// ============================================

fn test_list_chained_append() {
    nums := List<int>();
    nums.append(1);
    nums.append(2);
    nums.append(3);
    assert_eq(nums.length(), 3);
    assert_eq(nums.first(), 1);
    assert_eq(nums.last(), 3);
}
