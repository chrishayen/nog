fn test_optional_int_none_check() {
    int? x = none;
    int result = 0;

    if x is none {
        result = 1;
    }

    assert_eq(result, 1);
}

fn test_optional_int_value_check() {
    int? x = 42;
    int result = 0;

    if x {
        result = 1;
    }

    assert_eq(result, 1);
}

fn test_optional_int_not_none() {
    int? x = 42;
    int result = 0;

    if x is none {
        result = 1;
    }

    assert_eq(result, 0);
}

fn test_optional_str_none_check() {
    str? name = none;
    int result = 0;

    if name is none {
        result = 1;
    }

    assert_eq(result, 1);
}

fn test_optional_str_value_check() {
    str? name = "Chris";
    int result = 0;

    if name {
        result = 1;
    }

    assert_eq(result, 1);
}

fn test_optional_bool_none_check() {
    bool? flag = none;
    int result = 0;

    if flag is none {
        result = 1;
    }

    assert_eq(result, 1);
}

fn test_optional_bool_value_check() {
    bool? flag = true;
    int result = 0;

    if flag {
        result = 1;
    }

    assert_eq(result, 1);
}

Person :: struct { name str, age int }

fn test_optional_struct_none_check() {
    Person? p = none;
    int result = 0;

    if p is none {
        result = 1;
    }

    assert_eq(result, 1);
}

fn test_optional_struct_value_check() {
    Person? p = Person { name: "Chris", age: 32 };
    int result = 0;

    if p {
        result = 1;
    }

    assert_eq(result, 1);
}
