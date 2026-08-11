# OpenLara for SF2000 & GB300
This is a custom port of the classic Tomb Raider engine (OpenLara) specifically optimized for the Data Frog SF2000 and GB300 retro handheld consoles. 

The original software renderer struggled with performance and visual glitches on this low-end hardware. I've heavily modified the inner rendering loop and properly implemented the Z-buffer memory allocation so the game runs smooth and you no longer see objects clipping through walls. 

## Key Features
* Fully working Z-buffer (depth sorting is fixed).
* Hardware-specific optimizations (using CPU registers instead of slow RAM reads) to keep the framerate playable.
* Default resolution locked at 50% for optimal performance.
* Pre-configured controls for the SF2000/GB300 layout.

## Controls
* **D-Pad** - Movement
* **R** - Action / Shoot
* **L** - Walk (prevents falling off edges)
* **SELECT** - Look around / Side-step
* **START** - Inventory

## How to play
1. Put the compiled core file (`core_87000000`) into your `cores` folder on the SD card.
2. Copy your Tomb Raider 1 level files (`.PHD` format, like `LEVEL1.PHD`) to your SD card (for example into the `roms` folder).
3. Launch the `.PHD` file from the console menu and give it a few seconds to load.

***
Change name to core_87000000 
*(Based on the original OpenLara project by XProger)*
