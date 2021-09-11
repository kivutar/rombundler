# ROMBundler

ROMBundler is a way to release your homebrew retro game as an executable.

It is based on this example libretro frontend https://github.com/heuripedes/nanoarch

# Compiling

Dependencies are: GLFW 3

    make

# Usage

Modify the config.ini to fit your needs:

    title = Shrine Maiden Shizuka Demo 2
    core = blastem_libretro.dylib
    rom = Shrine Maiden Shizuka Demo 2.md

You can download libretro cores from http://buildbot.libretro.com/nightly/ and place it in the same folder, as well as your ROM. (But make sure to comply to the core license)

ROMBundler will read the ini file, load the emulator and the ROM and the game will start in full screen mode.

You can then rename `rombundler` with the name of your game, change it's icon, and distribute this as a zip.

# TODO

 * Windows releases
 * Linux releases
 * Static linking

