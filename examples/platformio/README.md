# PlatformIO Example

This project includes a `library.json` manifest so it can be used as a PlatformIO library. There is a small PlatformIO example project in `examples/platformio/controller`.

Quick usage (from a PlatformIO project):

1. Add the library via a git reference or the registry in your project's `platformio.ini`:

```ini
[env:myboard]
platform = atmelavr
framework = arduino
board = uno
lib_deps = https://github.com/FEASTorg/CRUMBS.git#v0.6.1
```

2. Include and use the library in your code: `#include <crumbs_arduino.h>`

Notes:

- `library.json` declares frameworks=arduino and platforms=\* which helps PlatformIO resolve and install the library.
- Local example uses `lib_extra_dirs` to reference the repo root so you can build the example during development.
