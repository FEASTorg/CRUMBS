# Linux HAL & example

This repository includes a lightweight Linux HAL that uses the "linux-wire" helper and a small example program you can run on a Linux host (e.g. Raspberry Pi) to act as a CRUMBS controller.

Two important pieces to get set up on Linux:

- linux-wire (the I²C helper library used by the HAL)
- access to the kernel I²C device node (e.g. /dev/i2c-1)

If you need step-by-step install instructions for linux-wire, see the dedicated guide: [Linux‑wire installation](linux-wire-install.md).

Quick build & run overview (example uses /dev/i2c-1):

1. Install prerequisites:

   - C compiler, CMake, make/ninja
   - linux-wire library (provides a CMake config package `linux_wire::linux_wire`) — see the install guide above
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


Notes & troubleshooting

- linux-wire availability: run the linux-wire install guide or use `-DCMAKE_PREFIX_PATH` to point CMake at a local linux-wire build (developer workflow). See `docs/linux-wire-install.md` for details.

- Device permissions: accessing `/dev/i2c-X` usually requires root or membership in the `i2c` group. You can either add your user to that group, configure a udev rule to adjust the node permissions, or run the example with `sudo`.

- I²C driver: confirm the kernel exports /dev/i2c-* devices (e.g. `modprobe i2c_dev`), and that the bus number matches what you pass to the example.

- Example device/address: the Linux example accepts an optional device path and target address on the command line. For example:

```bash
sudo ./crumbs_simple_linux_controller /dev/i2c-1 0x08
```

If you'd like I can add a wrapper script that discovers a reasonable default device path and optionally runs the example as a non-root user when configured with appropriate udev rules.

If you'd like, I can extend the example to accept the device path and address as command-line parameters instead of hardcoding them.
