# linux-wire installation

This guide documents a few safe ways to get linux-wire available to CMake so CRUMBS can find it via:

```cmake
find_package(linux_wire CONFIG REQUIRED)
target_link_libraries(my_target PUBLIC linux_wire::linux_wire)
```

Two approaches are shown below:

- System install (recommended for end users / test hosts)
- Local build and point CMake at the build tree (convenient for development)

Prerequisites

- C compiler (gcc/clang)
- CMake >= 3.13
- Make or Ninja (any generator supported by CMake)

If you plan to use CRUMBS on the same host against a physical I²C device, also ensure the kernel has the i2c-dev driver loaded (see "Permissions" below).

## System install (recommended)

1. Clone linux-wire

```sh
git clone https://github.com/FEASTorg/linux-wire.git
cd linux-wire
```

1. Configure and build

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
```

1. Install system-wide (default prefix /usr/local)

```sh
sudo cmake --install build --prefix /usr/local
```

This will install headers to /usr/local/include/linux_wire, the static library (liblinux_wire.a) in /usr/local/lib, and the CMake package files under /usr/local/lib/cmake/linux_wire. The package files are what allow `find_package(linux_wire CONFIG REQUIRED)` to succeed and expose the target `linux_wire::linux_wire`.

1. Verify the installation

If successful you should see the path to the installed linux_wireConfig.cmake under /usr/local/lib/cmake/linux_wire.

Run this command to verify:

```sh
cmake --find-package -DNAME=linux_wire -DCOMPILER_ID=GNU -DLANGUAGE=C -DMODE=EXIST
```

> should print: `"linux_wire found."`

## Local build (developer workflow)

If you prefer not to install system-wide during development, you can point CMake at a local build tree.

Example — build linux-wire in `~/src/linux-wire` and use it from CRUMBS without root install:

```sh
# inside linux-wire
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel

# inside CRUMBS repo
mkdir -p build && cd build
cmake .. -DCRUMBS_ENABLE_LINUX_HAL=ON -DCRUMBS_BUILD_EXAMPLES=ON \
    -DCMAKE_PREFIX_PATH=$HOME/src/linux-wire/build
cmake --build . --parallel
```

This tells CMake to search the linux-wire `build` directory for its config package files without needing to install them system-wide.

## Permissions & kernel I²C device access

Common friction when running examples on Linux is permissions on `/dev/i2c-*` and the kernel driver.

- Ensure `i2c-dev` is loaded:

```sh
lsmod | grep i2c_dev || sudo modprobe i2c-dev
```

- Running the example often requires root access or membership of a group that can access the device node (commonly `i2c`). Add your user to that group if desired:

```sh
sudo usermod -a -G i2c $USER
# then log out / log in for group change to take effect
```

- Alternatively run examples with `sudo` (quick, but grants privileges).

## Troubleshooting

- CMake can't find linux_wire: verify the config files exist under your install prefix (search for linux_wireConfig.cmake) or use `-DCMAKE_PREFIX_PATH` to point to a build/install location.
- Link errors referencing linux_wire symbols usually mean the package was not linked — confirm `target_link_libraries(... linux_wire::linux_wire)` is resolved by CMake.
- If you get permission denied when opening /dev/i2c-\* use `sudo` or fix group/udev rules.
- If using a Raspberry Pi, ensure I²C is enabled in `raspi-config` and the correct bus number is used (usually `/dev/i2c-1`).

If you want, I can add a small script to the repo to help developers build & test linux-wire + CRUMBS locally using a consistent CMAKE_PREFIX_PATH workflow.
