# Breakout

A small Breakout clone in C++20 with [raylib](https://www.raylib.com/).

## Build in CLion
Open this folder in CLion. On first load, CMake downloads raylib (needs internet).
Then select the `breakout` run configuration and press Run.

## Build from the command line
```bash
cmake -S . -B build
cmake --build build
./build/breakout
```

## Controls
- Left / Right arrows: move paddle
- Esc: quit
