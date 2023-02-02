/**
 * Test MaxBF. Some test cases taken from http://brainfuck.org/, written by
 * Daniel B Cristofani (cristofdathevanetdotcom) and shared under the Creative
 * Commons Attribution-ShareAlike 4.0 International License.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "minunit.h"


/*** Mock functions ***/
#define TEST_BUF_SIZE 1000
char mock_input_buf[TEST_BUF_SIZE] = { 0 };
char mock_output_buf[TEST_BUF_SIZE] = { 0 };

int (*old_fgetc)(FILE *stream) = fgetc;
static int mock_fgetc(FILE *stream)
{
    // If we are reading from stdin, we should use the mock input stream.
    // Otherwise, we should use the actual stream. This is because fgetc is used
    // to read characters in from the program file as well as reading in
    // characters from the program input stream.
    if (stream == stdin) {
        int result = (int)mock_input_buf[0];
        // Shift input one to the left.
        memmove(mock_input_buf, &mock_input_buf[1], TEST_BUF_SIZE - 1);
        if (result == 0) return EOF;
        return result;
    } else {
        return old_fgetc(stream);
    }
}
#undef fgetc
#define fgetc(stream) mock_fgetc(stream)

static int mock_fputc(int c, FILE *stream)
{
    size_t len = strlen(mock_output_buf);
    mock_output_buf[len] = (char)c;
    mock_output_buf[len + 1] = '\0';
    return c;
}
#undef fputc
#define fputc(c, stream) mock_fputc(c, stream)

/*** File to test. ***/
#include "maxbf.c"


int tests_run = 0; // Number of tests run.
int tmpnam_calls = 0; // Track calls to tmpnam to compare against TMP_MAX.


/*** Setup functions ***/
FILE *create_file_from_string(const char *s)
{
    char file_name[L_tmpnam];
    if (tmpnam_calls == TMP_MAX || tmpnam(file_name) == NULL) {
        goto error;
    }
    tmpnam_calls++;

    FILE *fp = fopen(file_name, "w+");
    if (fp == NULL) {
        goto error;
    }

    if (fputs(s, fp) == EOF) {
        fclose(fp);
        goto error;
    }
    fseek(fp, 0L, SEEK_SET);

    return fp;

error:
    puts("Could not generate temporary file for tests.");
    exit(EXIT_FAILURE);
}

void buf_cleanup(void)
{
    // These buffers are defined along with the mock functions in maxbf.c.
    mock_input_buf[0] = '\0';
    mock_output_buf[0] = '\0';
}


/*** Utilities ***/

/**
 * Check the interpreter's output given certain inputs.
 * @param program         The Brainfuck program to run as a string.
 * @param input           The input string fed into the program. Can be NULL.
 * @param debug_enabled   Whether the # command should be enabled.
 * @param expected_output The string to compare the output of the program to.
 *                        Can be NULL.
 * @param expected_status The expected ExecutionStatus value returned by the
 *                        interpreter.
 * @return True only if the expected output and status match the values returned
 *         by the interpreter.
 */
bool test_interpreter(const char *program, const char *input, bool debug_enabled,
                      const char *expected_output, ExecutionStatus expected_status)
{
    FILE *fp = create_file_from_string(program);
    struct interpreter_config config = {.input_file=NULL, .output_file=NULL,
                                        .debug_enabled=debug_enabled};

    if (input != NULL) {
        strcpy(mock_input_buf, input);
    }

    ExecutionStatus status = execute_brainfuck_from_stream(fp, stdin, stdout, &config);

    bool result;
    if (expected_output == NULL) {
        result = status == expected_status;
    } else {
        result = strcmp(mock_output_buf, expected_output) == 0
                 && status == expected_status;
    }

    // Cleanup.
    fclose(fp);
    buf_cleanup();

    return result;
}


/*** Test cases ***/
static char *test_hello_world()
{
    bool result = test_interpreter(">++++++++[<+++++++++>-]<.>++++[<+++++++>-]<"
                                   "+.+++++++..+++.>>++++++[<+++++++>-]<++.----"
                                   "--------.>++++++[<+++++++++>-]<+.<.+++.----"
                                   "--.--------.>>>++++[<++++++++>-]<+.",
                                   NULL, false, "Hello, World!", STATUS_OK);

    mu_assert("Error, Hello world failed.", result);
    return 0;
}

static char *test_array_size()
{
    bool result = test_interpreter("++++[>++++++<-]>[>+++++>+++++++<<-]>>++++<["
                                   "[>[[>>+<<-]<]>>>-]>-[>+>+<<-]>]+++++[>+++++"
                                   "++<<++>-]>.<<.", NULL, false, "#\n", STATUS_OK);


    mu_assert("Error, Check for cell 30,000 failed.", result);
    return 0;
}

static char *test_left_bound()
{
    bool result = test_interpreter("<", NULL, false, NULL, STATUS_ERR_LBOUND);

    mu_assert("Error, Moving left from cell 0 did not result in an error.", result);
    return 0;
}

static char *test_improper_nesting_1()
{
    bool result = test_interpreter("[[][][[]]", NULL, false, NULL, STATUS_ERR_NESTING);

    mu_assert("Error, Improper nesting (extra [) did not cause an error.", result);
    return 0;
}

static char *test_improper_nesting_2()
{
    bool result = test_interpreter("[[]][[]", NULL, false, NULL, STATUS_ERR_NESTING);

    mu_assert("Error, Improper nesting (extra ]) did not cause an error.", result);
    return 0;
}

static char *test_ignore_characters()
{
    bool result = test_interpreter("abcd[efg]123?", NULL, false, NULL, STATUS_OK);

    mu_assert("Error, Invalid characters were not ignored.", result);
    return 0;
}

static char *test_bracket_skipping()
{
    bool result = test_interpreter("[This: < and this [<] shouldn't cause an er"
                                   "ror]", NULL, false, NULL, STATUS_OK);

    mu_assert("Error, Valid nesting caused an error.", result);
    return 0;
}

static char *test_obscure_problems()
{
    bool result = test_interpreter("[]++++++++++[>>+>+>++++++[<<+<+++>>>-]<<<<-"
                                   "]\"A*$\";?@![#>>+<<]>[>>]<<<<[>++<[-]]>.>.",
                                   NULL, false, "H\n", STATUS_OK);

    mu_assert("Error, Check for several obscure errors failed.", result);
    return 0;
}

static char *test_input()
{
    bool result = test_interpreter(",.,.,,.>,.", "Y\n&?.", false, "Y\n?.", STATUS_OK);

    mu_assert("Error, Input check failed.", result);
    return 0;
}

static char *test_io()
{
    // Input: Newline + EOF.
    bool result = test_interpreter(">,>+++++++++,>+++++++++++[<++++++<++++++<+>"
                                   ">>-]<<.>.<<-.>.>.<<.", "\n", false, "LB\nLB\n",
                                   STATUS_OK);

    mu_assert("Error, I/O check failed.", result);
    return 0;
}


/*** Test Execution. ***/
static char *all_tests()
{
    mu_run_test(test_hello_world);
    mu_run_test(test_array_size);
    mu_run_test(test_left_bound);
    mu_run_test(test_improper_nesting_1);
    mu_run_test(test_improper_nesting_2);
    mu_run_test(test_ignore_characters);
    mu_run_test(test_bracket_skipping);
    mu_run_test(test_obscure_problems);
    mu_run_test(test_input);
    mu_run_test(test_io);

    return 0;
}

int main(void)
{
    char *result = all_tests();
    if (result != 0) {
        printf("%s\n", result);
    } else {
        printf("ALL TESTS PASSED\n");
    }
    printf("Tests run: %d\n", tests_run);

    return result != 0;
}
