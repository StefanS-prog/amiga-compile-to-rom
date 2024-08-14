Amiga Kickstart ROM generation
==============================
This project demonstrates the feasibility of directly cross-compiling to the Amiga hardware platform by producing an Amiga Kickstart ROM image. It uses a custom linker script so that the memory layout must not be defined in the source code.

Setup
-----
1. Install Ubuntu Server in a virtual machine of your choice. 4 GB RAM and 16 GB storage are plenty.
2. Install the missing packages in Ubuntu Server with `sudo apt install gcc gcc-m68k-linux-gnu libpng-dev ffmpeg`.
3. Clone the repository with `git clone https://github.com/StefanS-prog/amiga-compile-to-rom.git`.
4. Enter `cd amiga-compile-to-rom` and `chmod a+x build`.
5. Create the Kickstart ROM with `./build`. It is `game.a` in the same directory.
6. Execute the Kickstart ROM in an Amiga emulator.

Asset sources
-------------
- The tileset is based on this [one](https://opengameart.org/content/trivial-ega-tiles) (License CC0).
- The sound is downsampled from this [one](https://freesound.org/people/newagesoup/sounds/348243) (License CC0).
