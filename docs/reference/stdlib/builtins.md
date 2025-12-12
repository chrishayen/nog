# Built-in Functions

## Functions

### print

Prints values to standard output, followed by a newline.

```nog
fn print(... args)
```

**Parameters:**

- `args` (`...`): One or more values to print (separated by spaces)

**Example:**
```nog
print("Hello, World!");
print("x =", x, "y =", y);
```

### assert_eq

Asserts that two values are equal. Only available in test mode.

```nog
fn assert_eq(T expected, T actual)
```

**Parameters:**

- `expected` (`T`): The expected value
- `actual` (`T`): The actual value to compare

**Example:**
```nog
assert_eq(5, add(2, 3));
assert_eq("hello", greet());
```

