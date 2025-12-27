Person :: struct { name str, age int }

Person :: get_name(self) -> str {
    return self.name;
}

Person :: set_age(self, int new_age) {
    self.age = new_age;
}

Person :: greet(self, str greeting) -> str {
    return greeting;
}

fn read_name(Person *p) -> str {
    return p.name;
}

fn read_age(Person *p) -> int {
    return p.age;
}

fn set_age_to(Person *p, int new_age) {
    p.age = new_age;
}

fn call_method(Person *p) -> str {
    return p.get_name();
}

fn call_method_with_param(Person *p, str greeting) -> str {
    return p.greet(greeting);
}

fn call_mutating_method(Person *p, int new_age) {
    p.set_age(new_age);
}

fn test_pass_by_reference_read_field() {
    bob := Person { name: "Bob", age: 30 };
    assert_eq(read_name(&bob), "Bob");
    assert_eq(read_age(&bob), 30);
}

fn test_pass_by_reference_modify_field() {
    person := Person { name: "Alice", age: 25 };
    set_age_to(&person, 26);
    assert_eq(person.age, 26);
}

fn test_pass_by_reference_method_call() {
    chris := Person { name: "Chris", age: 32 };
    assert_eq(call_method(&chris), "Chris");
}

fn test_pass_by_reference_method_with_param() {
    dave := Person { name: "Dave", age: 40 };
    assert_eq(call_method_with_param(&dave, "Hello"), "Hello");
}

fn test_pass_by_reference_mutating_method() {
    frank := Person { name: "Frank", age: 35 };
    call_mutating_method(&frank, 36);
    assert_eq(frank.age, 36);
}

fn test_multiple_mutations() {
    person := Person { name: "Grace", age: 28 };
    set_age_to(&person, 29);
    assert_eq(person.age, 29);
    set_age_to(&person, 30);
    assert_eq(person.age, 30);
}
