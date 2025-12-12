# Channel Methods

## send

Sends a value through the channel.

```nog
s.send(T value)
```

**Parameters:**

- `value` (`T`): The value to send

**Example:**
```nog
await ch.send(42);
```

## recv

Receives a value from the channel.

```nog
s.recv() -> T
```

**Returns:** `T` - The received value

**Example:**
```nog
val := await ch.recv();
```

