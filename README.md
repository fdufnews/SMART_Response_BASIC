
#Currently under development not stable

SMART Response Basic
=============

The SMART Response Basic is based on the original work of [Robin Edwards](https://github.com/robinhedwards/ArduinoBASIC) and on the way [Tamakichi-San](https://github.com/Tamakichi/ArduinoBASIC) has modify the software to change its input and output streams.

Original README.md file can be [read here](README_original.md)

My aim is to make a mobile programming platform that can be used on the lab or on the road to test components. I have tried to use Bus Pirate but I have found its syntax a bit esoteric and have never used it as much as I originaly plan.

The SMART Response XE terminal is a very attractive platform :
* 384x136 pixels LCD screen,
* a good keyboard,
* a CPU with 128kB of program flash memory, 16kB of RAM, 4k of EEPROM
* 128kO of external flash
* it can work for hours on 4 AAA batteries.

Nevertheless, there are drawbacks. The main one is how the display works. There is a memory plane inside the display that hold the graphic but this memory cannot be read back when the display interface is in serial mode. This make it difficult to make addition of graphics over existing ones. Especially as the memory is written 3 pixels at a time. If you want to toggle one pixel you can potentially overwrite two others.

So, at first I will try to make BASIC work in text mode only.

TODO list
----------
- [ ] make BASIC work with keyboard and LCD display without any other modification
- [ ] add save/restore BASIC programs on external Flash
- [ ] add SPI support
- [ ] add I2C support

Prerequisites
-------------
1: A SMART Response XE terminal.

2: (Optional) A Piezoelectric buzzer for keyboard clicks and other sounds.

5: (Optional) An LED to show some status or debug software.

Getting Started
---------------
1: Download the zip file, unpack and copy the *folder* to your arduino sketches directory.

2: Install the PS/2 keyboard library if required. I've included the version I used in the zip file.

3: Install the SSD1306ASCII library. The normal Adafruit library is too RAM hungry for this project, so I'm using a massively cut down driver instead. I've modified this library a bit to get fast SPI transfers which improved the screen updating speed by a factor of about 4. The modified version is included in the zip file.

For both libraries, unzip the files and copy the *folder* to your arduino libraries directory.

4: Check your wiring corresponds to the pins in the comments/defines at the top of the Arduino_BASIC file.

BASIC Language
--------------
Variables names can be up to 8 alphanumeric characters but start with a letter e.g. a, bob32
String variable names must end in $ e.g. a$, bob32$
Case is ignored (for all identifiers). BOB32 is the same as Bob32. print is the same as PRINT

Array variables are independent from normal variables. So you can use both:
```
LET a = 5
DIM a(10)
```
There is no ambiguity, since a on its own refers to the simple variable 'a', and a(n) referes to an element of the 'a' array.

```
Arithmetic operators: + - * / MOD
Comparisons: <> (not equals) > >= < <=
Logical: AND, OR, NOT
```

Expressions can be numerical e.g. 5*(3+2), or string "Hello "+"world".
Only the addition operator is supported on strings (plus the functions below).

Commands
```
PRINT <expr>;<expr>... e.g. PRINT "A=";a
LET variable = <expr> e.g. LET A$="Hello".
variable = <expr> e.g. A=5
LIST [start],[end] e.g. LIST or LIST 10,100
RUN [lineNumber]
GOTO lineNumber
REM <comment> e.g. REM ** My Program ***
STOP
CONT (continue from a STOP)
INPUT variable e.g. INPUT a$ or INPUT a(5,3)
IF <expr> THEN cmd e.g. IF a>10 THEN a = 0: GOTO 20
FOR variable = start TO end STEP step
NEXT variable
NEW
GOSUB lineNumber
RETURN
DIM variable(n1,n2...)
CLS
PAUSE milliseconds
POSITION x,y sets the cursor
PIN pinNum, value (0 = low, non-zero = high)
PINMODE pinNum, mode ( 0 = input, 1 = output)
LOAD (from internal EEPROM)
SAVE (to internal EEPROM) e.g. use SAVE + to set auto-run on boot flag
LOAD "filename", SAVE "filename, DIR, DELETE "filename" if using with external EEPROM.
```

"Pseudo-identifiers"
```
INKEY$ - returns (and eats) the last key pressed buffer (non-blocking). e.g. PRINT INKEY$
RND - random number betweeen 0 and 1. e.g. LET a = RND
```

Functions
```
LEN(string) e.g. PRINT LEN("Hello") -> 5
VAL(string) e.g. PRINT VAL("1+2")
INT(number) e.g. INT(1.5)-> 1
STR$(number) e.g. STR$(2) -> "2"
LEFT$(string,n)
RIGHT$(string,n)
MID$(string,start,n)
PINREAD(pin) - see Arduino digitalRead()
ANALOGRD(pin) - see Arduino analogRead()
```
