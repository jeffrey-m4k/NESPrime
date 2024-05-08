<p align="center"><img src="https://github.com/jeffrey-m4k/NESPrime/assets/52832383/1428cf69-13ce-40ca-953a-765901518195" width=192 height=240/>
</p>

# Overview
NESPrime is a simple NES emulator written in C++ using the SDL2 library.

### Features
- Mostly cycle-accurate CPU, PPU, and APU emulation
- Expansion audio chip support (currently only Sunsoft 5B)
- Save file support for cartridges with battery-backed RAM
- Debug display for nametables and pattern tables
- Real-time oscilloscope viewer for APU and expansion chip channels
- Supports the most popular mappers, with more on the way

### Screenshots
![Screenshot with debug windows](https://github.com/jeffrey-m4k/NESPrime/assets/52832383/a5d32a30-514d-4e68-b788-9dd24b2be5d6)
![Oscilloscope view](https://github.com/jeffrey-m4k/NESPrime/assets/52832383/c1fd3185-e3e3-40ff-a661-071bc7fef32b)

### Keybinds
- **Console:**

| Keybind | Action |
| --- | --- |
| Z | A |
| X | B |
| TAB | Select|
| ENTER | Start|
| Arrow Keys | D-Pad |

- **Emulation:**

| Keybind | Action |
| --- | --- |
| ESC | Pause |
| F11 | Toggle Fullscreen |
| CTRL+1 | +5% Emulation Speed |
| CTRL+2 | -5% Emulation Speed |
| CTRL+3 | Reset Emulation Speed |
| CTRL+4 | Pattern Table Viewer |
| CTRL+5 | Nametable Viewer |
| CTRL+6 | APU Channel Viewer |
 
### Supported Mappers:

| Mapper | Example Games |
| --- | --- |
| 0 (NROM) | *Donkey Kong, Super Mario Bros, Ice Climber* |
| 1 (MMC1) | *Metroid, Mega Man 2, Legend of Zelda* |
| 2 (UNROM) | *Mega Man, Castlevania, DuckTales* |
| 3 (CNROM) | *Back to the Future, Ghostbusters, Friday the 13th* |
| 4 (MMC3) | *Mega Man 3-6, Super Mario Bros 2+3, Kirby's Adventure* |
| 7 (AxROM) | *Battletoads, Solstice* |
| 11 (Color Dreams) | *Spiritual Warfare, Exodus...* |
| 69 (Sunsoft FME-7) | *Batman: Return of the Joker, Gimmick!* |
| 228 (Active Ent.) | *Action 52* |

### Roadmap:
- More mappers!
- More fleshed-out UI with emulation settings, better ROM browsing, keybinds
- PAL compatibility? (for troublesome games like Elite)
- Debugging features

# Credits
Thanks to the wonderful [NESdev Wiki](https://www.nesdev.org/wiki/Nesdev_Wiki) for their very thorough documentation of the NES's internals, without which I doubt I could've made this.
