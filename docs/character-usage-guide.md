# Character Usage Guide

Quick reference for ASCII vs. Unicode character usage in CRUMBS codebase.

## Rule of Thumb

**Code = ASCII only. Docs = Professional typography allowed.**

## Code Files (.c, .cpp, .h, .hpp, .ino, .py)

### ❌ Never Use

| Don't                  | Do              | Context           |
| ---------------------- | --------------- | ----------------- |
| `I²C`                  | `I2C`           | Comments, strings |
| `90°`                  | `90deg` or `90` | String literals   |
| `—` (em-dash)          | `-`             | Comments          |
| `–` (en-dash)          | `-`             | Comments, ranges  |
| `'` `'` (curly quotes) | `'`             | String literals   |
| `"` `"` (curly quotes) | `"`             | String literals   |

**Why:** Compiler compatibility, text editor safety, terminal display issues

### Examples

```c
// ❌ BAD
/* Initialize I²C bus at address 0x08 — default config */
printf("Set servo to 90°\n");
float range = 0.0–1.0;

// ✅ GOOD
/* Initialize I2C bus at address 0x08 - default config */
printf("Set servo to 90deg\n");
float range = 0.0 to 1.0;  // or: 0.0-1.0
```

## Configuration Files (.json, .ini, .properties, CMakeLists.txt)

**Rule:** ASCII only - maximum tool compatibility

## Documentation Files (.md)

### ✅ Allowed (Enhances Readability)

| Character     | Usage               | Example                       |
| ------------- | ------------------- | ----------------------------- |
| `–` (en-dash) | Ranges, connections | `4–31 bytes`, `Arduino–Linux` |
| `—` (em-dash) | Punctuation, breaks | `CRUMBS — a protocol library` |
| Box-drawing   | Protocol diagrams   | `┌──┬──┐`                     |
| Curly quotes  | Narrative text      | "Controller sends…"           |

### ❌ Not Allowed

- Emojis (except badges/shields in README)
- Decorative Unicode symbols
- Non-standard punctuation

### Examples

```markdown
<!-- ✅ GOOD -->

- Message format: 4–31 bytes
- CRUMBS is lightweight — designed for embedded systems

<!-- ❌ BAD -->

- Message format: 4-31 bytes (use en-dash in docs)
- CRUMBS is lightweight - designed for... (use em-dash in docs)
```

## README Badges

Emoji/shields from badge services (shields.io, img.shields.io) are fine in README files only.

## Summary

| File Type                             | Rule                       |
| ------------------------------------- | -------------------------- |
| `.c` `.cpp` `.h` `.hpp` `.ino` `.py`  | ASCII only                 |
| `.json` `.ini` `.properties` `.cmake` | ASCII only                 |
| `.md` (docs/)                         | Professional typography OK |
| `README.md`                           | Typography OK + badges OK  |

**When in doubt:** Use ASCII. It always works everywhere.
