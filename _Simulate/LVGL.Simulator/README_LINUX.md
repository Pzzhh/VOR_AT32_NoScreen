# Linux SDL2 simulator

This is the Linux/SDL2 build of the existing LVGL 8.3 simulator. The Visual
Studio project and the AT32 firmware build remain separate.

## Dependencies (Ubuntu 24.04)

```sh
sudo apt-get update
sudo apt-get install -y build-essential cmake libsdl2-dev socat
```

## Build

Run from `_Simulate/LVGL.Simulator`:

```sh
cmake -S . -B build-linux -DCMAKE_BUILD_TYPE=Release
cmake --build build-linux --parallel
```

The executable is `build-linux/lvgl_simulator`. This target excludes the Win32
display driver, Windows serial-port implementation, and AT32 USB/USART drivers.

The application-level platform macros coexist and have separate meanings:

- `use_windows` remains owned by the existing Windows desktop application and
  is not changed by this Linux build.
- `use_linux=1` is defined only for the Linux simulator target and is available
  for Linux-specific application adaptations.
- `LV_PLATFORM_LINUX=1` remains the low-level LVGL/Linux port configuration
  switch.

## Virtual serial ports

Create the three PTY pairs in a separate terminal before running the simulator:

```sh
./linux/create_virtual_serials.sh
```

- RK right: `/tmp/ttyRKRight` (application), `/tmp/ttyRKRightPeer` (test peer)
- RK left: `/tmp/ttyRKLeft` (application), `/tmp/ttyRKLeftPeer` (test peer)
- USB ACM: `/tmp/ttyUSB` (application), `/tmp/ttyUSBPeer` (test peer)

The script stays in the foreground and removes its links when stopped.

The desktop persistence files `Task.hex` and `config.hex` are stored in the
simulator's current working directory. Linux uses POSIX path separators while
the existing Windows paths are retained unchanged.

## Run

From an Ubuntu graphical desktop session:

```sh
./build-linux/lvgl_simulator
```

When starting over SSH, export the graphical session's `DISPLAY` and
`XAUTHORITY` first. Runtime verification is intentionally left to the user.
