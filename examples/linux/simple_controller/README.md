# Simple Linux Controller Example

This small program demonstrates using the CRUMBS C API on Linux. It:

- Initializes a `crumbs_context_t` controller and a `crumbs_linux_i2c_t` handle
- Sends a sample message to the configured slice address (see `SLICE_ADDR` in source)
- Requests and decodes the reply using the Linux HAL I/O primitives

Build instructions are in the project root (CMake): enable `CRUMBS_ENABLE_LINUX_HAL` and `CRUMBS_BUILD_EXAMPLES`.

Hardware: the example expects a peripheral listening on `/dev/i2c-1` at I2C address 0x08 by default. You can override both at runtime:

```bash
./crumbs_simple_linux_controller /dev/i2c-1 0x08
```
