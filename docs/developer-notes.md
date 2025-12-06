# Developer Notes

This document captures design decisions, trade-offs, and historical context for CRUMBS internals. It's intended for contributors and future maintainers.

---

## Handler Dispatch: O(1) vs O(n) Trade-off

### Original Design (v0.8.x): O(1) Direct Indexing

The initial handler dispatch used a 256-entry lookup table for O(1) constant-time dispatch:

```c
struct crumbs_context_s {
    // ...
    crumbs_handler_fn handlers[256];     // 512 bytes on AVR
    void *handler_userdata[256];         // 512 bytes on AVR
};

// O(1) dispatch
crumbs_handler_fn fn = ctx->handlers[command_type];
if (fn) fn(ctx, cmd, data, len, ctx->handler_userdata[cmd]);
```

**Memory cost:** ~1KB on AVR (2-byte pointers), ~2KB on 32-bit platforms.

**Problem discovered:** The LED peripheral example compiled with **70% RAM usage** on Arduino Nano (ATmega328P), leaving only 613 bytes for stack, buffers, and user state. This was unacceptable for a simple LED controller.

### Current Design (v0.9.x): O(n) Sparse Table

Switched to a configurable sparse table with linear search:

```c
struct crumbs_context_s {
    // ...
    uint8_t handler_count;
    uint8_t handler_cmd[CRUMBS_MAX_HANDLERS];
    crumbs_handler_fn handlers[CRUMBS_MAX_HANDLERS];
    void *handler_userdata[CRUMBS_MAX_HANDLERS];
};

// O(n) dispatch
for (uint8_t i = 0; i < ctx->handler_count; i++) {
    if (ctx->handler_cmd[i] == cmd) {
        ctx->handlers[i](ctx, cmd, data, len, ctx->handler_userdata[i]);
        break;
    }
}
```

**Default:** `CRUMBS_MAX_HANDLERS=16` (~68 bytes on AVR)

**Trade-off analysis:**

| Handlers | O(n) Overhead @ 16MHz | Impact on I²C (100kHz) |
| -------- | --------------------- | ---------------------- |
| 4        | ~0.5 µs               | Negligible             |
| 16       | ~2 µs                 | Negligible             |
| 64       | ~8 µs                 | Still negligible       |

**Conclusion:** O(n) is acceptable for I²C applications. O(1) would only matter for:

- High-speed SPI messaging (5000+ msg/s)
- Internal software dispatch (non-I²C)
- Real-time systems with µs-level timing budgets

### Future Option: Restore O(1) for Power Users

If O(1) dispatch is needed in the future, options include:

1. **Automatic mode switching:** Use O(1) when `CRUMBS_MAX_HANDLERS == 256`
2. **Separate handler table:** Externalize handler storage so context stays fixed-size
3. **Compile-time mode flag:** `CRUMBS_HANDLER_MODE_SPARSE` vs `CRUMBS_HANDLER_MODE_DIRECT`

See `_reference/06_handler_configuration_and_performance.md` for detailed design options.

---

## Arduino Separate Compilation Bug

### The Problem

Arduino and PlatformIO compile library source files **separately** from sketch files. This causes a critical ABI mismatch when users define `CRUMBS_MAX_HANDLERS` in their sketch:

```cpp
// ❌ THIS DOES NOT WORK
#define CRUMBS_MAX_HANDLERS 8  // Sketch sees this
#include <crumbs.h>            // But library was compiled with default (16)
```

| Component | CRUMBS_MAX_HANDLERS | sizeof(crumbs_context_t) |
| --------- | ------------------- | ------------------------ |
| Library   | 16 (default)        | ~68 bytes                |
| Sketch    | 8 (user-defined)    | ~36 bytes                |

### The Crash

When `crumbs_init()` is called, the library executes:

```c
memset(ctx, 0, sizeof(*ctx));  // Library thinks ctx is 68 bytes
```

But the user's `ctx` variable is only 36 bytes. The library writes zeros **32 bytes past the end** of the actual struct, corrupting:

- Stack variables
- Return addresses
- Saved registers

**Symptom:** Crash on startup with garbled serial output ("Sta" then reset).

### The Solution

Users MUST set `CRUMBS_MAX_HANDLERS` via **build flags**, not sketch defines:

```ini
# platformio.ini
build_flags = -DCRUMBS_MAX_HANDLERS=8
```

This ensures both library and sketch are compiled with the same value.

### Runtime Detection

Added `crumbs_context_size()` to detect mismatches at runtime:

```c
void setup() {
    if (sizeof(crumbs_context_t) != crumbs_context_size()) {
        Serial.println("ERROR: CRUMBS_MAX_HANDLERS mismatch!");
        while(1);  // Halt — don't corrupt memory
    }
}
```

This catches the bug before `crumbs_init()` can corrupt memory.

---

## Variable-Length Payload Migration (v0.7.x)

### Old Format (Fixed 31 Bytes)

```text
type_id        1 byte
command_type   1 byte
data[7]        28 bytes (float32 × 7)
crc8           1 byte
```

**Problems:**

- Always transmitted 7 floats, even for simple commands
- Inefficient for small messages
- Hard-coded interpretation prevented evolution

### New Format (Variable 4–31 Bytes)

```text
type_id        1 byte
command_type   1 byte
data_len       1 byte (0–27)
data[]         0–27 bytes (opaque)
crc8           1 byte
```

**Benefits:**

- Minimal overhead for small commands (4 bytes for empty payload)
- Opaque bytes allow any encoding (floats, ints, structs, strings)
- Future-proof (typed payloads can be layered on top)

### Breaking Changes

| Old                        | New                             |
| -------------------------- | ------------------------------- |
| `float data[7]`            | `uint8_t data[27]` + `data_len` |
| `CRUMBS_DATA_LENGTH` (7)   | `CRUMBS_MAX_PAYLOAD` (27)       |
| `CRUMBS_MESSAGE_SIZE` (31) | `CRUMBS_MESSAGE_MAX_SIZE` (31)  |
| Fixed encode length        | Variable encode length          |

---

## CRC Implementation Selection

Evaluated multiple CRC-8 variants from AceCRC:

| Variant        | Flash  | RAM   | Speed   |
| -------------- | ------ | ----- | ------- |
| `crc8_bit`     | ~50 B  | 0     | Slowest |
| `crc8_nibble`  | ~90 B  | 0     | Good    |
| `crc8_nibblem` | ~130 B | 16 B  | Faster  |
| `crc8_byte`    | ~280 B | 256 B | Fastest |

**Selected:** `crc8_nibble` — optimal balance of flash size, speed, and zero RAM for AVR targets.

See `docs/crc.md` for full comparison and validation.

---

## API Design Decisions (v0.9.x Analysis)

During v0.9.x development, several API "improvements" were evaluated and rejected:

### Rejected: Sequential Payload Reader

**Proposal:** Add `crumbs_reader_t` with auto-advancing offset:

```c
crumbs_reader_t r;
crumbs_reader_init(&r, data, len);
crumbs_reader_u8(&r, &value);  // auto-advances offset
```

**Why rejected:** For typical 2-4 byte payloads, the setup overhead equals or exceeds the savings. Current offset-based reading is:

- Explicit about message layout (documents the wire format)
- No additional state to manage
- Supports random access if needed

### Rejected: Batch Handler Registration

**Proposal:** Register multiple handlers in one call:

```c
static const crumbs_handler_entry_t handlers[] = {
    { CMD_SET_ALL, handle_set_all, NULL },
    { CMD_SET_ONE, handle_set_one, NULL },
};
crumbs_register_handlers(&ctx, handlers, 2);
```

**Why rejected:** Same line count as individual calls, adds a type definition, requires counting entries. Individual registration is clearer at the call site.

### Confirmed Correct: Full Handler Signature

Minimal signatures like `void (*fn)(const uint8_t *data, uint8_t len)` were considered. The full signature provides:

- Access to `ctx->user_data` for shared state
- Command type for handlers that process multiple commands
- Per-handler user data pointer

The `(void)param;` casts are a one-time cost per handler.

---

## Design Principles

1. **C99 core, C++ HALs allowed** — Core must compile as pure C; HALs may use C++ where platform bindings require it (Arduino Wire).

2. **No dynamic allocation** — All storage is static or stack-allocated. Safe for embedded.

3. **Pay for what you use** — Features like handler dispatch can be disabled entirely (`CRUMBS_MAX_HANDLERS=0`).

4. **Platform guards** — Linux HAL files are guarded so Arduino toolchain can compile the repo without linux-wire dependency.

5. **ABI stability** — Context size is validated at runtime to catch configuration mismatches.
