# cout

The C Port of the __zigout__ program.

A makefile for [GNU Make](https://www.gnu.org/software/make/) is include for easy builds:

To build:
```shell
RELEASE=1 make
```

To run: 
```shell
RELEASE=1 make run
```
 
**Dependencies:**

- SDL2 and SDL2-TTF (Ubuntu: `sudo apt install libsdl2-dev libsdl2-ttf-dev`)


## Build without make

To build the game without make compile the file `nobuild.c`:
```shell
cc nobuild -o nobuild
```

Then build the game using:
```shell
./nobuild
```

To run:
```shell
./nobuild run
```

**Note**

When linking errors appear with SDL2 you need to modify most probably the SLD2 include directories in the linker flags in `nobuild.c`.
The line to modify is commented with a note. You do not need to rebuild the `nobuild` executable it will rebuild itself when `nobuild.c` gets modified, i.e. just run it again after the modification.
