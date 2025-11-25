# Linux HAL & example

This repository includes a lightweight Linux HAL that uses the "linux-wire" helper and a small example program
you can run on a Linux host (e.g. Raspberry Pi) to act as a CRUMBS controller.

In short: install linux-wire, ensure you can access /dev/i2c-* (root or proper permissions), then use CMake to build.

Quick steps (example uses /dev/i2c-1):

1. Install prerequisites:
   - C compiler, CMake, make/ninja
   - linux-wire library (provides a CMake config package `linux_wire::linux_wire`)
   - An account that can access the I2C device (or run the example with root privileges)

1. Configure & build (from repo root):

```bash
mkdir -p build && cd build
cmake .. -DCRUMBS_ENABLE_LINUX_HAL=ON -DCRUMBS_BUILD_EXAMPLES=ON
cmake --build . --parallel
```

1. Run the example (the example uses `/dev/i2c-1` by default):

```bash
sudo ./crumbs_simple_linux_controller
```

- Notes & troubleshooting

- The build expects linux-wire to be available as a CMake config package (target `linux_wire::linux_wire`). If you built linux-wire from source, install or export its CMake package file so CMake can locate it.

- Accessing `/dev/i2c-X` typically requires root or membership in a group with proper permissions (often `i2c`). Use `sudo` to run the example or configure udev rules for non-root access.
- If your I2C bus is not `/dev/i2c-1`, edit `examples/linux/simple_controller/simple_linux_controller.c` or pass an alternate device path by editing the source before building.

If you'd like, I can extend the example to accept the device path and address as command-line parameters instead of hardcoding them.
