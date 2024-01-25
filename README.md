#	Another Gameboy Advance Emulator

This is an hobby project on which I spent countless hours in the last couple of months. It is not meant for use, since it does not support goodies that emulators usually provide (such as savestates), and the UI offers little functionality.

# Building
For now there is no guide on how to build this project.

# Dependencies
The emulator uses quite a few dependencies, none for the emulation in itself, but for video and audio presentation.

<ul>
<li>SDL2/SDL_Image for creating a window and outputting samples to the sound driver</li>
<li>ImGui from https://github.com/ocornut/imgui, for UI</li>
<li>ImGui file dialog https://github.com/aiekick/ImGuiFileDialog, to open/write files (such as loading roms and storing saves)</li>
<li>mINI https://github.com/metayeti/mINI, for config file and rom database</li>
<li>Ring-Buffer https://github.com/jnk0le/Ring-Buffer, used to store and read audio samples</li>
<li>Glew https://glew.sourceforge.net/, loads the necessary OpenGL API functions</li>
<li>Uses OpenGL 3.3, but only very basic functionality. Any OpenGL version above 2 should work (maybe)</li>
</ul>

# What works
<li>Emulation should be accurate enough</li>
<li>Supports save types SRAM,EEPROM and FLASH</li>
<li>Supports RTC emulation with datetime taken from the internal RTC of the host computer</li>
<li>All PPU modes implemented, but unfortunately I did not test the bitmap modes, and problems might appear with sprites and blending in those modes</li>
<li>All audio channels implemented (not quite sure that the sound mixing is correct, though)</li>
<li>All object types implemented, even OBJWIN modes (used for Pokemon logos)</li>
<li>Passes all tests in ArmWrestler https://github.com/destoer/armwrestler-gba-fixed/tree/master</li>
<li>Passes all generated tests from https://github.com/DenSinH/FuzzARM</li>

Tested games:
<li>Doom</li>
<li>Pokemon Emerald (Testing right now with a playthrough, almost finished, everything works well)</li>
<li>TloZ: The Minish Cap</li>
<li>Final Fantasy 6 Advance</li>
<li>Fire Emblem</li>
<li>Castlevania Aria Of Sorrow</li>
<li>Super Mario World/Super Mario advance</li>

For all these games, no immediate problems were found, albeit I played for a couple of minutes (more testing required)

# What does not

<li>Solar sensor and rumble not implemented (The only implemented GPIO device is RTC)</li>
<li>Serial cable is not implemented</li>
<li>Does not pass some tests in the mGBA suite (and some of the subtests fail alltogether)</li>
<li>AGS aging cartridge works, but a lot (I mean, a LOT) of tests fail, moslty due to incorrect timings</li>

# Rom database
Here one should put an header with the internal name of a cartridge, 
and after that one or more options are possible
<li>backup_type = [rom backup type]</li>
<li>gpio = "list of gpio devices divided by a comma"</li>
<li>[gpio device type] = [list of pin connections]</li>

Some examples can already be found in the rom_db.txt file.
As one could probably imagine, this is "hard" to use, since it requires knowledge of the internals of the cartridge. 

If no header that corresponds to the internal game name is found, all options are set to NONE.

If the backup_type entry is not found, the emulator will default to NONE.
As such, saves will not work (or the game might not boot).

gpio is a list of the gpio devices on the cartridge, divided by commas and enclosed in "". For now, the only device that is taken in consideration by the emulator is RTC (used for counting datetime)

Then, for each device in the list, there is a corresponding entry with a list of pin connections (which literally is which pin on the cartridge slot corresponds to the pin on the GPIO device). This might not be necessary, since GPIO devices might be always connected in the same way, but since I didn't test enough I can't say for sure.

# Configuration file

Headers:

[BIOS]
skip = true/false (no default, tells the emulator if the bios entry sequence should be skipped)
file = [bios file location] 

[EMU]
start_paused = true/false 
scale = [integer number > 0] (the screen scaling value)

[ROM]
default_rom = [rom path] (if set, the emulator immediately loads the provided rom)

Each entry can be simply removed by putting # before it

# Important
Note that no roms or bios files are provided

To bring up the menu for loading roms/save files
and emulation start/pause: focus the emulator
window and press ESC.
Each window that is opened through the
main menu bar can be scaled.

Moreover: the project requires c++20 to be compiled,
and since the interpreter makes heavy use of templates,
compilation times could get quite long.

BIOS startup has been fixed, and the previous behaviour was caused by
as stupid BIOS read implementation. I have no clue why or how everything
else worked. There is a lesson to learn here
