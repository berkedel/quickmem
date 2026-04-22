# quickmem

An Android CLI memory tool powered by QuickJS. It exposes a frida-gum-like JavaScript API for reading and writing remote process memory, backed by Linux `process_vm_readv` / `process_vm_writev`.

## Overview

quickmem lets you attach to a running Android process by PID and script memory operations in JavaScript. It bundles the QuickJS-ng v0.14.0 engine and provides a `NativePointer`-style API for inspecting and modifying target memory without writing native code.

## Features

- Evaluate JavaScript inline, from a file, or via stdin
- Interactive REPL mode with `--repl`
- `ptr(value)` to create native pointers from numbers or hex strings
- Full set of typed read/write operations (8/16/32/64-bit, float, double)
- Byte array read/write with `ArrayBuffer` / `Uint8Array` support
- UTF-8 and C-string read/write helpers
- Pointer arithmetic: `add`, `sub`, `xor`, `shr`, `shl`
- Global `hexdump(target, options)` for dumping `NativePointer` or `ArrayBuffer` memory
- Global `console.log(...args)` for debugging scripts
- `TypeError` exceptions with descriptive errno translations (EPERM, ESRCH, EFAULT, EINVAL)

## Build Instructions

Build the native executable with Gradle:

```bash
./gradlew :app:assembleDebug
```

The binary is produced at a path like:

```
app/build/intermediates/cmake/debug/obj/arm64-v8a/quickmem
```

Push it to your device and run:

```bash
adb push app/build/intermediates/cmake/debug/obj/arm64-v8a/quickmem /data/local/tmp/
adb shell chmod +x /data/local/tmp/quickmem
adb shell /data/local/tmp/quickmem <pid> -e "console.log('hello world')"
```

## Usage

```
quickmem <pid> [-e "<js>"] [--repl] [script.js]
```

Arguments:

- `<pid>`: target process ID (required)
- `-e "<js>"`: execute an inline JavaScript expression
- `--repl`: start interactive REPL with line editing, history (up/down arrows), and cursor movement (left/right arrows). Type `.exit` to quit
- `script.js`: path to a JavaScript file to execute
- (none): read JavaScript from stdin until EOF

### Examples

**Inline evaluation**

```bash
quickmem 1234 -e "console.log('hello world')"
```

**Execute a script file**

```bash
quickmem 1234 myscript.js
```

**Read from stdin**

```bash
quickmem 1234 < myscript.js
```

**Interactive REPL**

```bash
quickmem 1234 --repl
quickmem> 1+2
3
quickmem> ptr('0x1000').toString()
0x1000
quickmem> [press up arrow]  # recalls previous command
quickmem> 1+2
3
quickmem> .exit
```

## JS API Reference

### `ptr(value)`

Creates a `NativePointer` from a number or a hex string.

- `value` (number | string): a numeric address or a hex string such as `"0x7ffd0000"`
- Returns: `NativePointer`
- Throws `TypeError` if the argument is not a number or a hex string, or if `BigInt` is passed

```js
const p = ptr(0x7ffd0000);
const q = ptr("0x7ffd0000");
```

### `NativePointer`

#### Methods

| Method | Description |
|--------|-------------|
| `toString()` | Returns a hex string like `"0x7ffd0000"` |
| `add(val)` | Returns a new `NativePointer` with `address + val` |
| `sub(val)` | Returns a new `NativePointer` with `address - val` |
| `xor(val)` | Returns a new `NativePointer` with `address ^ val` |
| `shr(val)` | Returns a new `NativePointer` with `address >> val` |
| `shl(val)` | Returns a new `NativePointer` with `address << val` |

#### Numeric reads

All read methods throw `TypeError` on failure.

| Method | Return type |
|--------|-------------|
| `readS8()` | signed 8-bit integer |
| `readU8()` | unsigned 8-bit integer |
| `readS16()` | signed 16-bit integer |
| `readU16()` | unsigned 16-bit integer |
| `readS32()` | signed 32-bit integer |
| `readU32()` | unsigned 32-bit integer |
| `readS64()` | signed 64-bit integer |
| `readU64()` | unsigned 64-bit integer |
| `readFloat()` | 32-bit float |
| `readDouble()` | 64-bit float |
| `readPointer()` | `NativePointer` (pointer-sized read) |

#### Numeric writes

All write methods throw `TypeError` on failure. They return `undefined` on success.

| Method | Argument |
|--------|----------|
| `writeS8(val)` | signed 8-bit integer |
| `writeU8(val)` | unsigned 8-bit integer |
| `writeS16(val)` | signed 16-bit integer |
| `writeU16(val)` | unsigned 16-bit integer |
| `writeS32(val)` | signed 32-bit integer |
| `writeU32(val)` | unsigned 32-bit integer |
| `writeS64(val)` | signed 64-bit integer |
| `writeU64(val)` | unsigned 64-bit integer |
| `writeFloat(val)` | 32-bit float |
| `writeDouble(val)` | 64-bit float |

#### Byte arrays

- `readByteArray(length)` — reads `length` bytes and returns an `ArrayBuffer`. Throws if `length <= 0` or if it exceeds the 1 MB cap.
- `writeByteArray(bytes)` — accepts an `ArrayBuffer` or `Uint8Array` and writes it to memory. Throws if the payload exceeds the 1 MB cap.

#### Strings

- `readUtf8String(size?)` — reads up to `size` bytes (default 2048) and returns a UTF-8 string. Throws `TypeError` if no null terminator is found within the scanned range.
- `writeUtf8String(string)` — writes a string plus a null terminator. Capped at 2048 bytes.
- `readCString(size?)` — reads up to `size` bytes (default 2048) and returns a C string. Throws `TypeError` if no null terminator is found.

### `hexdump(target, options?)`

Returns a formatted hexdump string of a `NativePointer` or `ArrayBuffer`.

- `target`: `NativePointer` or `ArrayBuffer` to dump
- `options` (optional object):
  - `address`: `NativePointer` or number for display base address (default: target's address or `0`)
  - `offset`: number, byte offset to start from (default: `0`)
  - `length`: number, how many bytes to dump (default: `256` for NativePointer, all bytes for ArrayBuffer)
  - `header`: boolean, whether to include the header row (default: `true`)
- Returns: `string`

Example output:

```
Address           Hex                                          ASCII
00000000  48 65 6c 6c 6f 20 57 6f  72 6c 64 21 0a 00 01 02  Hello World!....
```

Example usage:

```js
// Dump memory at a NativePointer
const p = ptr(0x7ffd0000);
console.log(hexdump(p, { length: 32 }));

// Dump an ArrayBuffer
const buf = new Uint8Array([0x48, 0x65, 0x6c, 0x6c, 0x6f]).buffer;
console.log(hexdump(buf));
```

### `Memory`

Global object exposing memory-related utilities.

- `Memory.alloc(size)` — allocates `size` bytes in the quickmem process and returns a `NativePointer`. The memory is zero-initialized. Maximum allocation is 1MB.
- `Memory.scanSync(address, size, pattern)` — scans a memory range for a byte pattern and returns an array of matches. The pattern is a hex string with optional wildcards (`??`).
  - `address`: `NativePointer` or number — start address
  - `size`: number — bytes to scan
  - `pattern`: string — hex pattern like `"7f 45 4c 46"` or `"00 ?? 13 37 ?? 42"`
  - Returns: `[{ address: NativePointer, size: number }, ...]`

```js
const m = Process.findModuleByName('libc.so');
const matches = Memory.scanSync(m.base, m.size, '7f 45 4c 46');
matches.forEach(match => {
    console.log('Found at:', match.address.toString());
});
```

### `Process`

Global object for process introspection.

- `Process.findModuleByName(name)` — searches `/proc/<pid>/maps` for a shared library matching `name`. Returns an object with:
  - `base`: `NativePointer` to the module base address
  - `size`: total mapped size in bytes
  - `name`: the queried name
  - `path`: full filesystem path to the library
  
  Returns `null` if the module is not found.

```js
const m = Process.findModuleByName('libc.so');
if (m) {
    console.log('Base:', m.base.toString());
    console.log('Size:', m.size);
    console.log('Path:', m.path);
}
```

### `console`

- `console.log(...args)` — prints arguments to stdout, separated by spaces, followed by a newline. Non-string values are coerced to strings.

### Error behavior

All API failures throw `TypeError` with a descriptive message. Messages include translated `errno` values when a memory operation fails:

- `EPERM`: insufficient privileges to access process memory
- `ESRCH`: target process does not exist or is a zombie
- `EFAULT`: invalid memory address
- `EINVAL`: invalid argument or alignment issue

Invalid PID arguments at the CLI level are rejected before any JS evaluation begins.

## Extending

To add a new `NativePointer` method:

1. **Implement the C++ function** in `app/src/main/cpp/quickmem.cpp`:

```cpp
static JSValue js_my_method(JSContext* ctx, JSValue this_val, int argc, JSValue* argv) {
    NativePointerData* data = static_cast<NativePointerData*>(
        JS_GetOpaque(this_val, js_native_pointer_class_id));
    if (!data) {
        return JS_ThrowTypeError(ctx, "Not a NativePointer");
    }

    // Validate arguments, perform work, and return a value
    // Use JS_ThrowTypeError(ctx, "...") for errors
    return JS_UNDEFINED;
}
```

2. **Register it on the prototype** inside `quickjs_init()`:

```cpp
JS_SetPropertyStr(g_ctx, np_proto, "myMethod",
    JS_NewCFunction(g_ctx, js_my_method, "myMethod", 1));
```

3. **Follow existing patterns**:
   - Extract opaque data with `JS_GetOpaque`
   - Validate argument count and types
   - Return a new object or primitive, or throw on error
   - Do not free values after attaching them with `JS_SetPropertyStr` (QuickJS-ng v0.14.0 does not increment refcount on property set)

To add a new global function or object, use `JS_SetPropertyStr(g_ctx, global_obj, "myGlobal", ...)`.

## License

MIT
