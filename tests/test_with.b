// Test with statement for resource management

// Simple resource with close method
Resource :: struct {
    name str,
    value int,
    closed bool
}

Resource :: close(self) {
    self.closed = true;
}

Resource :: get_value(self) -> int {
    return self.value;
}

fn create_resource(str name, int val) -> Resource {
    return Resource { name: name, value: val, closed: false };
}

fn test_basic_with() {
    with create_resource("test", 42) as r {
        assert_eq(r.get_value(), 42);
        assert_eq(r.closed, false);
    }
}

fn test_with_nested() {
    with create_resource("outer", 1) as outer {
        with create_resource("inner", 2) as inner {
            assert_eq(outer.get_value(), 1);
            assert_eq(inner.get_value(), 2);
        }
    }
}

fn test_with_field_access() {
    with create_resource("named", 100) as res {
        assert_eq(res.name, "named");
        assert_eq(res.value, 100);
    }
}

fn test_with_method_call() {
    with create_resource("method", 50) as res {
        v := res.get_value();
        assert_eq(v, 50);
    }
}
