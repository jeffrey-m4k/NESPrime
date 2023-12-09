<p align="center"><img src="https://github.com/jeffrey-m4k/NESPrime/assets/52832383/fc5f817b-523d-4e2f-b498-a3e1cef4a413" width=192 height=192/><br/>
  <img src="https://github.com/jeffrey-m4k/NESPrime/assets/52832383/4f3d3ea6-6265-4513-b9c9-5fe58ad5e328"/><br/>
</p>

# Overview
NESPrime is a simple NES emulator written in C++ using the SDL2 library.

### Features
- Mostly cycle-accurate CPU, PPU, and APU emulation
- Supports a few popular mappers, with more on the way

### Keybinds
- Console:
  - A: Z
  - B: X
  - Select: TAB
  - Start: ENTER
  - D-Pad: Arrow Keys
- Emulator:
  - Pause: ESC
  - Fullscreen: F11
 
### Supported Mappers:
  - 0 (NROM) - *Donkey Kong, Super Mario Bros, Ice Climber...*
  - 1 (MMC1) - *Metroid, Mega Man 2, Legend of Zelda...*
  - 2 (UNROM) - *Mega Man, Castlevania, DuckTales...*
  - 3 (CNROM) - *Back to the Future, Ghostbusters, Friday the 13th...*

### Roadmap:
- More mappers - MMC3, etc...
- More fleshed-out UI with emulation settings, better ROM browsing, keybinds
- PAL compatibility? (for troublesome games like Elite)
- Debugging features

# Credits
Thanks to the wonderful [NESdev Wiki](https://www.nesdev.org/wiki/Nesdev_Wiki) for their very thorough documentation of the NES's internals, without which I doubt I could've made this.
