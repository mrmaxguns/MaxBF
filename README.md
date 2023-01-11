# MaxBF

A Brainfuck interpreter written in C.

## Build

The program is configured to be built by CMake. It is written in one file using
standard C99 and can therefore be compiled by any spec-compliant compiler.

Example on GNU/Linux:

```sh
mkdir build
cd build
cmake ..
make
```

Then:

```sh
./maxbf FILE
```

## Specification

- Defined commands are `>`, `<`, `+`, `-`, `.`, `,`, `[` and `]`. All other characters
  are ignored.
- The array for data (tape) is dynamically allocated, and therefore extends 'infinitely'
  to the right (as long as more memory can be allocated). Going backwards from the first
  cell causes an error.
- Each cell is implemented as an unsigned character, and is guaranteed to be one bit. This
  means 8 bytes on most systems.
- Jump instructions `[` and `]` must be properly nested or the interpreter exits with an
  error.
- If an EOF is encountered during input, the cell is set to 0. The same thing applies if
  the input was a value larger than the size of a character.
