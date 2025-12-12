# str Methods

## length

Returns the number of characters in the string.

```nog
s.length() -> int
```

**Returns:** `int` - The string length

**Example:**
```nog
s := "hello";
len := s.length();  // 5
```

## empty

Returns true if the string has no characters.

```nog
s.empty() -> bool
```

**Returns:** `bool` - True if empty, false otherwise

**Example:**
```nog
s := "";
if s.empty() {
    print("String is empty");
}
```

## contains

Checks if the string contains the given substring.

```nog
s.contains(str substr) -> bool
```

**Parameters:**

- `substr` (`str`): The substring to search for

**Returns:** `bool` - True if found, false otherwise

**Example:**
```nog
s := "hello world";
if s.contains("world") {
    print("Found it!");
}
```

## starts_with

Checks if the string starts with the given prefix.

```nog
s.starts_with(str prefix) -> bool
```

**Parameters:**

- `prefix` (`str`): The prefix to check

**Returns:** `bool` - True if string starts with prefix

**Example:**
```nog
path := "/api/users";
if path.starts_with("/api") {
    print("API route");
}
```

## ends_with

Checks if the string ends with the given suffix.

```nog
s.ends_with(str suffix) -> bool
```

**Parameters:**

- `suffix` (`str`): The suffix to check

**Returns:** `bool` - True if string ends with suffix

**Example:**
```nog
file := "image.png";
if file.ends_with(".png") {
    print("PNG image");
}
```

## find

Returns the index of the first occurrence of a substring, or -1 if not found.

```nog
s.find(str substr) -> int
```

**Parameters:**

- `substr` (`str`): The substring to find

**Returns:** `int` - Index of first occurrence, or -1

**Example:**
```nog
s := "hello world";
idx := s.find("world");  // 6
```

## substr

Extracts a portion of the string.

```nog
s.substr(int start, int length) -> str
```

**Parameters:**

- `start` (`int`): Starting index (0-based)
- `length` (`int`): Number of characters to extract

**Returns:** `str` - The extracted substring

**Example:**
```nog
s := "hello world";
sub := s.substr(0, 5);  // "hello"
```

## at

Returns the character at the specified index.

```nog
s.at(int index) -> char
```

**Parameters:**

- `index` (`int`): The index (0-based)

**Returns:** `char` - The character at that position

**Example:**
```nog
s := "hello";
c := s.at(0);  // 'h'
```

