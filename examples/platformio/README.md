# PlatformIO Examples

This directory contains PlatformIO project examples for CRUMBS.

## Examples

| Directory                    | Description                            |
| ---------------------------- | -------------------------------------- |
| `controller/`                | Basic controller example               |
| `peripheral/`                | Basic peripheral example               |
| `handlers/led_peripheral/`   | LED peripheral with command handlers   |
| `handlers/servo_peripheral/` | Servo peripheral with command handlers |

## Quick Start

1. Open any example folder in VS Code with PlatformIO extension
2. Update the COM port in `platformio.ini`
3. Build and upload: `pio run -t upload`
4. Monitor: `pio device monitor`

## Using CRUMBS in Your Project

Add the library via the registry in your project's `platformio.ini`:

```ini
[env:myboard]
platform = atmelavr
framework = arduino
board = nanoatmega328new
lib_deps = cameronbrooks11/CRUMBS@^0.9.4
```

Or via git reference:

```ini
lib_deps = https://github.com/FEASTorg/CRUMBS.git#v0.9.4
```

Then include in your code: `#include <crumbs_arduino.h>`

## Memory Optimization

For memory-constrained devices, reduce handler table size:

```ini
build_flags = -DCRUMBS_MAX_HANDLERS=8
```

This saves ~40 bytes of RAM on AVR (from default of 16 handlers).

## Notes

- `library.json` declares frameworks=arduino and platforms=\* for PlatformIO resolution
- Handler examples demonstrate `crumbs_register_handler()` for command dispatch
- Command headers in `include/` define device-specific commands
