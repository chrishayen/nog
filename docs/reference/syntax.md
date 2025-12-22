# Nog Language Reference

## Types

### int

Integer type for whole numbers.

**Syntax:**
```
int
```

**Example:**
```nog
int x = 42;
```

### str

String type for text.

**Syntax:**
```
str
```

**Example:**
```nog
str name = "Chris";
```

### bool

Boolean type with values true or false.

**Syntax:**
```
bool
```

**Example:**
```nog
bool flag = true;
```

### char

Single character type.

**Syntax:**
```
char
```

**Example:**
```nog
char c = 'a';
```

### f32

32-bit floating point number.

**Syntax:**
```
f32
```

**Example:**
```nog
f32 pi = 3.14;
```

### f64

64-bit floating point number.

**Syntax:**
```
f64
```

**Example:**
```nog
f64 precise = 3.14159265359;
```

### u32

Unsigned 32-bit integer.

**Syntax:**
```
u32
```

**Example:**
```nog
u32 count = 100;
```

### u64

Unsigned 64-bit integer.

**Syntax:**
```
u64
```

**Example:**
```nog
u64 big_num = 9999999999;
```

### Optional Types

Optional type that can hold a value or none.

**Syntax:**
```
T?
```

**Example:**
```nog
int? maybe_num = none;
int? value = 42;
if value is none {
    print("No value");
}
```

> Use `is none` to check for none, or use a truthy check

## Variables

### Explicit Declaration

Declare a variable with an explicit type.

**Syntax:**
```
type name = expr;
```

**Example:**
```nog
int x = 42;
str name = "Chris";
bool flag = true;
```

### Type Inference

Declare a variable with inferred type using :=.

**Syntax:**
```
name := expr;
```

**Example:**
```nog
x := 100;
name := "Hello";
pi := 3.14;
```

## Functions

### Function Declaration

Declare a function with parameters and return type.

**Syntax:**
```
fn name(type param, ...) -> return_type { }
```

**Example:**
```nog
fn add(int a, int b) -> int {
    return a + b;
}
```

### Void Function

Function with no return type (void).

**Syntax:**
```
fn name(params) { }
```

**Example:**
```nog
fn greet(str name) {
    print("Hello, " + name);
}
```

### Function References

Pass functions as arguments to other functions.

**Syntax:**
```
fn(param_types) -> return_type
```

**Example:**
```nog
fn apply_op(int x, int y, fn(int, int) -> int op) -> int {
    return op(x, y);
}
result := apply_op(3, 4, add);
```

## Structs

### Struct Definition

Define a custom data structure.

**Syntax:**
```
Name :: struct { field type, ... }
```

**Example:**
```nog
Person :: struct {
    name str,
    age int
}
```

### Struct Instantiation

Create an instance of a struct.

**Syntax:**
```
TypeName { field: value, field: value }
```

**Example:**
```nog
p := Person { name: "Chris", age: 32 };
req := http.Request { method: "GET", path: "/", body: "" };
```

### Field Access

Access or modify struct fields using dot notation.

**Syntax:**
```
instance.field
```

**Example:**
```nog
name := p.name;
p.age = 33;
```

## Methods

### Method Definition

Define a method on a struct type.

**Syntax:**
```
Type :: name(self, params) -> return_type { }
```

**Example:**
```nog
Person :: get_name(self) -> str {
    return self.name;
}

Person :: greet(self, str greeting) -> str {
    return greeting + ", " + self.name;
}
```

### Method Call

Call a method on a struct instance.

**Syntax:**
```
instance.method(args)
```

**Example:**
```nog
name := p.get_name();
msg := p.greet("Hello");
```

## Control Flow

### if

Conditional branching with if and optional else.

**Syntax:**
```
if condition { ... } else { ... }
```

**Example:**
```nog
if x > 10 {
    print("big");
} else {
    print("small");
}
```

### while

Loop while a condition is true.

**Syntax:**
```
while condition { ... }
```

**Example:**
```nog
i := 0;
while i < 5 {
    print(i);
    i = i + 1;
}
```

## Operators

### Arithmetic Operators

Mathematical operations.

**Syntax:**
```
+ - * /
```

**Example:**
```nog
sum := a + b;
diff := a - b;
prod := a * b;
quot := a / b;
```

> Precedence: `*` and `/` bind tighter than `+` and `-` (left-associative within each level).

### Parentheses

Group expressions to override precedence and enable postfix access on expression results.

**Syntax:**
```
(expr)
```

**Example:**
```nog
result := (1 + 2) * 3;
len := ("hi" + "there").length();
```

### Comparison Operators

Compare values.

**Syntax:**
```
== != < > <= >=
```

**Example:**
```nog
if x == y { }
if x != y { }
if x < y { }
if x >= y { }
```

### String Concatenation

Join strings with the + operator.

**Syntax:**
```
str + str
```

**Example:**
```nog
msg := "Hello, " + name + "!";
```

## Imports

### import

Import a module to use its types and functions.

**Syntax:**
```
import module_name;
```

**Example:**
```nog
import http;
import myproject.utils;
```

> Use dot notation for nested modules

### Qualified Access

Access imported module members with dot notation.

**Syntax:**
```
module.member
```

**Example:**
```nog
resp := http.text("Hello");
result := utils.helper();
```

## Visibility

### @private

Mark a function or struct as private to its module.

**Syntax:**
```
@private fn/struct
```

> Private members are not exported when the module is imported

### Doc Comments

Document functions, structs, and fields with /// comments.

**Syntax:**
```
/// description
```

**Example:**
```nog
/// This is a doc comment for the function
fn add(int a, int b) -> int {
    return a + b;
}

/// Doc comment for struct
Person :: struct {
    /// Doc comment for field
    name str
}
```
