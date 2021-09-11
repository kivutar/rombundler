# ROMBundler

ROMBundler is a way to release your homebrew retro game as an executable.

It is based on this example libretro frontend https://github.com/heuripedes/nanoarch

# Compiling

Dependencies are: GLFW 3

    make

# Usage

Modify the config.ini to fit your needs:

    title = Micro Mages
    core = nes_libretro.dylib
    rom = Micro Mages.nes

You can download libretro cores from http://buildbot.libretro.com/nightly/ and place it in the same folder, as well as your ROM. (But make sure to comply to the core license)

ROMBundler will read the ini file, load the emulator and the ROM and the game will start in full screen mode.

