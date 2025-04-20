# ROMBundler

ROMBundler is a way to release your homebrew retro game as an executable.

It is based on this example libretro frontend https://github.com/heuripedes/nanoarch

The main differences are that the frontend is controlled by an ini file instead of command line flags to make it easier to bundle ROMs. Also we use glad instead of glew, added openal for cross platform audio, and will add joypad support and maybe a few shaders.

# Compiling

Dependencies are:

 * GLFW 3
 * OpenAL
 * OpenGL 2.1

See our Github Action config files for detailed instructions on how to setup dependencies per OS.

To compile:

```shell
make
```

# Usage

You don't need to compile `rombundler` if you just want to use it, you can get one of the binary releases [here](https://github.com/kivutar/rombundler/releases)

Then modify the config.ini to fit your needs:
```ini
title = Shrine Maiden Shizuka Demo 2
core = ./blastem_libretro.dylib
rom = ./Shrine Maiden Shizuka Demo 2.md
swap_interval = 1
full_screen = false
hide_cursor = false
map_analog_to_dpad = true
window_width = 800
window_height = 600
aspect_ratio = 1.333333
```
You can download libretro cores from http://buildbot.libretro.com/nightly/ and place it in the same folder, as well as your ROM. (But make sure to comply to the core license).

For Windows, the core need to be a `.DLL`, for OSX it needs to be a `.dylib`, and for Linux a `.so`. ROMBundler releases are for 64bit systems only for now.

Place your ROM in the same folder. And set the ROM name in your config.ini.

ROMBundler will read the ini file, load the emulator and the ROM and the game will start.

You can then rename `rombundler` with the name of your game, change it's icon, and distribute this as a zip.

## Inputs configuration

You can choose what kind of device is plugged in the console ports in the config.ini:

```ini
port0 = 3
port1 = 1
```

The identifiers can be found in the libretro.h:

```cpp
#define RETRO_DEVICE_NONE         0
#define RETRO_DEVICE_JOYPAD       1
#define RETRO_DEVICE_MOUSE        2
#define RETRO_DEVICE_KEYBOARD     3
#define RETRO_DEVICE_LIGHTGUN     4
#define RETRO_DEVICE_ANALOG       5
#define RETRO_DEVICE_POINTER      6
```

## Emulator configuration

If you need special options for the emulator, you can set them by creating a options.ini file like this:

```ini
fceumm_sndvolume = 7
fceumm_palette = default
fceumm_ntsc_filter = composite
```
## Shaders

In the config.ini, you can specify a single pass shader. For example:

```ini
shader = zfast-crt
filter = linear
```

Default values are

```ini
shader = default
filter = nearest
```

For now available shaders are:

 * default
 * zfast-crt
 * zfast-lcd

We recommand using the linear filter with the CRT and LCD shaders.

# TODO

 * Provide an OSX .app
 * Static linking for Linux
 * Switch full screen / windowed
 * Multitap option

# Known working cores

 * lutro_libretro
 * fceumm_libretro
 * blastem_libretro
 * bluemsx_libretro
 * snes9x_libretro
 * genesis_plus_gx_libretro
 * nes_libretro
 * mesen_libretro
 * mesen-s_libretro
 * dosbox_pure_libretro
 * mgba_libretro
 * gambatte_libretro
 * gearsystem_libretro
 * mednafen_psx_libretro
 * pcsx_rearmed_libretro
 * melonds_libretro
 * swanstation_libretro
 * duckstation_libretro
 * fbneo_libretro

Not yet compatible:

 * md_libretro (input issues)
 * sameboy_libretro (audio issues)
 * mupen64plus_next_libretro (various issues)
 * parallel_n64_libretro (various issues)
 * ppsspp_libretro (GL issues)
