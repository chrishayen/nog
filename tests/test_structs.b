Person :: struct { name str, age int }

Point :: struct { x int, y int }

Person :: get_name(self) -> str {
    return self.name;
}

Person :: set_age(self, int new_age) {
    self.age = new_age;
}

Person :: greet(self, str greeting) -> str {
    return greeting;
}

Person :: is_adult(self) -> bool {
    if self.age >= 18 {
        return true;
    }

    return false;
}

Person :: birth_year(self, int current_year) -> int {
    return current_year - self.age;
}

Point :: sum(self) -> int {
    return self.x + self.y;
}

fn test_struct_field_access() {
    p := Person { name: "Chris", age: 32 };
    assert_eq(p.name, "Chris");
    assert_eq(p.age, 32);
}

fn test_struct_multiple_instances() {
    a := Person { name: "Alice", age: 25 };
    b := Person { name: "Bob", age: 30 };
    assert_eq(a.name, "Alice");
    assert_eq(b.name, "Bob");
    assert_eq(a.age, 25);
    assert_eq(b.age, 30);
}

fn test_numeric_struct() {
    p := Point { x: 10, y: 20 };
    assert_eq(p.x, 10);
    assert_eq(p.y, 20);
    assert_eq(p.x + p.y, 30);
}

fn test_method_no_params() {
    p := Person { name: "Chris", age: 32 };
    assert_eq(p.get_name(), "Chris");
}

fn test_method_with_param() {
    p := Person { name: "Chris", age: 32 };
    assert_eq(p.greet("Hello"), "Hello");
}

fn test_method_mutation() {
    p := Person { name: "Chris", age: 32 };
    p.set_age(33);
    assert_eq(p.age, 33);
}

fn test_method_returns_bool_true() {
    adult := Person { name: "Chris", age: 32 };
    assert_eq(adult.is_adult(), true);
}

fn test_method_returns_bool_false() {
    child := Person { name: "Kid", age: 10 };
    assert_eq(child.is_adult(), false);
}

fn test_method_computation() {
    p := Person { name: "Chris", age: 32 };
    assert_eq(p.birth_year(2025), 1993);
}

fn test_different_struct_method() {
    pt := Point { x: 10, y: 20 };
    assert_eq(pt.sum(), 30);
}

fn test_multiple_method_calls() {
    p := Person { name: "Alice", age: 30 };
    assert_eq(p.get_name(), "Alice");
    assert_eq(p.is_adult(), true);
    assert_eq(p.birth_year(2025), 1995);
}

fn test_method_on_different_instances() {
    p1 := Person { name: "Bob", age: 25 };
    p2 := Person { name: "Eve", age: 17 };
    assert_eq(p1.is_adult(), true);
    assert_eq(p2.is_adult(), false);
}
