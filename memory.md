Usage of memory in BASIC

As I spent some time understanding how mem[] was used, I have made this memo to keep the information for later use

mem[] size is defined in basic.h
```
#define MEMORY_SIZE	(10L*1024L)
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
