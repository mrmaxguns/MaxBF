# MaxBF

A bulletproof, tested, Brainfuck interpreter written in C.

## Specification

For full documentation, see [DOCUMENTATION.md](DOCUMENTATION.md).

- Defined commands are `>`, `<`, `+`, `-`, `.`, `,`, `[` and `]`. All other characters
  are ignored.
- The array for data (tape) is dynamically allocated, and therefore extends 'infinitely'
  to the right (as long as more memory can be allocated). Going backwards from the first
  cell causes an error.
- Each cell is implemented as an unsigned character, and is guaranteed to be one bit. This
  means 8 bytes on most systems.
- Jump instructions `[` and `]` must be properly nested or the interpreter exits with an
  error.
- If an EOF is encountered during input, the cell is set to 0.

## Build and run

The program is configured to be built by CMake. It is written in one file using
standard C99 and can therefore be compiled by any spec-compliant compiler.

Build and install MaxBF:

```sh
cmake -S . -B build
cmake --build build
cmake --build build --target install
```

Run MaxBF:

```sh
maxbf FILE
```

See complete instructions at [DOCUMENTATION.md](DOCUMENTATION.md#compilation).

## Test

With CMake:

```
ctest
```

Or with the generated Makefile:

```
make test
```

## License

MaxBF is distributed as free software under the GNU GPL V3 license.
