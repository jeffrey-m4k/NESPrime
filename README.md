<p align="center"><img src="https://github.com/jeffrey-m4k/NESPrime/assets/52832383/1428cf69-13ce-40ca-953a-765901518195" width=192 height=240/>
</p>

# Overview
NESPrime is a simple NES emulator written in C++ using the SDL2 library.

![demo](https://github.com/jeffrey-m4k/NESPrime/assets/52832383/590795cf-dc30-4b3d-aed6-664979539a91)

### Features
- Mostly cycle-accurate CPU, PPU, and APU emulation
- Debug display for nametable and pattern table contents
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
