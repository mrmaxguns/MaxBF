/**
 * MaxBF: A Brainfuck interpreter.
 */

#include <ctype.h>
#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define INITIAL_TAPE_SIZE       1000
#define INITIAL_JUMP_STACK_SIZE 100

#define TOK_RIGHT      '>'
#define TOK_LEFT       '<'
#define TOK_INCREMENT  '+'
#define TOK_DECREMENT  '-'
#define TOK_OUTPUT     '.'
#define TOK_INPUT      ','
#define TOK_JUMP_ZERO  '['
#define TOK_JUMP_NZERO ']'

#ifdef TESTING // Mock functions for testing.
#define TEST_BUF_SIZE 1000
char mock_input_buf[TEST_BUF_SIZE] = { 0 };
char mock_output_buf[TEST_BUF_SIZE] = { 0 };

static inline int mock_getchar(void)
{
    int result = (int)mock_input_buf[0];
    // Shift input one to the left.
    memmove(mock_input_buf, &mock_input_buf[1], TEST_BUF_SIZE - 1);
    if (result == 0) {
        return EOF;
    }
    return result;
}
#undef getchar
#define getchar() mock_getchar()

static inline int mock_putchar(int c)
{
    size_t len = strlen(mock_output_buf);
    mock_output_buf[len] = (char)c;
    mock_output_buf[len + 1] = '\0';
    return c;
}
#undef putchar
#define putchar(c) mock_putchar(c)

#endif // ifdef TESTING

#define OK_OR_ERROR(fn, ...)                  \
    do {                                      \
        ExcecutionStatus s = fn(__VA_ARGS__); \
        if (s != STATUS_OK)                   \
            return s;                         \
    } while (0);

#define OK_OR_GOTO_ERROR(fn, ...)             \
    do {                                      \
        ExcecutionStatus s = fn(__VA_ARGS__); \
        if (s != STATUS_OK) {                 \
            status = s;                       \
            goto error;                       \
        }                                     \
    } while (0);

/** Represent interpreter errors. */
typedef enum {
    STATUS_OK,          /** No errors. */
    STATUS_ERR_ALLOC,   /** The interpreter failed to dynamically allocate memory. */
    STATUS_ERR_LBOUND,  /** The user went past the start of the tape. */
    STATUS_ERR_NESTING, /** The user made an error with nesting brackets (jumps) []. */
} ExcecutionStatus;

/**
 * Represents the brainfuck tape which the program can manipulate.
 */
typedef struct {
    unsigned char *data;    /** The array of characters representing the tape. */
    size_t size;            /** Array size (to check if more needs to be allocated). */
    unsigned char *pointer; /** The pointer to the current section of the tape. */
} Tape;

/**
 * The jump stack holds the file positions of all the jump-if-zero ([) instructions so
 * that when the matching (]) instruction is found, it is possible to jump back.
 */
typedef struct {
    fpos_t *data;     /** The array of jump-if-zero instruction positions. */
    size_t size;      /** Array size (to check if more needs to be allocated). */
    fpos_t *pos;      /** The latest instruction in the array. */
    fpos_t *skip_pos; /** Set to NULL by default. Set to an instruction if it is a skip
                          instruction so that we know when to ignore nested braces. */
} JumpStack;

/** Excecute a brainfuck program from a FILE stream. */
ExcecutionStatus excecute_brainfuck_from_stream(FILE *fp);

/** Given a Tape, allocate data and initialize all values. Return false on allocation failure. */
bool init_tape(Tape *tape);

/** Deallocate Tape data. */
void destroy_tape(Tape *tape);

/** Given a JumpStack, allocate data and initialize all values. Return false on allocation failure. */
bool init_jump_stack(JumpStack *jump_stack);

/** Deallocate JumpStack data. */
void destroy_jump_stack(JumpStack *jump_stack);

/** Move the tape pointer to the right, allocating more memory if necessary. */
ExcecutionStatus tape_move_right(Tape *tape, JumpStack *jump_stack);

/** Move the tape pointer to the left, ensuring that the user does not go past the start of the tape. */
ExcecutionStatus tape_move_left(Tape *tape, JumpStack *jump_stack);

/** Increment the value currently pointed by the tape. */
ExcecutionStatus tape_increment(Tape *tape, JumpStack *jump_stack);

/** Decrement the value currently pointed by the tape. */
ExcecutionStatus tape_decrement(Tape *tape, JumpStack *jump_stack);

/** Print the value currently pointed by the tape. */
ExcecutionStatus tape_output(Tape *tape, JumpStack *jump_stack);

/** Read in a character and store it in the place currently pointed by the tape. */
ExcecutionStatus tape_input(Tape *tape, JumpStack *jump_stack);

/** Add the current jump if zero to the jump stack and enable skipping if the value is 0. */
ExcecutionStatus tape_jump_if_zero(Tape *tape, JumpStack *jump_stack, fpos_t *pos);

/** Push a value to the jump stack, allocating more memory if necessary. */
ExcecutionStatus jump_stack_push(JumpStack *jump_stack, fpos_t *pos);

/** Remove the matching jump if zero from the jump stack and jump if current value is not zero. */
ExcecutionStatus tape_jump_if_not_zero(Tape *tape, JumpStack *jump_stack, FILE *fp);

/** Pop a value from the jump stack, returning STATUS_ERR_NESTING if there's nothing to pop. */
ExcecutionStatus jump_stack_pop(JumpStack *jump_stack, fpos_t *pos);

static inline void exit_with_error(char *msg)
{
    printf("ERROR: %s\n", msg);
    exit(EXIT_FAILURE);
}

static inline void warn(char *msg)
{
    printf("WARNING: %s\n", msg);
}

#ifndef TESTING
int main(int argc, char *argv[])
{
    if (argc != 2) {
        exit_with_error("Usage: maxbf FILE");
    }

    FILE *fp = fopen(argv[1], "r");
    if (fp == NULL) {
        exit_with_error("Could not open file.");
    }

    switch (excecute_brainfuck_from_stream(fp)) {
    case STATUS_OK:
        break;
    case STATUS_ERR_ALLOC:
        exit_with_error("Error while allocating memory.");
        break;
    case STATUS_ERR_LBOUND:
        exit_with_error("The program went past the start of the tape.");
        break;
    case STATUS_ERR_NESTING:
        exit_with_error("Improperly nested jumps [ and ].");
        break;
    }

    fclose(fp);
    return EXIT_SUCCESS;
}
#endif // ifndef TESTING

ExcecutionStatus excecute_brainfuck_from_stream(FILE *fp)
{
    // Initialize tape and jump stack.
    Tape tape;
    if (!init_tape(&tape)) {
        return STATUS_ERR_ALLOC;
    }
    JumpStack jump_stack;
    if (!init_jump_stack(&jump_stack)) {
        destroy_tape(&tape);
        return STATUS_ERR_ALLOC;
    }

    // Track the current position to record the positions of jumps.
    fpos_t pos;
    fgetpos(fp, &pos);

    // This will be set in case of an error.
    ExcecutionStatus status = STATUS_OK;

    int ch;
    while ((ch = fgetc(fp)) != EOF) {
        switch (ch) {
        case TOK_RIGHT:
            OK_OR_GOTO_ERROR(tape_move_right, &tape, &jump_stack);
            break;
        case TOK_LEFT:
            OK_OR_GOTO_ERROR(tape_move_left, &tape, &jump_stack);
            break;
        case TOK_INCREMENT:
            OK_OR_GOTO_ERROR(tape_increment, &tape, &jump_stack);
            break;
        case TOK_DECREMENT:
            OK_OR_GOTO_ERROR(tape_decrement, &tape, &jump_stack);
            break;
        case TOK_OUTPUT:
            OK_OR_GOTO_ERROR(tape_output, &tape, &jump_stack);
            break;
        case TOK_INPUT:
            OK_OR_GOTO_ERROR(tape_input, &tape, &jump_stack);
            break;
        case TOK_JUMP_ZERO:
            OK_OR_GOTO_ERROR(tape_jump_if_zero, &tape, &jump_stack, &pos);
            break;
        case TOK_JUMP_NZERO:
            OK_OR_GOTO_ERROR(tape_jump_if_not_zero, &tape, &jump_stack, fp);
            break;
        }

        // Update position in file.
        fgetpos(fp, &pos);
    }

    // If there are any opening parentheses that weren't closed, we tell the user now.
    if (jump_stack.pos != jump_stack.data) {
        status = STATUS_ERR_NESTING;
        goto error;
    }

error:
    destroy_tape(&tape);
    destroy_jump_stack(&jump_stack);
    return status;
}

bool init_tape(Tape *tape)
{
    tape->data = calloc(INITIAL_TAPE_SIZE, 1);
    if (tape->data == NULL) {
        return false;
    }
    tape->size = INITIAL_TAPE_SIZE;
    tape->pointer = tape->data;

    return true;
}

void destroy_tape(Tape *tape)
{
    free(tape->data);
}

bool init_jump_stack(JumpStack *jump_stack)
{
    jump_stack->data = malloc(sizeof(*jump_stack->data) * INITIAL_JUMP_STACK_SIZE);
    if (jump_stack->data == NULL) {
        return false;
    }
    jump_stack->size = INITIAL_JUMP_STACK_SIZE;
    jump_stack->pos = jump_stack->data;
    jump_stack->skip_pos = NULL;

    return true;
}

void destroy_jump_stack(JumpStack *jump_stack)
{
    free(jump_stack->data);
}

ExcecutionStatus tape_move_right(Tape *tape, JumpStack *jump_stack)
{
    if (jump_stack->skip_pos != NULL)
        return STATUS_OK;

    // If we have moved to the edge of the tape.
    if (tape->pointer == tape->data + tape->size - 1) {
        size_t pointer_offset = tape->pointer - tape->data;

        size_t original_size = tape->size;
        tape->size *= 2;

        unsigned char *temp = realloc(tape->data, tape->size);
        if (temp == NULL)
            return STATUS_ERR_ALLOC;
        tape->data = temp;

        // Initialize the new memory to 0.
        memset(tape->data + original_size, 0, tape->size - original_size);

        // Update the tape pointer, just in case realloc copied the memory into a different place.
        tape->pointer = tape->data + pointer_offset;
    }
    tape->pointer++;
    return STATUS_OK;
}

ExcecutionStatus tape_move_left(Tape *tape, JumpStack *jump_stack)
{
    if (jump_stack->skip_pos != NULL)
        return STATUS_OK;

    if (tape->pointer == tape->data) {
        return STATUS_ERR_LBOUND;
    }
    tape->pointer--;
    return STATUS_OK;
}

ExcecutionStatus tape_increment(Tape *tape, JumpStack *jump_stack)
{
    if (jump_stack->skip_pos != NULL)
        return STATUS_OK;

    (*tape->pointer)++;
    return STATUS_OK;
}

ExcecutionStatus tape_decrement(Tape *tape, JumpStack *jump_stack)
{
    if (jump_stack->skip_pos != NULL)
        return STATUS_OK;

    (*tape->pointer)--;
    return STATUS_OK;
}

ExcecutionStatus tape_output(Tape *tape, JumpStack *jump_stack)
{
    if (jump_stack->skip_pos != NULL)
        return STATUS_OK;

    unsigned char ch = *tape->pointer;
    putchar(ch);
    return STATUS_OK;
}

ExcecutionStatus tape_input(Tape *tape, JumpStack *jump_stack)
{
    if (jump_stack->skip_pos != NULL)
        return STATUS_OK;

    int ch = getchar();
    if (ch == EOF) {
        ch = 0;
    }
    *tape->pointer = (char)ch;
    return STATUS_OK;
}

ExcecutionStatus tape_jump_if_zero(Tape *tape, JumpStack *jump_stack, fpos_t *pos)
{
    // Add current value to stack.
    OK_OR_ERROR(jump_stack_push, jump_stack, pos);

    // Ignore current value if in skip mode.
    if (jump_stack->skip_pos != NULL)
        return STATUS_OK;

    // If the value is indeed zero, enable skip to "jump" to the closing ].
    if (*tape->pointer == 0) {
        jump_stack->skip_pos = jump_stack->pos;
    }

    return STATUS_OK;
}

ExcecutionStatus jump_stack_push(JumpStack *jump_stack, fpos_t *pos)
{
    if (jump_stack->pos == jump_stack->data + jump_stack->size - 1) {
        size_t pos_offset = jump_stack->pos - jump_stack->data;
        size_t skip_pos_offset = 0;
        if (jump_stack->skip_pos != NULL) {
            skip_pos_offset = jump_stack->skip_pos - jump_stack->data;
        }

        jump_stack->size *= 2;
        fpos_t *temp = realloc(jump_stack->data, sizeof(*temp) * jump_stack->size);
        if (temp == NULL)
            return STATUS_ERR_ALLOC;
        jump_stack->data = temp;

        // Update the positions, just in case realloc copied the memory into a different place.
        jump_stack->pos = jump_stack->data + pos_offset;
        if (jump_stack->skip_pos != NULL) {
            jump_stack->skip_pos = jump_stack->data + skip_pos_offset;
        }
    }

    jump_stack->pos++;
    *jump_stack->pos = *pos;
    return STATUS_OK;
}

ExcecutionStatus tape_jump_if_not_zero(Tape *tape, JumpStack *jump_stack, FILE *fp)
{
    // If we have finally matched the [ that initiated the skip.
    if (jump_stack->skip_pos == jump_stack->pos) {
        jump_stack->skip_pos = NULL;
    }

    fpos_t pos;
    OK_OR_ERROR(jump_stack_pop, jump_stack, &pos);

    // Jump if not zero.
    if (*tape->pointer != 0) {
        fsetpos(fp, &pos);
    }

    return STATUS_OK;
}

ExcecutionStatus jump_stack_pop(JumpStack *jump_stack, fpos_t *pos)
{
    // If we have nothing to pop, that means a [ could not be found to match the user's ].
    if (jump_stack->pos == jump_stack->data) {
        return STATUS_ERR_NESTING;
    }
    *pos = *jump_stack->pos;
    jump_stack->pos--;
    return STATUS_OK;
}
