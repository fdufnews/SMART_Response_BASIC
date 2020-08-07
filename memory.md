Usage of memory in BASIC

As I spent some time understanding how mem[] was used, I have made this memo to keep the information for later use

# Memory map
mem[] size is defined in basic.h
```
#define MEMORY_SIZE (10L*1024L)
extern unsigned char mem[];
```
mem is declared in SMART_Response_BASIC.ino

mem is sliced in different subsections that are adressed using variables used as indexes inside mem
* Program
* Stack
* Variables
* Gosub

|Direction|ID| Use |
|------|-----|----|
| ` V ` |  0 | |
| ` V ` |    | BASIC Program |
| ` V ` | sysPROGEND | |
| ` V ` |sysSTACKSTART | |
| ` V ` |  | evaluation stack |
| ` V ` | sysSTACKEND  |
|       |  | Free memory |
| ` ^ ` |sysVARSTART | |
| ` ^ ` |   | Variables |
| ` ^ ` | sysVAREND | |
| ` ^ ` | sysGOSUBSTART |
| ` ^ ` |   |  Temporary storage for gosub|
| ` ^ ` | sysGOSUBEND | |
| ` ^ ` | MEMORY_SIZE | |

After reset:
* sysPROGEND, sysSTACKSTART, sysSTACKEND are set to 0
* sysGOSUBEND, sysGOSUBSTART, sysVAREND, sysVARSTART are set to MEMORY_SIZE

They grow up or down when necessary.

It shall be noticed that using GOSUB is at some cost. When pushing (or pulling) return addresses on the GOSUB stack, the software must shift the variables stack to make (or recover) some room. The same apply to the variables so it is perhaps more efficient to declare the variables at the beginning of the program so there will be no time spent moving the stack during the execution.

# Program space
Program space starts at 0 in mem\[\] and goes up to sysPROGEND

Each line starts with

| size | function |
|---|---|
| int | offset to next line |
| int | line number |

Next are tokens as defined in basic.h
#define TOKEN_EOL       0
#define TOKEN_IDENT     1   // special case - identifier follows
#define TOKEN_INTEGER   2   // special case - integer follows (line numbers only)
#define TOKEN_NUMBER    3   // special case - number follows
#define TOKEN_STRING    4   // special case - string follows

#define TOKEN_LBRACKET  8
#define TOKEN_RBRACKET  9
#define TOKEN_PLUS      10
#define TOKEN_MINUS     11
#define TOKEN_MULT      12

up to

#define TOKEN_DIR       64
#define TOKEN_DELETE    65
#define TOKEN_ASC       66
#define TOKEN_SLEEP     67
#define TOKEN_TONE      68
#define TOKEN_NOTONE    69

Lines are terminated with TOKEN_EOL (00)
There is no token indicating end of program. End of program is detected when program pointer equates sysPROGEND
