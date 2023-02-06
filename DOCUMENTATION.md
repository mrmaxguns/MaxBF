# MaxBF Documentation

MaxBF is an interpreter for [Brainfuck](https://en.wikipedia.org/wiki/Brainfuck).

Contents:
- [Compilation](#compilation)
- [Usage](#usage)
- [Specification](#specification)

## Compilation

The easiest way to compile MaxBF using CMake. This requires [CMake to be installed][cmake_install],
along with a build system supported by CMake.

Then, run:

```sh
cmake -S . -B build # To make a build directory
cmake --build build # To build MaxBF
```

To install MaxBF, use:

```sh
cmake --build build --target install
```

## Usage

```
Usage: maxbf [OPTIONS] FILE
A bulletproof interpreter for Brainfuck.
  -h, --help                 Print a help message
  -v, --version              Print current MaxBF version
  -i, --input-file=FILE      Specify a file as input for the brainfuck program
  -o, --output-file=FILE     Specify a file as output for the brainfuck program
  -d, --debug                Enable the # command for debugging
```

## Specification

### The Program Tape

When a program is executed by MaxBF a tape of one-byte cells of unsigned numbers is created.
Each one-byte cell is at least 8 bits in size (although it may be more on some
computers). This means that the value in each cell ranges from 0 to at least 255.
Wraparound happens if a cell is overflown.

The tape is dynamically allocated, and thus extends 'infinitely' forward (`+`)
from the initial position, as long as more memory can be allocated. Attempting
to go backward (`-`) from the starting position results in an error.

### The Data Pointer

The data pointer points to the current cell in the program tape. It can be moved
to point to different cells in the tape, as long as they are within its range.
When the pointer moves from one cell to another, the data in the cell is saved.

#### The Instruction Pointer

The instruction pointer keeps track of the current position within the Brainfuck
program.

### Commands

Brainfuck supports 8 commands. The interpreter starts at the beginning of the input
file and moves from left to right, reading each character. A character that is
not a valid Brainfuck command is ignored, effectively treated as a comment.

#### Tape movement

The `>` command increments the data pointer.

The `<` command decrements the data pointer.

#### Byte manipulation

The `+` command increases the byte at the currently pointed cell, wrapping to `0`
if incremented past the maximum value.

The `-` command decreases the byte at the currently pointed cell, wrapping to the
maximum value if decremented past `0`.

#### I/O

The `.` command prints the character to standard output representing the value
at the currently pointed cell in the given character set (typically ASCII).

The `,` command reads in a character from standard input and converts it to its
numeric form in the current character set. If EOF is encountered, the cell is
instead set to `0`.

#### Instruction pointer manipulation

The `[` command jumps to the matching `]` command if the currently pointed value
is `0`. Otherwise, execution continues normally.

The `]` command jumps to the matching `[` command if the currently pointed value
is not `0`. Otherwise, execution continues normally.

The `[` and `]` commands must be properly nested (each `[` must have a matching `]`).
Otherwise, the interpreter prints an error message.

#### Debugging

The `#` command can be enabled with the `--debug` flag. If enabled, the interpreter
prints the current pointed value and surrounding cells.

[cmake_install]: https://cmake.org/download/
