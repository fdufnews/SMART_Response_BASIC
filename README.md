SMART Response Basic
=============

The SMART Response Basic is based on the original work of [Robin Edwards](https://github.com/robinhedwards/ArduinoBASIC).

Original README.md file can be [read here](README_original.md)

My aim is to make a mobile programming platform that can be used on the lab or on the road to test components.

The SMART Response XE terminal is a very attractive platform :
* 384x136 pixels LCD screen,
* a good keyboard,
* a CPU with 128kB of program flash memory, 16kB of RAM, 4k of EEPROM
* 128kB of external flash
* it can work for hours on 4 AAA batteries.

Nevertheless, there are drawbacks. The main one is how the display works. There is a memory plane inside the display that hold the graphic but this memory cannot be read back as the display interface is in serial mode. This make it difficult to make addition of graphics over existing ones. Especially as the memory is written 3 pixels at a time. If you want to toggle one pixel you can potentially overwrite two others.

So, at first I will try to make BASIC work in text mode only.

TODO list
----------
- [x] make BASIC work with keyboard and LCD display without any other modification
- [ ] enhance display functions
  - [ ] use hardware scroll to speed up display (coded but not currently in use)
  - [x] font change and dynamic adaptation to font size
  - [x] use bottom line of screen to show status of terminal (battery status, font in use, line wrap, free mem...)
- [x] add save/restore BASIC programs using ~~Flash memory of the terminal~~ an SD card
- [ ] add SPI support
- [ ] add I2C support

Prerequisites
-------------
1. A SMART Response XE terminal.

2. (Optional) A Piezoelectric buzzer for keyboard clicks and other sounds [see hack on the github of the hardware](https://github.com/fdufnews/SMART-Response-XE-schematics).

3. (Optional) An LED to show some status or to help debug software [see hack on the github of the hardware](https://github.com/fdufnews/SMART-Response-XE-schematics).

4. (Optionnal) An SD card reader in order to save/restore progr to show some status or to help debug software [see hack on the github of the hardware](https://github.com/fdufnews/SMART-Response-XE-schematics).

5. (Optionnal) An SD card reader in order to save/restore program [see hack on the github of the hardware](https://github.com/fdufnews/SMART-Response-XE-schematics).

   

Getting started
-------------

1. Download the zip file, unpack and copy the *folder* to your arduino sketches directory.

2. Install the [SMART response XE library](https://github.com/fdufnews/SMART-Response-XE-Low_level) it is [bitbank2's library](https://github.com/bitbank2/SmartResponseXE) with some modification. Unzip the file and copy the *folder* in a hardware directory as explained in the readme.md of the library.
3. Install the [MemoryFree Library](https://github.com/McNeight/MemoryFree)
4. You may want to add/suppress some features.

    a. in host.h, SHOW_SPLASHSCREEN tells the compiler to add or not the splashscreen when the terminal is powered on.
    
    b. in host.h, SD_CARD tells the compiler to add or not support for an sd card
    
    c. in basic.h MEMORY_SIZE can change the size of the memory allocated to basic

BASIC Language
--------------
See [Basic Cheatsheet](BasicCheatsheet.md)


Configuration / Status
-------------
In any mode, 8 lines are kept on bottom of screen to display status of the system.
First the state of the battery, the font used, the state of the SD card, the memory usage.

* The system gives the user the option to switch from one font to another. There are 4 fonts:
  * **normal**, using a 9x8 matrix and giving 17 lines of 42 characters
  * **small**, using a 6x8 matrix and giving 17 lines of 64 characters (really, really small difficult to use for poor old eyes)SHOW_SPLASHSCREENSHOW_SPLASHSCREEN
  * **medium**, using a 12x16 matrix and giving 8 lines of 32 characters
  * **large**, using a 15x16 matrix and giving 8 lines of 25 characters
The medium and large font leave 8 lines at bottom of screen so it was decided to do the same for the normal and small one. Theese 8 lines are used to display the status.
* SD card state, can be one of:
  * NO SD CARD
  * SD CARD OK
  * the name of the last loaded or saved file 
* Memory usage displays 2 numbers:
  * BASIC free mem, the BASIC interpreter has a reserved buffer used to store program and data. The value displayed gives how much of the buffer is free
  * C free mem, the second number is the size of the stack left to the C that is running the interpreter

Currently, BASIC has a 10kB buffer and the remaining 6kB are for C. After setup, there are a bit less than 4kB of free RAM for C. As much of the memory is allocated during setup, 4kB seems to be very conservative. It can probably be reduced if BASIC interpreter needs more free memory. MEMORY_SIZE, defined in basic.h, defines the size of the BASIC memory. For detailed information on memory usage see [memory.md](memory.md)

Not part of the BASIC language, there is support for the MENU key (SYM + space).
That key is used to make a call to the host_setConf function in which user can change the font used with the display.
You can see you have entered configuration mode as the cursor stops blinking. Using UP and DOWN keys the user can change the font. The name of the selected font is displayed on the status line.
You leave configuration mode by pressing ENTER (DEL key) or by pressing MENU (SYM + space).
The screen buffer is cleared, but the current line is kept and displayed on top with cursor at the right position, if user was currently editing a line when switching to configuration mode.

Usually, a BASIC program can be halted with a CTRL+C. As there are no control keys on the terminal, the square root key has been chosen for that.
