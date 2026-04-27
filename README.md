# My-Take-on-SFC
My Take on SFC Emulation. This project is only for educational purposes to understand and explore the inner workings of the legendary SNES/SFC. Honestly you really don't want to use it for anything else.

This repository (eventually) is going to focus on code readability and ease of understanding of the SNES's operation, and not performance and convenience.
I am planning to further optimize things at the cost of complex coding, but that is going to happen in [this repo](https://github.com/MoeenFJ/PicoSFC).
#### This project is currently heavily WIP, A LOT of the features are missing and the performance is onpar with a potato.

# How to use
Compile using:
```
git clone https://github.com/MoeenFJ/My-Take-on-SFC.git
cd My-Take-on-SFC
mkdir build
cd build
cmake ..
make
```
Run using:
```
./MTOFCS <path_to_sfc_file>
```
#### Emulator will start paused. To pause and unpause use the P key.
### Emulator keys
Esc -> Close and save the sram

P -> pause/unpause

O -> Print some PPU info

0 -> Run one frame

9 -> Run one master clock

\- -> Run one instruction
  
= -> Run one line

1 -> Toggle console CPU log

2 -> Dump wram

3 -> Toggle trace (save cpu log to a file, will take a LOT of space)

4 -> Dump vram

5 -> Dump oaram

F1 - F4 -> Only show a specific BG

F5 -> Only show the objects

F6 -> Show everything (default)

### In-game keys
A, S, Z, X are mapped to Y, X, B, A respectively.

Arrow key is mapped to direction pad.

Space is mapped to Start.

Enter is mapped to Select.

L, R are not mapped yet.
