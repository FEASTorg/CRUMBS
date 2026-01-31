# Linux HAL & example

This repository includes a lightweight Linux HAL that uses the "linux-wire" helper and a small example program you can run on a Linux host (e.g. Raspberry Pi) to act as a CRUMBS controller.

> **Important:** The Linux HAL currently supports **controller mode only**. Peripheral mode (acting as an I²C target device) is not implemented. Linux hosts typically act as I²C controllers anyway; for peripheral devices, use an Arduino or other microcontroller with the Arduino HAL.

Two important pieces to get set up on Linux:

- linux-wire (the I²C helper library used by the HAL)
- access to the kernel I²C device node (e.g. /dev/i2c-1)

If you need step-by-step install instructions for linux-wire, see the dedicated guide: [Linux‑wire installation](linux-wire-install.md).

Quick build & run overview (example uses /dev/i2c-1):

1. Install prerequisites
   - C compiler, CMake, make/ninja
   - linux-wire library (provides a CMake config package `linux_wire::linux_wire`) — see the install guide above
   - An account that can access the I2C device (or run the example with root privileges)

1. Configure & build (from repo root)

```bash
mkdir -p build && cd build
cmake .. -DCRUMBS_ENABLE_LINUX_HAL=ON -DCRUMBS_BUILD_EXAMPLES=ON
cmake --build . --parallel
```

1. Run the example (the example uses `/dev/i2c-1` by default)

```bash
sudo ./crumbs_simple_linux_controller
```

1. Installing a release to the system (optional)

If you want to install the built CRUMBS library and headers system-wide:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
```

Then install with:

```bash
sudo cmake --install . --prefix /usr/local
```

Notes & troubleshooting

- linux-wire availability: run the linux-wire install guide or use `-DCMAKE_PREFIX_PATH` to point CMake at a local linux-wire build (developer workflow). See `docs/linux-wire-install.md` for details.

- Device permissions: accessing `/dev/i2c-X` usually requires root or membership in the `i2c` group. You can either add your user to that group, configure a udev rule to adjust the node permissions, or run the example with `sudo`.

- I²C driver: confirm the kernel exports /dev/i2c-\* devices (e.g. `modprobe i2c_dev`), and that the bus number matches what you pass to the example.

Example device/address and scan mode

The Linux example accepts an optional device path and target address on the command line. For example (send a message to 0x08):

```bash
sudo ./crumbs_simple_linux_controller /dev/i2c-1 0x08
```

Scanner mode

The example also supports a simple built-in scan mode to discover CRUMBS devices on the bus. Usage:

```bash
# Non-strict probe (attempt reads, send probe write if needed)
sudo ./crumbs_simple_linux_controller scan

# Strict probe (read-only checks)
sudo ./crumbs_simple_linux_controller scan strict
```

Note: the example currently treats the first argument as either a device path (normal run) or a literal `scan` command; to scan using a non-default device path you can run the binary from that working directory or modify the example to accept a path + scan mode.

## Interactive Controller

For interactive testing and debugging, use the `crumbs_interactive_controller` example which provides a command-line interface to control LED and servo peripherals:

```bash
sudo ./crumbs_interactive_controller /dev/i2c-1
```

This opens an interactive prompt where you can type commands:

```text
crumbs> help              # Show available commands
crumbs> scan              # Scan for CRUMBS devices
crumbs> led set_all 0x0F  # Turn on LEDs 0-3
crumbs> led set 2 0       # Turn off LED 2
crumbs> led state         # Get current LED state
crumbs> servo angle 0 90  # Set servo 0 to 90 degrees
crumbs> servo sweep 0 0 180 10  # Sweep servo 0
crumbs> addr led 0x10     # Change LED address
crumbs> quit              # Exit
```

See `examples/linux/interactive_controller/` for the full source.

## Wrapper script / udev

If you'd prefer the example to run as a non-root user, add a udev rule to adjust `/dev/i2c-*` permissions or add your account to the `i2c` group. I can add a small wrapper script that discovers the first available `/dev/i2c-*` device and invokes the example with appropriate parameters.
