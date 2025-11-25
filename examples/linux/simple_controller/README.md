# Simple Linux Controller Example

This small program demonstrates using the CRUMBS C API on Linux. It:

- Initializes a `crumbs_context_t` controller and a `crumbs_linux_i2c_t` handle
- Sends a sample message to the configured slice address (see `SLICE_ADDR` in source)
- Requests and decodes the reply using the Linux HAL I/O primitives

Build instructions are in the project root (CMake): enable `CRUMBS_ENABLE_LINUX_HAL` and `CRUMBS_BUILD_EXAMPLES`.

Prerequisites & linux-wire

- The Linux HAL depends on `linux-wire` (CMake package `linux_wire::linux_wire`). See `docs/linux-wire-install.md` for install or local-build instructions.

Device access

- Example assumes `/dev/i2c-1` is available and readable. Either run it with `sudo` or add your user to the `i2c` group / set a udev rule so your user can access the device node.

Hardware: the example expects a peripheral listening on `/dev/i2c-1` at I2C address 0x08 by default. You can override both at runtime:

```bash
./crumbs_simple_linux_controller /dev/i2c-1 0x08
```
